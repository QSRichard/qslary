/**
 * @file IOManager.h
 * @author qiao (1325726511@qq.com)
 * @brief 
 * @version 0.1
 * @date 2021-12-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __QSLARY_IOMANAGER_H_
#define __QSLARY_IOMANAGER_H_

#include "qslary/base/Fiber.h"
#include "qslary/base/Mutex.h"
#include "qslary/net/Scheduler.h"
#include "qslary/net/FdManager.h"
#include "qslary/net/Timer.h"

#include <cstddef>
#include <functional>
#include <memory>
namespace qslary {
class IOManager : public Scheduler{
 public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWMutexType;

  enum Event {

    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x4,
  };

 private:
  struct FdContext {

    typedef std::function<void()> ReadCallbackFun;
    typedef std::function<void()> WriteCallbackFun;

    FdContext() : scheduler(nullptr), mutex_(), fd(0), readCallback_(nullptr), writeCallback_(nullptr), events_(NONE) {}


    Scheduler* getSchedule() { return scheduler; }
    ReadCallbackFun getReadCallback() { return readCallback_; }
    WriteCallbackFun getWriteCallback() { return writeCallback_; }


    void setReadCallback(ReadCallbackFun newfun) { readCallback_ = newfun; }
    void setWriteCallback(WriteCallbackFun newfun) { writeCallback_ = newfun; }

    void triggerEvent(Event event);

    Scheduler* scheduler;
    qslary::MutexLock mutex_;
    int fd = 0; // 句柄
    std::function<void()> readCallback_;
    std::function<void()> writeCallback_;
    Event events_ = NONE;  // 已注册事件
  };

 public:
  IOManager(size_t threads = 1,
            const std::string& name = "");
  ~IOManager();


  // 操作fd相关联的事件
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);
  bool cancelAll(int fd);

  // 操作定时器
  TimerId runAt(Timestamp time, std::function<void()> cb);
  TimerId runAfter(double delay, std::function<void()> cb);
  TimerId runEvery(double interval, std::function<void()> cb);
  void cancel(TimerId timerId);


  
  bool stopping(uint64_t& timeout);
  static IOManager* GetIOManager();

  protected:

    void contextResize(size_t size);

    /**
     * @brief 继承自Scheduler的虚函数
     *
     */
    virtual void tickle() override;
    virtual bool stopping() override;
    virtual void idle() override;

    virtual void run() override;



  private:
    int epollfd_ = 0;
    int tickleFds_[2];
    std::atomic<size_t> pendingEventCount_={0};
    qslary::MutexLock mutex_;
    std::vector<FdContext*> fdcontexts_;

    TimerManger::ptr timermanger_;
};
}  // namespace qslary

#endif