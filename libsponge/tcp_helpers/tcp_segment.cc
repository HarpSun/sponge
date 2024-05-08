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

void TCPSegment::print_tcp_segment() const {
    cout << "======= TCPSegment =========" << endl;
    cout << "header: " << header().seqno << " syn: " << header().syn << " fin: " << header().fin << " ack: " << header().ack
         << " seqno: " << header().seqno << " ackno: " << header().ackno << " checksum: " << header().cksum
         << " window_size:" << header().win << endl;
    cout << "payload: " << payload().size() << " " << payload().str() << endl;
    cout << "======= END =========" << endl;
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
