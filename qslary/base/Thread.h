#ifndef __QSLARY_THREAD_h
#define __QSLARY_THREAD_h

#include "qslary/base/Atomic.h"
#include "qslary/base/CountDownLatch.h"

#include <functional>
#include <memory>
#include <thread>
#include <string>
#include <pthread.h>
#include <semaphore.h>
namespace qslary
{


  class Thread : noncopyable
  {
  public:
    typedef std::shared_ptr<Thread> ptr;
    typedef std::function<void()> ThreadFunc;

    Thread(ThreadFunc func, const std::string &name = string());
    ~Thread();

    void start();
    int join();

    bool started() const { return started_; };
    pid_t getId() const { return tid_; };

    // static const std::string &getThreadName();

  private:
    void setDefaultName();

    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    CountDownLatch latch_;
    std::string name_;

    bool started_;
    bool joined_;

    static AtomicInt64 threadNum; // 创建的线程数量

  };
}

#endif