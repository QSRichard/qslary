#ifndef __QSLARY_TIMERID_H_
#define __QSLARY_TIMERID_H_

#include "qslary/base/Atomic.h"
#include "qslary/base/copyable.h"
#include <cstdint>

namespace qslary
{
class Timer;

class TimerId : public copyable
{
public:
    TimerId() : timer_(nullptr), sequence_(0){};
    TimerId(Timer *timer, int64_t sequence) : timer_(timer), sequence_(sequence)
    {}
    friend class TimerManger;

private:
    Timer *timer_;
    int64_t sequence_;
};

} // namespace qslary

#endif
