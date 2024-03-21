#include "retransmission_timer.hh"

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 构造函数
RetransmissionTimer::RetransmissionTimer(unsigned int initial_retransmission_timeout):
    _init_retransmission_timeout(initial_retransmission_timeout),
    retransmission_timeout(initial_retransmission_timeout) {}



void RetransmissionTimer::tick(size_t ms) {
    _uptime += ms;
}

void RetransmissionTimer::start() {
    _running = true;
    _uptime = 0;
}

void RetransmissionTimer::stop() {
    _running = false;
}

void RetransmissionTimer::reset() {
    _uptime = 0;
    retransmission_timeout = _init_retransmission_timeout;
}

bool RetransmissionTimer::timeout() {
    return _uptime >= retransmission_timeout;
}

void RetransmissionTimer::backoff() {
    retransmission_timeout *= 2;
}

bool RetransmissionTimer::running() const {
    return _running;
}