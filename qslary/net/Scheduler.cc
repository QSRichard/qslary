#include "qslary/base/Fiber.h"
#include "qslary/base/Hook.h"
#include "qslary/base/Logger.h"
#include "qslary/base/Mutex.h"
#include "qslary/net/Scheduler.h"

#include <bits/c++config.h>
#include <cstdint>
#include <functional>
#include <string>

namespace qslary
{
// extern uint64_t s_fiber_id;

static qslary::Logger::ptr g_logger = QSLARY_LOG_NAME("system");

static thread_local Scheduler *thread_scheduler = nullptr;
static thread_local Fiber *t_scheduler_main_fiber = nullptr;

/**
 * @brief
 *
 * @return Scheduler*
 */
Scheduler *Scheduler::GetThreadScheduler() { return thread_scheduler; }

/**
 * @brief
 *
 * @return Fiber*
 */

Fiber *Scheduler::GetSchedulerFirstFiber() { return t_scheduler_main_fiber; }

void Scheduler::setThreadScheduler() { thread_scheduler = this; }

Fiber *Scheduler::GetMainFiber() { return nullptr; }

Scheduler::Scheduler(std::size_t threadNum, const std::string &name)
{
    assert(threadNum > 0);
    name_ = name;
    threadCount_ = threadNum;
    Fiber *fiber = new Fiber(std::bind(&Scheduler::run, this));
    m_main_fiber.reset(fiber);
}

Scheduler::~Scheduler()
{

    assert(clean_);
    if (GetThreadScheduler() == this)
    {
        thread_scheduler = nullptr;
    }
}

// 创建调度器线程
void Scheduler::start()
{
    qslary::MutexLockGuard lock(mutex_);
    if (!clean_)
    {
        return;
    }
    clean_ = false;
    assert(threads_.empty());
    threads_.resize(threadCount_);
    for (size_t i = 0; i < threadCount_; i++)
    {
        threads_[i].reset(
            new qslary::Thread(std::bind(&Scheduler::run, this),
                               "Scheduler::Thread " + std::to_string(i)));
        threads_[i]->start();
        threadsId_.push_back(threads_[i]->getId());
    }
}

void Scheduler::stop()
{
    QSLARY_LOG_INFO(g_logger) << "entry stop" << this;
    autoStop_ = true;
    if (threadCount_ == 0)
    {
        QSLARY_LOG_INFO(g_logger) << "stopped" << this;
        clean_ = true;
        if (IsClean())
        {
            return;
        }
    }
    clean_ = true;
    assert(GetThreadScheduler() != this);
    for (auto &i : threads_)
    {
        i->join();
        QSLARY_LOG_INFO(g_logger) << "join 出返回";
    }
}

void Scheduler::scheduler(std::function<void()> cb)
{
    bool need_wakeup = false;
    {
        qslary::MutexLockGuard lock(mutex_);
        need_wakeup = schedulerNoLock(cb);
    }
    if (need_wakeup)
    {
        Wakeup();
    }
}
void Scheduler::run()
{
    setThreadScheduler();
    // set_hook_enable(true);
    // 新线程 为其创建主协程
    t_scheduler_main_fiber = qslary::Fiber::GetThreadCurrentFiber().get();
    Fiber::ptr idleFiber(new Fiber(std::bind(&Scheduler::Idle, this)));
    Fiber::ptr doing;
    while (true)
    {
        bool wakeUp = false;
        // FIXME Fiber每次进入时都要reset
        doing.reset();
        { // TODO(qiaoshuo.qs): vscode 选中一行
            MutexLockGuard guard(mutex_);
            for (auto it = pendingFunctors_.begin();
                 it != pendingFunctors_.end(); it++)
            {
                if ((*it)->getState() == Fiber::INIT ||
                    (*it)->getState() == Fiber::HOLD ||
                    (*it)->getState() == Fiber::READY)
                {
                    doing = *it;
                    pendingFunctors_.erase(it);
                    wakeUp = true;
                    break;
                }
            }
        }
        if (wakeUp)
        {
            Wakeup();
        }
        // 拿到一个合法的Fiber
        if (doing)
        {
            QSLARY_LOG_INFO(g_logger) << "拿到一个合法Fiber";
            ++activeThreadCount_;
            doing->swapIn();
            --activeThreadCount_;
            if (doing->getState() == Fiber::READY)
                scheduler(doing->cb_);
            else if (doing->getState() != Fiber::TERM &&
                     doing->getState() != Fiber::EXCEPTION)
                doing->state_ = Fiber::HOLD;
        }
        else
        {
            if (idleFiber->getState() == Fiber::EXCEPTION ||
                idleFiber->getState() == Fiber::TERM)
            {
                break;
            }
            idleThreadCount_++;
            idleFiber->swapIn();
            if (idleFiber->getState() != Fiber::EXCEPTION &&
                idleFiber->getState() != Fiber::TERM)
            {
                idleFiber->state_ = Fiber::HOLD;
            }
            --idleThreadCount_;
        }
    }
}

void Scheduler::Wakeup() { QSLARY_LOG_INFO(g_logger) << "wakeup"; }

bool Scheduler::IsClean()
{
    // 没有忙碌的线程，没有尚未执行的fiber
    MutexLockGuard guard(mutex_);
    return autoStop_ && clean_ && pendingFunctors_.empty() &&
           activeThreadCount_ == 0;
}

void Scheduler::Idle()
{
    QSLARY_LOG_INFO(g_logger) << "idle";
    while (!IsClean())
    {
        qslary::Fiber::YieldToHold();
    }
}

} // namespace qslary