#ifndef __QSLARY_EVENTLOOPTHREAD_H_
#define __QSLARY_EVENTLOOPTHREAD_H_

#include "qslary/base/Condition.h"
#include "qslary/base/Mutex.h"
#include "qslary/base/Thread.h"

#include <functional>
#include <memory>

namespace qslary
{

class EventLoop;

class EventLoopThread : public noncopyable
{
public:
    typedef std::shared_ptr<EventLoopThread> ptr;
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());

    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();
    MutexLock mutex_;
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    Condition cond_;
    ThreadInitCallback callback_;
};

} // namespace qslary

#endif