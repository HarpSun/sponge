#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , retx_timer(retx_timeout)
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight;
}

/*
 获取 sender 的状态
 sender 状态比较复杂，写的时候还是要完全参考文档
 */
bool TCPSender::closed() const { return next_seqno_absolute() == 0; }
bool TCPSender::syn_sent() const { return next_seqno_absolute() > 0 and next_seqno_absolute() == bytes_in_flight(); }
bool TCPSender::syn_acked() const {
    return (next_seqno_absolute() > bytes_in_flight() and not stream_in().eof())
            or (stream_in().eof() and next_seqno_absolute() < stream_in().bytes_written() + 2);
}
bool TCPSender::fin_sent() const {
    return stream_in().eof()
           and next_seqno_absolute() == stream_in().bytes_written() + 2
           and bytes_in_flight() > 0;
}
bool TCPSender::fin_acked() const {
    return stream_in().eof()
           and next_seqno_absolute() == stream_in().bytes_written() + 2
           and bytes_in_flight() == 0;
}

// 没数据可发了
bool TCPSender::no_more_to_send() {
    // 没数据可发分两种情况
    // 1. buffer 是空的并且还没写完(注意: 写完了要发送 fin)
    // 2. 已经发送了 fin
    return (stream_in().buffer_empty() and not stream_in().eof()) or fin_sent();
}

/*
 fill_window 负责发送数据到 segments_out 队列中,为了充分利用带宽，我们会一次发送多个 segment
 segment 的大小受到两个因素限制：
    1. MAX_PAYLOAD_SIZE，是 payload 所能携带的最大数据量，而 segment_size 还要再次之上 + 1
    (这里要小心区分 payload_size 和 segment_size，payload_size 是发送数据的大小，而 segment_size 则在次基础之上还包含了 SYN 或 FIN)
    2. window_size，不能发送超过 window_size 大小的包
 所以综上每一次发送 segment 的大小应该由 window_size, MAX_PAYLOAD_SIZE + 1 的最小值决定

 注意：这个函数并不会重发数据，TCP 中的重发只会通过超时来触发，这个函数只负责填充 window
 */
void TCPSender::fill_window() {
    if (closed()) {
        // 还没建立连接的时候，还不知道对面的 window_size，所以就只发送一个 SYN
        send_segment(0);
    } else if (syn_acked()) {
        _fill_window();
    } else {
        // syn_sent 之后只能等着回包，没有 window 可以填充
        // fin_sent 同理，等着回包
        // fin_acked 就结束发送了
    }
}

void TCPSender::_fill_window() {
    // sender 视角下的 window_size，需要扣除掉已经发送但还未确认的数据
    size_t window_size = _window_size > 0 ? _window_size - bytes_in_flight() : 0;
    while (window_size > 0 and not no_more_to_send()) {
        size_t segment_size = min(
            TCPConfig::MAX_PAYLOAD_SIZE + 1,
            static_cast<size_t>(window_size)
        );

        size_t size = send_segment(segment_size);
        window_size -= size;
    }
}

size_t TCPSender::send_segment(size_t segment_size) {
    TCPSegment segment = make_segment(segment_size, wrap(next_seqno_absolute(), _isn));
    segments_out().emplace(segment);
    _outstanding_segments.push(segment);
    if (not retx_timer.running()) {
        retx_timer.start();
    }
    _next_seqno += segment.length_in_sequence_space();
    _bytes_in_flight += segment.length_in_sequence_space();
    return segment.length_in_sequence_space();
}

/*
生成要发送的 TCP 包
*/
TCPSegment TCPSender::make_segment(size_t segment_size, WrappingInt32 seqno) {
    TCPSegment segment = TCPSegment();
    size_t payload_size = min(segment_size, TCPConfig::MAX_PAYLOAD_SIZE);
    string payload = stream_in().read(payload_size);
    segment.payload() = std::string(payload);
    segment.header().seqno = seqno;

    if (closed()) {
        segment.header().syn = true;
    }

    // buffer 全部读完了，并且 segment 还有空间，就带上 fin
    if (stream_in().eof() and payload.length() < segment_size) {
        segment.header().fin = true;
    }
    // segment.print_tcp_segment();
    return segment;
}

/*
收到 receiver 的响应后被调用
其中 ack 和 window_size 需要记录下来，之后要根据这两个值计算要发送 segment 的大小和数量

同时还要检查 segments_out 中是否有 seqno 小于 ack 的包，如果有的话就从 segments_out 中去掉
这个很好理解，因为小于 ack 的都是已经被接收的，不需要再重新发送，这就是所谓的“累计确认”

注意: 如果 receiver 窗口是 0，需要发送一个 1 字节的包，否则就不知道什么时候 receiver 的窗口空出来了
*/
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ackno_abs = unwrap(ackno, _isn, next_seqno_absolute());
    if (ackno_abs > next_seqno_absolute() or ackno_abs < _ackno_abs) {
        // ack 没有发送过的数据, 或者 ack 小于上一次的
        cout << "ackno is invalid" << endl;
        return;
    }

    size_t ackedBytes = ackno_abs - _ackno_abs;
    // 有新确认的数据之后，重置 timer
    if (ackedBytes > 0) {
        retx_timer.reset();
        _consecutive_retransmissions = 0;
    }

    _ackno_abs = ackno_abs;
    _bytes_in_flight -= ackedBytes;
    _window_size = window_size;
    remove_acknowledged_segments(ackno_abs, segments_out());
    remove_acknowledged_segments(ackno_abs, _outstanding_segments);
    if (_window_size == 0) {
        send_segment(1);
    } else {
        fill_window();
    }
}

void TCPSender::remove_acknowledged_segments(uint64_t ackno_abs, queue<TCPSegment> &segments_out) {
    while (not segments_out.empty()) {
        TCPSegment seg = segments_out.front();
        uint64_t last_byte = unwrap(seg.header().seqno, _isn, next_seqno_absolute())
                             + seg.length_in_sequence_space();
        if (last_byte > ackno_abs) {
            break;
        } else {
            // has been acked
            segments_out.pop();
        }
    }
}

/*
 检查是否超过 RTO 时间，如果超过了就重发
 如果 window_size 是 0，不会让 rto 翻倍，这种情况由于不是网络拥塞造成的，只是 receiver 处理慢，所以快速的重发
 注意：syn_ack 之前，window_size 虽然是默认值 0，但是这时候其实还不知道 window_size，所以 rto 也会翻倍
 */
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    retx_timer.tick(ms_since_last_tick);
    if (_outstanding_segments.empty()) {
        retx_timer.stop();
    } else {
        TCPSegment segment = _outstanding_segments.front();
        if (retx_timer.timeout()) {
            segments_out().emplace(segment);
            _consecutive_retransmissions += 1;
            if (_window_size > 0 or syn_sent()) {
                retx_timer.backoff();
            }
            retx_timer.start();
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg = make_segment(0, wrap(next_seqno_absolute(), _isn));
    segments_out().emplace(seg);
}
