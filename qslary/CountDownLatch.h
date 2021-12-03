#ifndef __QSLARY_COUNTDOWNLATCH_H_
#define __QSLARY_COUNTDOWNLATCH_H_

#include "Condition.h"
#include "mutex.h"

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
} // namespace qslary end

#endif // __QSLARY_COUNTDOWNLATCH_H_