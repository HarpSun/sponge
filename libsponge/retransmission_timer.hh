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
    unsigned int _uptime{0};
    bool _running{false};

  public:
    RetransmissionTimer(unsigned int initial_retransmission_timeout);

    void tick(size_t ms);
    void start();
    void stop();
    void reset();
    bool timeout();
    void backoff();
    bool running() const;
};

#endif  // SPONGE_LIBSPONGE_RETRANSMISSION_TIMER_HH
