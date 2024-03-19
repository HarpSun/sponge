#ifndef SPONGE_LIBSPONGE_RETRANSIMISSION_TIMER_HH
#define SPONGE_LIBSPONGE_RETRANSIMISSION_TIMER_HH

#include <iostream>
#include <string>
#include <deque>

using namespace std;

class RetransimissionTimer {
  private:
    unsigned int retransmission_timeout;
    unsigned int uptime;

  public:
    RetransimissionTimer();
};

#endif  // SPONGE_LIBSPONGE_RETRANSIMISSION_TIMER_HH
