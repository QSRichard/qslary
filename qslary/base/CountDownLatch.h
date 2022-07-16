#ifndef __QSLARY_COUNTDOWNLATCH_H_
#define __QSLARY_COUNTDOWNLATCH_H_

#include "qslary/base/Condition.h"
#include "qslary/base/Mutex.h"

namespace qslary
{
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int count);

    void wait();

    void countDown();

    int getCount() const;

private:
    mutable MutexLock m_mutex;
    Condition m_condition;
    int m_count;
};
} // namespace qslary

#endif // __QSLARY_COUNTDOWNLATCH_H_