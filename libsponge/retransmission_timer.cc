#include "retransmission_timer.hh"

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 构造函数
RetransmissionTimer::RetransmissionTimer(unsigned int initial_retransmission_timeout):
    _init_retransmission_timeout(initial_retransmission_timeout),
    retransmission_timeout(initial_retransmission_timeout),
    _uptime(0) {}


// tick 函数，用于更新 retransmission_timeout
void RetransmissionTimer::tick(size_t ms) {
    _uptime += ms;
}

