#ifndef __QSLARY_CONDITION_H_
#define __QSLARY_CONDITION_H_

#include <pthread.h>

#include "qslary/base/Mutex.h"

namespace qslary {

class Condition : noncopyable {
 public:
  explicit Condition(MutexLock& mutex) : m_mutex(mutex) {
    MCHECK(pthread_cond_init(&m_pcond, NULL));
  }

  ~Condition() { MCHECK(pthread_cond_destroy(&m_pcond)); }

  void wait() {
    MutexLock::UnassignGuard unguard(m_mutex);

    /*
       pthread_cond_wait 使用前要在外部加锁
       pthread_cond_wait 阻塞时会在内部解锁
       pthread_cond_wait 返回前会在内部加锁
    */
    MCHECK(pthread_cond_wait(&m_pcond, m_mutex.getPthreadMutex()));
  }

  bool waitForSeconds(double seconds);

  void notify() { MCHECK(pthread_cond_signal(&m_pcond)); }

  void notifyAll() { MCHECK(pthread_cond_broadcast(&m_pcond)); }

 private:
  MutexLock& m_mutex;
  pthread_cond_t m_pcond;
};
}  // namespace qslary

#endif  //__QSLARY_CONGITION_H_