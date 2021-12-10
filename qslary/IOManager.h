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

#include "Scheduler.h"
#include "Timer.h"
#include "mutex.h"

#include <cstddef>
#include <functional>
#include <memory>
namespace qslary {
class IOManager : public Scheduler,public TimerManger{
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
    struct EventContext {
      Scheduler* scheduler = nullptr;  // 事件执行的Scheduler
      Fiber::ptr fiber;                // 事件协程
      std::function<void()> cb;        // 事件回调函数
    };

    EventContext& getEventContext(Event event);
    void resetEventContext(EventContext& new_ctx);
    void triggerEvent(Event event);
    qslary::MutexLock mutex_;
    int fd;                     // 句柄
    EventContext read;
    EventContext write;
    Event events_ = NONE;  // 已注册事件
  };

 public:
  IOManager(size_t threads = 1, bool useCaller = true,
            const std::string& name = "");
  ~IOManager();
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);
  bool cancelAll(int fd);
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

    /**
     * @brief 继承自TimerManager的虚函数
     * 
     */
    virtual void onTimerInsertAtFront() override;

  private:
    int epollfd_=0;
    int tickleFds_[2];
    std::atomic<size_t> pendingEventCount_={0};
    qslary::RWMutex mutex_;
    std::vector<FdContext*> fdcontexts_;
};
}  // namespace qslary

#endif