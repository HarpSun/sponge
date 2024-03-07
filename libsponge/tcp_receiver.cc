#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/*
窗口的概念
窗口是一个比喻 表示一个范围 这个范围是接收端可以接收的数据大小
范围左边是 reassembler 下一个要重组的字节下标 （这个下标也叫 ackno)
右边是 reassembler 容量可以存储的数据的上限
窗口是为了解决发送端到接收端数据传输效率最大化的问题
因为发送端的数据输出能力和接收端不一定匹配，如果发送端发的数据量超过了接收端能重组的数据量
就会造成很多数据丢失，重发，从而浪费带宽
*/
void TCPReceiver::segment_received(const TCPSegment &seg) {
    // TCP Connection 收到数据之后会调用这个函数
    // 调用 reassembler 的 push_string 方法重组数据
    // 有两点要注意：
    // 1. SYN 的值需要我们记录 序列号转换的时候会用到
    // 2. 参数是一个 TCPSegment 对象 我们收到的原始数据是二进制的 tcp 包
    // 原本应该是要解析一下的 但是官方帮我们解析好了并封装成了 TCPSegment
    // 有两个属性 header 是 tcp 的头部
    // SYN FIN flag、序列号这些数据可以从 header 获取
    // payload 就是需要重组的数据
    DUMMY_CODE(seg);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // 返回 reassembler 下一个要重组的字节 结果是一个 tcp 序列号
    // 序列号是从 SYN 开始的
    // 所以没有接收到 SYN 之前 这个函数返回空
    return {};
}

size_t TCPReceiver::window_size() const {
    return {};
}
