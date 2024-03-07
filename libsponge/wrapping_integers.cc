#include "wrapping_integers.hh"

#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/*
tcp 将数据拆成一小段 一小段发送和接收
每一小段都有一个序列号 （sequence number）
这个序列号表示的是这一段数据在完整数据中的下标 （就是 reassembler 中的 index）
tcp header 用一个 32 位的整数表示这个序列号

关于这个序列号有 3 点要注意的
1. 32 位的整数能表示的数据最大只有 4G 超过 4G 之后就溢出了 从零开始
然而我们平时网络传输数据上限可不止 4G 比如你下个游戏可能要 50G
可能你会好奇 为啥不把序列号设置的长点
你要考虑到 tcp 是上古时期(1974)发明的协议。。。那会儿的电脑是啥样的
你参考一下这个 https://zh.wikipedia.org/wiki/Apple_I

2. 这个序列号出于安全考虑 防止被别人猜出来 不是从 0 开始 而是从 0 - 2^32 范围内的一个随机数开始

3. 数据的起始和结束各有一个特殊标识符 SYN FIN
这两个标识符各占一个序列号
注意这两个不算完整数据的一部分 就是一个标识

文档中举了一个例子帮我们理清这几个概念
假如我们要发送一段数据是 "cat"
首先我们要随机生成起始序列号 也就是 SYN 结果是 2 ^ 32 - 2
那么 c 就是 2 ^ 32 - 1
a 就是 0 (溢出了)
t 就是 1
FIN 就是 2
*/

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    return WrappingInt32{isn + n};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
/*
seqno 是 32 位的，我们要计算的 abs seqno 是 64 位的

这种情况怎么确定绝对序列号的值呢 所以有 checkpoint 这个参数
checkpoint 是我们最后 reassembler 的 nextReassembledIndex 的值，也就是窗口的左边
（思考：窗口的大小可以超过 2 ** 32 吗？如果超过了会怎么样）
我们算出的绝对序列号应该是最接近这个 checkpoint 的一个值
要注意绝对序列号不一定大于 checkpoint
因为数据可以重复
比如说数据是 hello 重组到了 hel 这时候 checkpoint 就是 2
这时候可能会收到 ello 那么 seq 是 1

1. 将 checkpoint 转换为 WrappingInt32
2. 计算 checkpoint 到 seqno 的距离
3. 计算两个值之间和 checkpoint 的距离 最小的为结果
 */
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    int32_t offset = n - wrap(checkpoint, isn);
    int64_t res = static_cast<int64_t>(checkpoint) + offset;
    if (res >= 0) {
        // 不直接返回 res，因为转换成 int64_t 会丢失精度
        return checkpoint + offset;
    } else {
        return (1ul << 32) + res;
    }
}

// 草 这个作业太 sb 了
// tcp header 迭代一版本 支持 64 bit 序列号不就没事了。。。
// 查了一下还真有这个提案。。。
// http://www.watersprings.org/pub/id/draft-looney-tcpm-64-bit-seqnos-00.html