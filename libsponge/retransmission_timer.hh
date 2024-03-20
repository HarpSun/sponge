#ifndef SPONGE_LIBSPONGE_RETRANSMISSION_TIMER_HH
#define SPONGE_LIBSPONGE_RETRANSMISSION_TIMER_HH

#include <iostream>
#include <string>
#include <deque>

using namespace std;

class RetransmissionTimer {
  private:
    unsigned int _init_retransmission_timeout;
    unsigned int retransmission_timeout;
    unsigned int _uptime;

  public:
    RetransmissionTimer(unsigned int initial_retransmission_timeout);
    void tick(size_t ms);
    unsigned int uptime() { return _uptime; }
};

#endif  // SPONGE_LIBSPONGE_RETRANSMISSION_TIMER_HH
