class BitStream:

    def __init__(self, bytes) -> None:
        self.raw_bytes = bytes
        self.offset = 0
        self.size = len(bytes) * 8

    def remain_size(self):
        return self.size - self.offset - 1

    def skip(self, n):
        self.offset += n

    def seek(self, n):
        self.offset = n

    def read(self, n):
        index = self.offset // 8
        if index >= len(self.raw_bytes):
            return []
        else:
            byte = self.raw_bytes[index]
            current = self.offset % 8

            bits = []
            i = 0
            while self.offset < self.size and i < n:
                b = (byte >> (7 - current)) & 0b00000001
                bits.append(b)
                self.offset += 1
                byte = self.raw_bytes[self.offset // 8]
                current = self.offset % 8
                i += 1

            return bits

# big endian
def bin2Int(bit_arr):
    res = 0
    n = len(bit_arr)
    for i, b in enumerate(bit_arr):
        res += b * 2 ** (n - i - 1)
    return res


class TCPPacket:

    def __init__(self, data) -> None:
        self.src_port = data["src_port"]
        self.dst_port = data["dst_port"]
        self.seqno = data["seqno"]
        self.ackno = data["ackno"]
        self.ack = data["ack"]
        self.rst = data["rst"]
        self.syn = data["syn"]
        self.fin = data["fin"]
        self.window_size = data["window_size"]
        self.checksum = data["checksum"]
        self.payload = data["payload"]

    @classmethod
    def parse(cls, stream):
        data = {}
        data["src_port"] = bin2Int(stream.read(16))
        data["dst_port"] = bin2Int(stream.read(16))
        data["seqno"] = bin2Int(stream.read(32))
        data["ackno"] = bin2Int(stream.read(32))
        data_offset = bin2Int(stream.read(4))
        stream.skip(4)
        flags = stream.read(8)
        data["ack"] = flags[3]
        data["rst"] = flags[5]
        data["syn"] = flags[6]
        data["fin"] = flags[7]
        data["window_size"] = bin2Int(stream.read(16))
        data["checksum"] = bin2Int(stream.read(16))
        stream.skip(16)
        # option
        header_size = data_offset * 4
        stream.skip(header_size - 20)
        # payload
        data["payload"] = stream.read(stream.remain_size())
        return cls(data)

    def __str__(self):
        return "<TCP src_port: {} dst_port: {} seqno: {} ackno: {} ack: {} rst: {} syn: {} fin: {} checksum: {}".format(
            self.src_port, self.dst_port, self.seqno, self.ackno, self.ack, self.rst, self.syn, self.fin, self.checksum
        )


class IPPacket:

    def __init__(self, data) -> None:
        self.version = data["version"]
        self.header_len = data["header_len"]
        self.total_len = data["total_len"]
        self.ttl = data["ttl"]
        self.protocol = data["protocol"]
        self.checksum = data["checksum"]
        self.src_ip = data["src_ip"]
        self.dst_ip = data["dst_ip"]
        self.payload = data["payload"]

    @classmethod
    def parse(cls, raw_bytes):
        data = {}
        stream = BitStream(raw_bytes)
        data["version"] = bin2Int(stream.read(4))
        data["header_len"] = bin2Int(stream.read(4))
        stream.skip(8)
        data["total_len"] = bin2Int(stream.read(16))
        stream.skip(32)
        data["ttl"] = bin2Int(stream.read(8))
        data["protocol"] = bin2Int(stream.read(8))
        data["checksum"] = bin2Int(stream.read(16))
        data["src_ip"] = bin2Int(stream.read(32))
        data["dst_ip"] = bin2Int(stream.read(32))
        # option
        header_size = data["header_len"] * 4
        stream.skip(header_size - 20)

        # tcp
        if data["protocol"] == 6:
            data["payload"] = TCPPacket.parse(stream)
        return cls(data)

raw_bytes = b'\x45\x00\x00\x28\x00\x00\x40\x00\x40\x06\xc4\xc8\xa9\xfe\x91\x01\xa9\xfe\x91\x09\x36\x56\x3c\xd3\xb2\x94\x2b\x97\x00\x00\x00\x00\x50\x04\x00\x00\xe8\x83\x00\x00'
ip_packet = IPPacket.parse(raw_bytes)
print(ip_packet.src_ip, ip_packet.dst_ip)
print(ip_packet.payload)
