#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

/*
 接收到的信息逻辑上可以分成两种，一种是 sender 发来的要传输的数据，另一种是 receiver 发送的 ACK
 这两种信息可以在一个 segment 里面
 */
void TCPConnection::segment_received(const TCPSegment &seg) {
    _receiver.segment_received(seg);
    _sender.ack_received(seg.header().ackno, seg.header().win);

    if (_sender.segments_out().empty()) {
        TCPSegment segment = TCPSegment();
        if (_receiver.ackno().has_value()) {
            segment.header().ackno = _receiver.ackno().value();
        }
        segment.header().win = _receiver.window_size();
        _sender.segments_out().pop();
    } else {
        while (not _sender.segments_out().empty()) {
            TCPSegment segment = _sender.segments_out().front();
            if (_receiver.ackno().has_value()) {
                segment.header().ackno = _receiver.ackno().value();
            }
            segment.header().win = _receiver.window_size();
            _segments_out.push(segment);
            _sender.segments_out().pop();
        }
    }
}

bool TCPConnection::active() const {
    return not _sender.stream_in().eof() or
           not _receiver.stream_out().eof() or
           _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
    return _sender.stream_in().write(data);
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPConnection::end_input_stream() {}

void TCPConnection::connect() {
    _sender.fill_window();
    _segments_out.push(_sender.segments_out().front());
    _sender.segments_out().pop();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
