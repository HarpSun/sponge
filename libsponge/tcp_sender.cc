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

uint64_t TCPSender::bytes_in_flight() const { return {}; }

void TCPSender::fill_window() {
//    stream_in().read()
    size_t buffer_size = stream_in().buffer_size();
    size_t max_payload_size = TCPConfig::MAX_PAYLOAD_SIZE;
    size_t payload_size = min(buffer_size, max_payload_size);
    cout << payload_size;
    TCPSegment segment = TCPSegment();
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
    _ackno = ackno;
    _window_size = window_size;

    if (segments_out().empty()) {
        return;
    }

    // not empty
    TCPSegment seg = segments_out().front();
    while (seg.header().seqno < ackno) {
        segments_out().pop();
        if (segments_out().empty()) {
            break;
        }
        seg = segments_out().front();
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
