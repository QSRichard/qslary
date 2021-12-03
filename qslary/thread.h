#ifndef __QSLARY_THREAD_h
#define __QSLARY_THREAD_h

#include "Atomic.h"
#include "CountDownLatch.h"

#include <functional>
#include <memory>
#include <thread>
#include <string>
#include <pthread.h>
#include <semaphore.h>
namespace qslary
{

  class Semaphore : noncopyable
  {
  public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();
    void wait();
    void notify();

  private:
    sem_t semaphore_;
  };

  class Thread : noncopyable
  {
  public:
    typedef std::shared_ptr<Thread> ptr;
    typedef std::function<void()> ThreadFunc;

    Thread(ThreadFunc func, const std::string &name = string());
    ~Thread();

    void start();
    int join();

    bool started() const { return m_started; };
    pid_t getId() const { return m_tid; };

    // static const std::string &getThreadName();

  private:
    void setDefaultName();

    pthread_t m_pthreadId;
    pid_t m_tid;
    ThreadFunc m_func;
    CountDownLatch m_latch;
    std::string m_name;

    bool m_started;
    bool m_joined;

    static AtomicInt64 threadNum; // 创建的线程数量

  };
}

#endif