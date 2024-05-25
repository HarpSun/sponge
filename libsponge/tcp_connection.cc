#include "tcp_connection.hh"

#include <iostream>
#include <mutex>

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

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

/*
 segment_received 要做的事情有两件
 1. 处理对面发送来的包
 如果是 ack 就要通知 sender 发送更多的包，如果是数据包就要通知 receiver 重组，如果是 rst 就要将连接中断
 2. 往对面发包
 发送对面的包分两种，一种是 ack，还有一种是数据包
 其中 ack 只有收到数据包之后才发送，而数据包是只要 sender 有可发的就发送，并且数据包可以带上 ack 一起发
 */
void TCPConnection::segment_received(const TCPSegment &seg) {
    if (_receiver.listen() and not (seg.header().ack or seg.header().syn)) {
        return;
    }

    // handle received segment
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
    }

    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    if (seg.length_in_sequence_space() > 0) {
        _receiver.segment_received(seg);
        _sender.fill_window();
        if (inbound_stream().eof() and not _sender.stream_in().input_ended()) {
            _linger_after_streams_finish = false;
        }
    }

    // send segment to peer
    if (_sender.segments_out().empty() and seg.length_in_sequence_space() > 0) {
        send_ack();
    } else {
        send_data();
    }

    // reset timer
    _time_since_last_segment_received = 0;
}

void TCPConnection::send_ack() {
    _sender.send_empty_segment();
    send_data();
}

void TCPConnection::send_rst() {
    _sender.send_empty_segment();
    while (not _sender.segments_out().empty()) {
        TCPSegment segment = _sender.segments_out().front();
        _sender.segments_out().pop();
        segment.header().rst = true;
        _segments_out.push(segment);
    }
}

void TCPConnection::send_data() {
    while (not _sender.segments_out().empty()) {
        TCPSegment segment = _sender.segments_out().front();
        _sender.segments_out().pop();
        patch_ack(segment);
        _segments_out.push(segment);
    }
}

void TCPConnection::patch_ack(TCPSegment& segment) {
    if (_receiver.ackno().has_value()) {
        segment.header().ackno = _receiver.ackno().value();
        segment.header().ack = true;
    }
    segment.header().win = static_cast<uint16_t>(_receiver.window_size());
}

bool TCPConnection::active() const {
    bool error = _sender.stream_in().error() and _receiver.stream_out().error();
    bool done = _sender.fin_acked() and
                _receiver.fin_recv() and
                not _linger_after_streams_finish;
    return (not error) and (not done);
}

size_t TCPConnection::write(const string &data) {
    size_t size = _sender.stream_in().write(data);
    _sender.fill_window();
    send_data();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    if (inbound_stream().eof() and _sender.fin_acked()) {
        if (_time_since_last_segment_received >= _cfg.rt_timeout * 10) {
            _linger_after_streams_finish = false;
        }
        return;
    }

    if (_sender.consecutive_retransmissions() >= TCPConfig::MAX_RETX_ATTEMPTS) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        send_rst();
        return;
    }

    _sender.tick(ms_since_last_tick);
    send_data();
}

void TCPConnection::end_input_stream() {
    // receiver has received all data
    _sender.stream_in().end_input();
    // 被动关闭连接
    _sender.fill_window();
    send_data();
}

void TCPConnection::connect() {
    _sender.fill_window();
    send_data();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
