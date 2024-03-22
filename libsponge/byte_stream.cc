#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// ByteStream 可以看作是一个存储 byte 的双端队列
// 一边是 writer, 另一边是 reader
// writer 写数据是分多次写的，reader 读也是分多次读
// 对于 reader 来说，他关注的是是否读完所有数据了，这个通过调用 eof 方法可以知道
// 而 writer 则负责在写完数据之后调用 end_input 来告知所有数据已写完

// 构造函数
ByteStream::ByteStream(const size_t capacity):
    // 实例属性的初始化顺序必须和头文件声明的保持一致
    // size 表示内存的大小 类型是 int
    // bytesWritten，bytesRead 分别表示读了多少 byte 和 写了多少 byte
    // inputEnded 供 writer 调用 表示如果数据写完了
    // byteQueue 用来存储数据 类型是 deque<char>，是一个双端队列，自身没有容量大小限制，会动态分配内存，所以需要我们来限制
    size(capacity),
    bytesWritten(0),
    bytesRead(0),
    inputEnded(false),
    byteQueue() {
}

/*
 往内存里面写数据 但是因为我们 bytestream 大小有限，所以要考虑 data 超过内存大小的情况
 这里采用简单的方案
 1. 检查 data 长度
 2. 如果超了就把超过的部分丢掉
 3. 写入
 */
size_t ByteStream::write(const string &data) {
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

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
/*
 读取 len 长度的 byte 并返回
 */
std::string ByteStream::read(const size_t len) {
    string res = "";
    size_t i = 0;
    while (i < len and not this->byteQueue.empty()) {
        res += this->byteQueue.front();
        this->byteQueue.pop_front();
        i++;
    }
    this->bytesRead += i;
    return res;
}

//! \param[in] len bytes will be copied from the output side of the buffer
/*
 查看 len 长度的 byte 数据，但是不读取出来
 */
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
/*
 丢弃 len 长度的 byte 数据
 */
void ByteStream::pop_output(const size_t len) {
    for (size_t i = 0; i < len; i++) {
        this->byteQueue.pop_front();
    }
    this->bytesRead += len;
}

/*
 由 writer 负责调用，告知已完成写入
 */
void ByteStream::end_input() {
    this->inputEnded = true;
}

/*
 获取 inputEnded 属性的值
 */
bool ByteStream::input_ended() const {
    return this->inputEnded;
}

/*
 获取 bytestream 里面由多少 byte 的数据
 */
size_t ByteStream::buffer_size() const {
    return this->byteQueue.size();
}

/*
 bytestream 是否为空，要注意为空并不一定是已经读完了，还有可能是新数据还没写入
 */
bool ByteStream::buffer_empty() const {
    return this->byteQueue.size() == 0;
}

/*
 是否已经读完，满足 bytestream 为空，并且 inputEnded 被设置为 true
 */
bool ByteStream::eof() const {
    return this->byteQueue.size() == 0 and this->inputEnded;
}

/*
 写了多少 byte 数据
 */
size_t ByteStream::bytes_written() const {
     return this->bytesWritten;
}

/*
 读了多少 byte 数据
 */
size_t ByteStream::bytes_read() const {
    return this->bytesRead;
}

/*
 bytestream 还剩下多少空间可供写入
 */
size_t ByteStream::remaining_capacity() const {
    return this->size - this->byteQueue.size();
}
