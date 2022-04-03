#include "stream_reassembler.hh"

#include <vector>

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
StreamReassembler::StreamReassembler(const size_t capacity) :
    _output(capacity),
    _capacity(capacity),
    receiveEof(false),
    nextReassembledIndex(0),
    unassembledMap() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
// tcp reciver 收到数据后通过调用这个函数将数据进行重组
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof)  {
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

    // 所以实现思路如下
    // 1. index 和 nextReassembledIndex 对比
    // 如果等于表示这段数据正好可以写入 output
    // 如果小于表示这段数据有重复的 那么舍弃重复部分写入 output
    // 如果大于表示这段数据是乱序的 先写入临时存储 unassebmledMap 供后续使用
    // 2. 写入 data 之后 扫描 unassebmledMap 的 key 也就是等待重组的数据段下标
    // 和 nextReassembledIndex 对比 按照 1 的逻辑写入数据
    // 3. 检查数据是否写完 完了的话调用 end_input 标示重组完成
    if (not data.empty()) {
        this->_push_substring(data, index);

        // indexToBeRemoved 记录循环 unassembledMap 过程中写入成功的数据段的下标
        // 之后将这些数据从 unassembledMap 中移除
        // 为什么这样做？因为循环 map 的过程中改变 map 内容会有问题
        vector<size_t> indexToBeRemoved = {};
        // 检查 unassembledMap 中的数据段是否可以继续重组
        for (auto const& x : this->unassembledMap) {
            size_t i = x.first;
            string d = x.second;
            size_t numOfBytesWritten = this->_push_substring(d, i);
            // 写入数据和 data 长度一致表示 data 所有内容都成功写入了
            if (numOfBytesWritten == d.size()) {
                indexToBeRemoved.push_back(i);
            }
        }

        // 把重组成功的从 unassembledMap 中移除
        for (size_t i = 0; i < indexToBeRemoved.size(); i++) {
            this->unassembledMap.erase(indexToBeRemoved[i]);
        }
    }

    this->_check_eof(eof);
}

// 这个函数只负责重组数据 返回重组成功的字节数
size_t StreamReassembler::_push_substring(const string &data, const size_t index) {
    if (index == nextReassembledIndex) {
        size_t numOfBytesWritten = this->_output.write(data);
        this->nextReassembledIndex += numOfBytesWritten;
        return numOfBytesWritten;
    } else if (index < this->nextReassembledIndex) {
        // i 表示 data 不重复的部分的开始坐标
        // 如果 i >= data 的长度
        // 表示整个 data 都是重复的 那么这次收到的数据就没有什么用 直接结束重组过程
        size_t i = this->nextReassembledIndex - index;
        if (i >= data.size()) {
            return data.size();
        } else {
            string s = data.substr(i, data.size());
            size_t numOfBytesWritten = this->_output.write(s);
            this->nextReassembledIndex += numOfBytesWritten;
            return numOfBytesWritten + i;
        }
    } else {
        this->unassembledMap[index] = data;
        return 0;
    }
}

void StreamReassembler::_check_eof(bool eof) {
    // 所有数据重组完之后 需要调用 end_input
    // eof 为 true 表示我们收到的数据是完整数据的最后一段
    // 由于数据接受的顺序是无法保证的 所以即使 eof 为 true 也不能说我们的数据都重组完了
    // 所以在满足两个条件后才可以调用 end_input
    // 1. 收到了完整数据的最后一段
    // 2. 临时存储为空
    if (eof) {
        this->receiveEof = true;
    }

    if (this->receiveEof and this->unassembledMap.empty()) {
        this->_output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    // 将 unassembledMap 中的字节拿出来数一遍
    // 要注意考虑如下问题：
    // unassembledMap 中有两段数据 1: 'bcd', 2: 'c'
    // c 是重复的 所以计算出来的未重组字节数是 3 不是 4

    // 这里用 nextUnassembledIndex 记录下一个等待重组的下标
    // 比如计算过 1: 'bcd' 之后 nextUnassembledIndex 的值就是 4
    // 比较 nextUnassembledIndex 和 index
    // 如果大于表示这段数据有一部分已经计算过了 删去这段重复的部分 加上剩下的
    // 如果小于或等于表示这段数据还没有计算过 直接加上
    size_t res = 0;
    size_t nextUnassembledIndex = 0;
    for (auto const& x : this->unassembledMap) {
        size_t index = x.first;
        string data = x.second;
        if (nextUnassembledIndex > index) {
            size_t i = nextUnassembledIndex - index;
            if (i < data.size()) {
                string s = data.substr(i, data.size());
                res += s.size();
                nextUnassembledIndex += s.size();
            }
        } else {
            res += data.size();
            nextUnassembledIndex = index + data.size();
        }
    }
    return res;
}

bool StreamReassembler::empty() const { return {}; }