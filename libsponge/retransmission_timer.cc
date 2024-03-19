#include "retransmission_timer.hh"

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 构造函数
RetransimissionTimer::RetransimissionTimer():
    retransmission_timeout(0),
    uptime(0) {}

