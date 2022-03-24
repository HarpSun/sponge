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
// endInput 表示供 writer 调用 如果数据写完了
// byteQueue 用来存储数据 类型是 deque<char>
// 为什么用 deque
ByteStream::ByteStream(const size_t capacity):
    size(capacity),
    bytesWritten(0),
    bytesRead(0),
    inputEnded(false),
    byteQueue() {

}

size_t ByteStream::write(const string &data) {
    // 往内存里面写数据 但是因为我们内存大小有限 所以要考虑 data 超过内存大小的情况
    // 这里采用简单的方案
    // 1. 检查 data 长度
    // 2. 如果超了就把超过的部分丢掉
    // 3. 写入内存
    string writtenData = data;
    size_t emptySize = this->size - this->byteQueue.size();
    if (data.size() > emptySize) {
        writtenData = data.substr(0, emptySize);
    }
    // write to stream
    for (size_t i = 0; i < writtenData.size(); i++) {
        this->byteQueue.push_back(writtenData[i]);
    }
    this->bytesWritten += writtenData.size();
    return writtenData.size();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // 有一个边界情况就是如果 len 超过了数据的长度怎么处理
    // 这里先不管
    string res = "";
    for (size_t i = 0; i < len; i++) {
        res += this->byteQueue[i];
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    for (size_t i = 0; i < len; i++) {
        this->byteQueue.pop_front();
    }
    this->bytesRead += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // 和 pop_output 很相似 唯一的区别是要返回读到的数据
    string res = "";
    for (size_t i = 0; i < len; i++) {
        // c++ dequeu pop_front 不返回 pop 掉的元素。。。
        // 所以 pop 前要先获取 front
        res += this->byteQueue.front();
        this->byteQueue.pop_front();
    }
    this->bytesRead += len;
    return res;
}

void ByteStream::end_input() {
    this->inputEnded = true;
}

bool ByteStream::input_ended() const {
    return this->inputEnded;
}

size_t ByteStream::buffer_size() const {
    // buffer 就是还没有读的数据
    return this->byteQueue.size();
}

bool ByteStream::buffer_empty() const {
    return this->byteQueue.size() == 0;
}

bool ByteStream::eof() const {
    // 数据是否读完 和 buffer_empty 有啥区别？？？
    return this->byteQueue.size() == 0 and this->inputEnded;
}

size_t ByteStream::bytes_written() const {
     return this->bytesWritten;
}

size_t ByteStream::bytes_read() const {
    return this->bytesRead;
}

size_t ByteStream::remaining_capacity() const {
    return this->size - this->byteQueue.size();
}
