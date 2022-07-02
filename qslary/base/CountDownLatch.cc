
#include "qslary/base/CountDownLatch.h"

namespace qslary
{

  CountDownLatch::CountDownLatch(int count) : m_mutex(),
                                              m_condition(m_mutex),
                                              m_count(count)
  {
  }

  void CountDownLatch::wait()
  {
    // 加锁
    MutexLockGuard lock(m_mutex);
    while (m_count > 0)
    {
      // 阻塞时在 wait内会自动解锁  wait返回时会自动加锁
      m_condition.wait();
    }
  }

  void CountDownLatch::countDown()
  {
    MutexLockGuard lock(m_mutex);
    --m_count;
    if (m_count == 0)
    {
      m_condition.notifyAll();
    }
  }

  int CountDownLatch::getCount() const
  {
    MutexLockGuard lock(m_mutex);
    return m_count;
  }

} // namespace qslary end