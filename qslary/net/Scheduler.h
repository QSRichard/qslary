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

#include "qslary/base/Fiber.h"
#include "qslary/base/Logger.h"
#include "qslary/base/Mutex.h"
#include "qslary/base/Thread.h"

#include <atomic>
#include <bits/c++config.h>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <vector>
namespace qslary
{

class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;

    Scheduler(std::size_t threadNum = 1, const std::string &name = "");

    virtual ~Scheduler();
    const std::string &getName() const { return name_; }

    void start();
    void stop();
    virtual void run();

    static Scheduler *GetThreadScheduler();
    static Fiber *GetSchedulerFirstFiber();
    static Fiber *GetMainFiber();

    void scheduler(std::function<void()> callback);

    template <class InputInterator>
    void scheduler(InputInterator begin, InputInterator end)
    {

        bool need_wakeup = false;
        {
            qslary::MutexLockGuard lock(mutex_);
            while (begin != end)
            {
                // TODO(qiaoshuo.qs): std::move()
                need_wakeup = schedulerNoLock(*begin) || need_wakeup;
                begin++;
            }
        }
        if (need_wakeup)
        {
            Wakeup();
        }
    }

    bool schedulerNoLock(std::function<void()> callback)
    {
        bool need_wakeup = pendingFunctors_.empty();
        Fiber::ptr ft(new Fiber(callback));
        pendingFunctors_.push_back(ft);
        return need_wakeup;
    }

protected:
    bool hasIdleThreads() { return idleThreadCount_ > 0; };
    virtual void Wakeup();
    virtual bool IsClean();
    virtual void Idle();
    void setThreadScheduler();

public:
    qslary::MutexLock mutex_;
    std::string name_;
    Fiber::ptr m_main_fiber;
    size_t threadCount_ = 0;
    std::vector<int> threadsId_;
    std::vector<qslary::Thread::ptr> threads_;
    std::list<Fiber::ptr> pendingFunctors_;
    std::atomic<size_t> activeThreadCount_ = {0};
    std::atomic<size_t> idleThreadCount_ = {0};
    bool clean_ = true;
    bool autoStop_ = false;
};

} // namespace qslary

#endif // __QSLARY_SCHEDULER_H_