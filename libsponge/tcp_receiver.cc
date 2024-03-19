#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/*
TCP Connection 收到数据之后会调用这个函数
调用 reassembler 的 push_string 方法重组数据
有两点要注意：
1. SYN 的值需要我们记录 序列号转换的时候会用到
2. 参数是一个 TCPSegment 对象 我们收到的原始数据是二进制的 tcp 包
原本应该是要解析一下的 但是官方帮我们解析好了并封装成了 TCPSegment
有两个属性 header 是 tcp 的头部
SYN FIN flag、序列号这些数据可以从 header 获取
payload 就是需要重组的数据

关于窗口
窗口是一个比喻 表示一个范围 这个范围是接收端可以接收的数据大小
具体来说：receiver 的 capacity 它决定了窗口的右边界
nextReassembledIndex 决定了窗口的左边界
窗口的变化可以分两种情况：
 1. 当 receiver 重组数据之后更新 ACK 的值时，窗口左边界会向右移动
 2. 当 receiver buffer 存储的数据被上层应用读取之后，释放了空间，窗口右边界会向右移动
 */
void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (listen()) {
        handle_listen(seg);
    }

    if (syn_recv()) {
        handle_syn_recv(seg);
    }

    if (fin_recv()) {
        // ?
    }
}

/*
 receiver 的状态
 */
bool TCPReceiver::listen() const { return _isn == nullopt; }
bool TCPReceiver::syn_recv() const { return _isn != nullopt and not stream_out().input_ended(); }
bool TCPReceiver::fin_recv() const { return stream_out().input_ended(); }


void TCPReceiver::handle_listen(const TCPSegment &seg) {
    // seg.print_tcp_segment();
    TCPHeader header = seg.header();
    if (header.syn) {
        _isn = header.seqno;
    }
}

void TCPReceiver::handle_syn_recv(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    const Buffer& payload = seg.payload();

    // syn 不是 stream 的一部分，如果一个 segment 中既包含 syn 和 stream data，那么需要先将 seqno + 1
    WrappingInt32 seqno = header.syn? header.seqno + 1 : header.seqno;
    uint64_t checkpoint = _reassembler.nextReassembledIndex;
    uint64_t absolute_seqno = unwrap(seqno, _isn.value(), checkpoint);
    uint64_t stream_index = absolute_seqno - 1;
    // cout << "stream_index: " << stream_index << endl;
    _reassembler.push_substring(payload.copy(), stream_index, header.fin);
}


/*
返回 reassembler 下一个要重组的字节 结果是一个 tcp 序列号
序列号是从 SYN 开始的
所以没有接收到 SYN 之前 这个函数返回空
 */
optional<WrappingInt32> TCPReceiver::ackno() const {
    if (listen()) {
        return nullopt;
    } else if (syn_recv()) {
        uint64_t dataIndex = _reassembler.nextReassembledIndex;
        return wrap(dataIndex + 1, _isn.value());
    } else {
        // FIN_RECV
        uint64_t dataIndex = _reassembler.nextReassembledIndex;
        return wrap(dataIndex + 2, _isn.value());
    }
}

size_t TCPReceiver::window_size() const {
    // 窗口
    return _capacity - stream_out().buffer_size();
}
