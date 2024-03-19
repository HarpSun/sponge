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
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _ackno(nullopt) {}

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

/*
 fill_window 负责发送数据到 segments_out 队列中
 为了充分利用带宽，我们会一次发送多个包，包的大小由 buffer_size, window_size, MAX_PAYLOAD_SIZE 三个因素共同决定
 基本上来说，会尽量发送 MAX_PAYLOAD_SIZE 大小的包，但是如果 buffer_size 或 window_size 不足，则用两者之间最小的一个来决定包的大小
 */
void TCPSender::fill_window() {
    if (closed()) {
        cout << "closed" << endl;
        // 还没建立连接的时候，还不知道对面的 window_size，所以就只发送一个 SYN
        TCPSegment segment = make_segment(0, wrap(0, _isn));
        segments_out().emplace(segment);
        _next_seqno = 1;
        _bytes_in_flight = 1;
    } else if (syn_sent() or syn_acked()) {
        cout << "syn_sent or " << "syn_acked" << endl;
        _fill_window();
    } else {
        // fin_sent or fin_acked
    }
}

void TCPSender::_fill_window() {
    size_t max_payload_size = TCPConfig::MAX_PAYLOAD_SIZE;
    while (_window_size > 0 and not fin_sent()) {
        size_t buffer_size = stream_in().buffer_size();
        size_t payload_size = min(
            min(buffer_size, max_payload_size),
            static_cast<size_t>(_window_size)
        );
        // cout << "payload_size: " << payload_size << endl;
        // cout << "next_seqno: " << next_seqno_absolute() << " bytes_flight: " << bytes_in_flight() << endl;
        if (payload_size == 0 and not stream_in().eof()) {
            // buffer 是空的，但是还没有写完所有数据
            break;
        }

        TCPSegment segment = make_segment(payload_size, wrap(next_seqno_absolute(), _isn));
        segments_out().emplace(segment);

        _next_seqno += segment.length_in_sequence_space();
        _bytes_in_flight += segment.length_in_sequence_space();
        _window_size -= segment.length_in_sequence_space();
    }
}

/*
生成要发送的 TCP 包
*/
TCPSegment TCPSender::make_segment(size_t payload_size, WrappingInt32 seqno) {
    TCPSegment segment = TCPSegment();
    string payload = stream_in().read(payload_size);
    cout << "__log1: " << payload << endl;
    segment.payload() = std::string(payload);
    segment.header().seqno = seqno;

    if (closed()) {
        segment.header().syn = true;
    }

    if (stream_in().eof()) {
        segment.header().fin = true;
    }
    segment.print_tcp_segment();
    return segment;
}

/*
收到 receiver 的响应后被调用
其中 ack 和 window_size 需要记录下来，之后要根据这两个值计算要发送 segment 的大小和数量

同时还要检查 segments_out 中是否有 seqno 小于 ack 的包，如果有的话就从 segments_out 中去掉
这个很好理解，因为小于 ack 的都是已经被接收的，不需要再重新发送，这就是所谓的“累计确认”
*/
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if (ackno > wrap(next_seqno_absolute(), _isn)) {
        // ackno 是 sender 没有发送过的数据，不应该出现这种情况
        return;
    }

    _window_size = window_size;
    _bytes_in_flight = wrap(next_seqno_absolute(), _isn) - ackno;
    remove_acknowledged_segments(ackno);
    fill_window();
}

void TCPSender::remove_acknowledged_segments(const WrappingInt32 ackno) {
    size_t queue_length = segments_out().size();
    for (size_t i = 0; i < queue_length; i++) {
        TCPSegment seg = segments_out().front();
        if (seg.header().seqno + seg.length_in_sequence_space() - ackno <= 0) {
            // has been acked
            segments_out().pop();
        } else {
            segments_out().pop();
            segments_out().emplace(seg);
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    DUMMY_CODE(ms_since_last_tick);
}

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {
    if (_ackno.has_value()) {
        TCPHeader header = TCPHeader();
        header.seqno = _ackno.value();
        TCPSegment seg = TCPSegment();
        seg.header() = header;
        segments_out().emplace(seg);
    }
}
