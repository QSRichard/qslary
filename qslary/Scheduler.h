/**
 * @file Scheduler.h
 * @author qiao (1325726511@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __QSLARY_SCHEDULER_H_
#define __QSLARY_SCHEDULER_H_

#include <bits/c++config.h>

#include <atomic>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <vector>
#include "logger.h"
#include "Fiber.h"
#include "mutex.h"
#include "thread.h"
namespace qslary {

class Scheduler {
 public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef qslary::MutexLock MutexType;

  Scheduler(std::size_t threadNum = 1, bool useCaller = true,
            const std::string& name = "");

  virtual ~Scheduler();
  const std::string& getName() const {return name_;}

  void start();
  void stop();
  void run();

  static Scheduler* GetThreadScheduler();
  static Fiber* GetMainFiber();

  template <class FiberOrCallback>
  void scheduler(FiberOrCallback fc, int thread = -1) {
    std::cout << "进入了scheduler(fc,threadId) name " << CurrentThread::name() << " tid" << CurrentThread::tid()<<std::endl;
    bool need_tickle = false;
    {
      qslary::MutexLockGuard lock(mutex_);
      need_tickle = schedulerNoLock(fc, thread);
    }
    if (need_tickle) {
      tickle();
    }
  }

  template <class InputInterator>
  void scheduler(InputInterator begin, InputInterator end, int thread = -1) {

    // QSLARY_LOG_DEBUG(g_logger)<<"进入了scheduler(begin,end)";
    std::cout<<"进入了scheduler(begin,end,threadId) name "<<CurrentThread::name()<<" tid"<<CurrentThread::tid()<<std::endl;
    bool need_tickle = false;
    {
      qslary::MutexLockGuard lock(mutex_);
      while (begin != end) {
        need_tickle = schedulerNoLock(&*begin, thread) || need_tickle;
        begin++;
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

  template <class FiberOrCallback>
  bool schedulerNoLock(FiberOrCallback fc, int thread) {
    std::cout << "进入了schedulerNoLock(fc,threadId) name " << CurrentThread::name() << " tid" << CurrentThread::tid()<<std::endl;
    bool need_tickle = fibers_.empty();
    FiberAndThread ft(fc, thread);
    if (ft.fiber != nullptr || ft.callback != nullptr) {
      fibers_.push_back(ft);
    }
    return need_tickle;
  }

 protected:
  bool hasIdleThreads(){return idleThreadCount_>0;};
  virtual void tickle();
  virtual bool stopping();
  virtual void idle();
  void setThreadScheduler();

 private:
  struct FiberAndThread {
    std::shared_ptr<Fiber> fiber;
    std::function<void()> callback;
    int threadID;

    FiberAndThread(std::shared_ptr<Fiber> f, int id) : fiber(f), threadID(id) {}

    FiberAndThread(std::shared_ptr<Fiber>* f, int id) : threadID(id) {
      fiber.swap(*f);
    }

    FiberAndThread(std::function<void()> cb, int id)
        : callback(cb), threadID(id) {}

    FiberAndThread(std::function<void()>* cb, int id) : threadID(id) {
      callback.swap(*cb);
    }

    // for STL
    FiberAndThread():threadID(-1) {}

    void reset(){
      fiber=nullptr;
      callback=nullptr;
      threadID=-1;
    }
  };

 private:
  MutexType mutex_;
  std::vector<qslary::Thread::ptr> threads_;
  std::list<FiberAndThread> fibers_;
  std::string name_;
  Fiber::ptr rootFiber_;
  bool useCaller_;

protected:
  std::vector<int> threadsId_;
  size_t threadCount_=0;
  std::atomic<size_t> activeThreadCount_={0};
  std::atomic<size_t> idleThreadCount_={0};
  bool stopping_=true;
  bool autoStop_=false;
  int rootThreadId=0;
};

}  // namespace qslary

#endif  // __QSLARY_SCHEDULER_H_