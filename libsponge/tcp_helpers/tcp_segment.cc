#include "tcp_segment.hh"

#include "parser.hh"
#include "util.hh"

#include <variant>
#include <iostream>

using namespace std;

//! \param[in] buffer string/Buffer to be parsed
//! \param[in] datagram_layer_checksum pseudo-checksum from the lower-layer protocol
ParseResult TCPSegment::parse(const Buffer buffer, const uint32_t datagram_layer_checksum) {
    InternetChecksum check(datagram_layer_checksum);
    check.add(buffer);
    if (check.value()) {
        return ParseResult::BadChecksum;
    }

    NetParser p{buffer};
    _header.parse(p);
    _payload = p.buffer();
    return p.get_error();
}

size_t TCPSegment::length_in_sequence_space() const {
    return payload().str().size() + (header().syn ? 1 : 0) + (header().fin ? 1 : 0);
}

string TCPSegment::print_tcp_segment() const {
    string s = "======= TCPSegment =========\n";
    s += "header: " + to_string(header().dport) + " " + to_string(header().sport) + "\n";
    s += "header: " + to_string(header().seqno.raw_value()) + " syn: " + to_string(header().syn) + " fin: " + to_string(header().fin) + " ack: " + to_string(header().ack) + "rst: " + to_string(header().rst)
         + " seqno: " + to_string(header().seqno.raw_value()) + " ackno: " + to_string(header().ackno.raw_value()) + " checksum: " + to_string(header().cksum)
         + " window_size:" + to_string(header().win) + "\n";
    s += "payload: " + to_string(payload().size()) + "\n";
    s += "======= END =========\n";
    return s;
}

//! \param[in] datagram_layer_checksum pseudo-checksum from the lower-layer protocol
BufferList TCPSegment::serialize(const uint32_t datagram_layer_checksum) const {
    TCPHeader header_out = _header;
    header_out.cksum = 0;

    // calculate checksum -- taken over entire segment
    InternetChecksum check(datagram_layer_checksum);
    check.add(header_out.serialize());
    check.add(_payload);
    header_out.cksum = check.value();

    BufferList ret;
    ret.append(header_out.serialize());
    ret.append(_payload);

    return ret;
}
