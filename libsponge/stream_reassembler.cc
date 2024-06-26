#include "stream_reassembler.hh"

#include <vector>
#include <sstream>

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
    unassembledMap(),
    receiveEof(false),
    nextReassembledIndex(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
/*
tcp reciver 收到数据后通过调用这个函数将数据进行重组
三个参数 data 是收到的数据
index 是 data 在完整数据中的下标
eof 是表示 data 是否是完整数据的最后一段
举个例子
我要通过网络给你的电脑发送一段数据是 'hello'
数据被拆成两个部分 he, llo
那 he 部分调用这个函数的时候 参数就是 ("he", 0, false)
llo 调用的时候就是 ("llo", 2, true)，因为 l 在 hello 中下标为 2

实现这个函数的时候要考虑如下情况
1. 收到的数据有可能包含重复的 比如 he, ello
2. 收到的数据有可能是乱序的  比如 llo, he

所以实现思路如下
1. index 和 nextReassembledIndex 对比
如果等于表示这段数据正好可以写入 output
如果小于表示这段数据有重复的 那么舍弃重复部分写入 output
如果大于表示这段数据是乱序的 先写入临时存储 unassebmledMap 供后续使用
2. 写入 data 之后 扫描 unassebmledMap 的 key 也就是等待重组的数据段下标
和 nextReassembledIndex 对比 按照 1 的逻辑写入数据
3. 检查数据是否写完 完了的话调用 end_input 标示重组完成
 */
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof)  {
    // capacity 是 receiver 的容量大小，它决定了窗口的右边界
    // 而左边界是由 nextReassembledIndex 决定的
    // unacceptedByteIndex 就是第一个超过窗口尺寸的 sequence number (tcp 中就是 index)
    // 如果数据只有一部分超过了，那么要对数据进行切割
    size_t unacceptedByteIndex = _output.bytes_read() + _capacity;
    uint64_t remainCapacity = static_cast<int>(unacceptedByteIndex) - index;
    if (not data.empty() and remainCapacity > 0) {
        const string d = data.substr(0, remainCapacity);
        reassemble_data(d, index);
    }

    check_eof(data, index, eof);
}

void StreamReassembler::check_eof(const string &data, const uint64_t index, const bool eof) {
    if (eof) {
        // 没有发送数据，SYN 之后直接 FIN
        if (index == 0 and data.empty()) {
            end_index = 0;
        } else {
            end_index = !data.empty() ? index + data.length() : index;
        }
    }

    if (end_index.has_value() and nextReassembledIndex == end_index) {
        _output.end_input();
    }
}

// 输出内部状态，调试用
void StreamReassembler::_log_internal_status() {
    cout << "buffer: " << _output.peek_output(_output.buffer_size()) << " " << nextReassembledIndex << endl;
    for (auto const& x : unassembledMap) {
        cout << "cache: " << x.first << " " << x.second << endl;
    }
}

// 这个函数只负责重组数据 返回重组成功的字节数
// 分三种情况
// 1. 当前 data 的下标正好是待重组数据的下一个坐标，直接写入 out stream buffer
// 2. 当前 data 的一部分和已重组数据有重叠
// 3. 当前 data 在待重组数据的后面，需要先加入临时存储
void StreamReassembler::reassemble_data(const string &data, const uint64_t index) {
    if (index + data.size() <= nextReassembledIndex) {
        // 数据是完全重复的
        return;
    } else if (index > nextReassembledIndex) {
        cache_data(data, index);
    } else {
        write_data(data, index);
        write_deliverable_cache();
    }
}

void StreamReassembler::write_deliverable_cache() {
    // 如果写入了数据, 那么需要检查 unassembled 数据中是否有能够继续写入的
    // indexToBeRemoved 记录循环 unassembledMap 过程中写入成功的数据段的下标
    // 之后将这些数据从 unassembledMap 中移除
    // 为什么这样做？因为循环 map 的过程中改变 map 内容会有问题
    vector<size_t> indexToBeRemoved = {};
    // 检查 unassembledMap 中的数据段是否可以继续重组
    for (auto const& x : unassembledMap) {
        size_t i = x.first;
        string d = x.second;
        if (i + d.size() <= nextReassembledIndex) {
            // 数据是完全重复的
            indexToBeRemoved.push_back(i);
        } else if (i > nextReassembledIndex) {
            break;
        } else {
            write_data(d, i);
            indexToBeRemoved.push_back(i);
        }
    }

    // 把重组成功的从 unassembledMap 中移除
    for (unsigned long & i : indexToBeRemoved) {
        unassembledMap.erase(i);
    }
}

void StreamReassembler::write_data(const string &data, const uint64_t index) {
    if (index == nextReassembledIndex) {
        size_t numOfBytesWritten = _output.write(data);
        nextReassembledIndex += numOfBytesWritten;
    } else if (index < nextReassembledIndex) {
        size_t i = nextReassembledIndex - index;
        string s = data.substr(i, data.size());
        size_t numOfBytesWritten = _output.write(s);
        nextReassembledIndex += numOfBytesWritten;
    }
}

void StreamReassembler::cache_data(const string &data, const uint64_t index) {
    if (unassembledMap.find(index) == unassembledMap.end()
        or (unassembledMap.find(index) != unassembledMap.end()
            and unassembledMap[index].length() < data.length()))
    {
        unassembledMap[index] = data;
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
    for (auto const& x : unassembledMap) {
        size_t index = x.first;
        string data = x.second;
        if (index + data.size() <= nextUnassembledIndex) {
            // 数据是完全重复的
            continue;
        } else if (index < nextUnassembledIndex) {
            string s = data.substr(index, data.size());
            res += s.size();
            nextUnassembledIndex += s.size();
        } else {
            res += data.size();
            nextUnassembledIndex = index + data.size();
        }
    }
    return res;
}

bool StreamReassembler::empty() const { 
    return unassembledMap.empty(); 
}


// 使用 shell 传参来调试程序
//int main(int argc, char* argv[]) {
//    // argc contains the number of arguments passed to the program
//    // argv is an array of C-style strings (char*) containing the arguments
//
//    size_t size = stoi(argv[1]);
//    cout << "size of reassembler: " << size << "\n";
//    StreamReassembler buf{size};
//
//    std::string input;
//    while (true) {
//        // Prompt the user for input
//        std::cout << "Enter your input (type 'exit' to quit): ";
//        std::getline(std::cin, input);
//
//        // Check if the input is 'exit'
//        if (input == "exit") {
//            std::cout << "Exiting..." << std::endl;
//            break;
//        }
//
//        // Process the input
//        std::cout << "You entered: " << input << std::endl;
//        // Create a string stream from the input string
//        std::istringstream iss(input);
//        // Vector to store the split substrings
//        std::vector<std::string> tokens;
//        // Temporary string to hold each substring
//        std::string token;
//
//        // Read each substring separated by space and store it in the vector
//        while (iss >> token) {
//            tokens.push_back(token);
//        }
//
//        const string data = tokens[0];
//        int index = stoi(tokens[1]);
//        bool eof = stoi(tokens[2]);
//        // cout << "tokens: " << data << " " << index << " " << eof << "\n";
//
//        buf.push_substring(data, index, eof);
//    }
//
//    return 0;
//}