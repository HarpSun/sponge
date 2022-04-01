#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// StreamReassembler 的作用是将接受到的数据重组
// 因为 IP 协议并非可靠的 所以收到的数据可能是乱序的
// （不可能没收到 因为这是 TCP 协议没收到会重发 不过这里先不用管。。。）
// 我们要将乱序的数据重组之后写到 _output 属性里面
// _output 就是 lab0 的 ByteStream
StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
// tcp reciver 收到数据后通过调用这个函数将数据进行重组
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 三个参数 data 是收到的数据
    // index 是 data 在完整数据中的下标
    // eof 是表示 data 是否是完整数据的最后一段
    // 举个例子
    // 我要通过网络给你的电脑发送一段数据是 'hello'
    // 数据被拆成两个部分 he, llo
    // 那 he 部分调用这个函数的时候 参数就是 ("he", 0, false)
    // llo 调用的时候就是 ("llo", 2, true)

    // 实现这个函数的时候要考虑如下情况
    // 1. 收到的数据有可能包含重复的 比如 he, ello
    // 2. 收到的数据有可能是乱序的  比如 llo, he
    DUMMY_CODE(data, index, eof);
}

size_t StreamReassembler::unassembled_bytes() const { return {}; }

bool StreamReassembler::empty() const { return {}; }
