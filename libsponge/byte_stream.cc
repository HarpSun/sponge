#include <cstring>

#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// ByteStream 就是一块固定大小的内存 可以对它进行读写
// 所以初始化就是申请一块内存 大小为参数 capacity 所指定

// 这里官方设置的编译器会要求你按照特定方式来写构造函数 非常 nb
// 初始化可以直接抄下面的写法
// 经过试错 发现规则如下（感兴趣可以看，不感兴趣直接抄就好，这些知识没啥用）
// 1. 实例属性的初始化必须按照 member initlization list 的写法(就问你 nb 不 nb)
// 而不是正常人那样在构造函数里面初始化 至于这个 dududu 写法怎么写参考下面
// 2. 实例属性的初始化顺序必须和头文件声明的保持一致

// size 表示内存的大小 类型是 int
// offset 用来记录 reader 读到了哪里 类型是 int
// memory 用来存储数据 类型是 vector<char>
ByteStream::ByteStream(const size_t capacity):
    size{capacity},
    offset{0},
    memory{} {

}

size_t ByteStream::write(const string &data) {
    // 往内存里面写数据 但是因为我们内存大小有限 所以要考虑 data 超过内存大小的情况
    // 这里采用简单的方案
    // 1. 检查 data 长度
    // 2. 如果超了就把超过的部分丢掉
    // 3. 写入内存
    string writtenData = data;
    if (data.size() > this->size) {
        writtenData = data.substr(0, this->size);
    }
    for (size_t i = 0; i < writtenData.size(); i++) {
        this->memory.push_back(writtenData[i]);
    }
    return this->memory.size();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    return {};
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { DUMMY_CODE(len); }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    return {};
}

void ByteStream::end_input() {}

bool ByteStream::input_ended() const { return {}; }

size_t ByteStream::buffer_size() const { return {}; }

bool ByteStream::buffer_empty() const { return {}; }

bool ByteStream::eof() const { return false; }

size_t ByteStream::bytes_written() const {
     return this->memory.size();
}

size_t ByteStream::bytes_read() const { return {}; }

size_t ByteStream::remaining_capacity() const {
    return this->size - this->memory.size();
}
