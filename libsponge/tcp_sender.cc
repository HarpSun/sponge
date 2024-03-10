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

void TCPSender::fill_window() {
    size_t buffer_size = stream_in().buffer_size();
    size_t max_payload_size = TCPConfig::MAX_PAYLOAD_SIZE;
    size_t payload_size = min(buffer_size, max_payload_size);
    cout << "payload_size: " << payload_size << endl;
    cout << "next_seqno: " << next_seqno_absolute() << " bytes_flight: " << bytes_in_flight() << endl;
    // closed
    if (next_seqno_absolute() == 0) {
        cout << "closed" << endl;
        TCPSegment segment = make_segment(0, wrap(0, _isn), true);
        segments_out().emplace(segment);
        _next_seqno = 1;
        _bytes_in_flight = 1;
    }

    // syn_acked
    if (next_seqno_absolute() > bytes_in_flight() and not stream_in().eof()) {
        cout << "syn_acked" << endl;
        _fill_window(_window_size);
    } else if (stream_in().eof() and next_seqno_absolute() == stream_in().bytes_written() + 1) {
        cout << "fin_sent" << endl;
        if (_window_size > 0) {
            TCPSegment segment = make_segment(0, wrap(next_seqno_absolute(), _isn), false, true);
            segments_out().emplace(segment);
            _next_seqno += 1;
            _bytes_in_flight = 1;
        }
    }
}

void TCPSender::_fill_window(uint16_t window_size) {
    if (window_size == 0 or stream_in().buffer_size() == 0) {
        return;
    } else {
        size_t buffer_size = stream_in().buffer_size();
        size_t max_payload_size = TCPConfig::MAX_PAYLOAD_SIZE;
        size_t payload_size = min(
            min(buffer_size, max_payload_size),
            static_cast<size_t>(window_size)
            );
        TCPSegment segment = make_segment(payload_size, wrap(next_seqno_absolute(), _isn));
        segment.print_tcp_segment();
        segments_out().emplace(segment);
        _next_seqno += segment.length_in_sequence_space();
        _fill_window(window_size - payload_size);
    }
}

/*
生成要发送的 TCP 包
*/
TCPSegment TCPSender::make_segment(size_t payload_size, WrappingInt32 seqno, bool syn, bool fin) {
    TCPSegment segment = TCPSegment();
    string payload = stream_in().read(payload_size);
    cout << "__log1: " << payload << endl;
    segment.payload() = std::string(payload);
    segment.print_tcp_segment();
    segment.header().seqno = seqno;
    if (syn) {
        segment.header().syn = true;
    }

    if (fin) {
        segment.header().fin = true;
    }
    return segment;
}

/*
收到 receiver 发送来的 ack 和 window_size 后被调用
记录下来，之后要根据这两个值计算要发送 segment 的大小和数量

同时还要检查 segments_out 中是否有 seqno 小于 ack 的包，如果有的话就从 segments_out 中去掉
这个很好理解，因为小于 ack 的都是已经被接收的，不需要再重新发送，这就是所谓的“累计确认”
*/
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if (ackno > wrap(next_seqno_absolute(), _isn)) {
        // acked segno not sent!
        return;
    }

    _window_size = window_size;
    // _next_seqno = unwrap(ackno, _isn, next_seqno_absolute());
    _bytes_in_flight = wrap(next_seqno_absolute(), _isn) - ackno;

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
