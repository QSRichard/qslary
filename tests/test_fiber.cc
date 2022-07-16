#include "qslary/qslary.h"

qslary::Logger::ptr g_logger = QSLARY_LOG_ROOT();

void run_in_fiber()
{

    QSLARY_LOG_INFO(g_logger) << "run in_fiber begin";
    qslary::Fiber::YieldToHold();
    QSLARY_LOG_INFO(g_logger) << "run in fiber end";
    qslary::Fiber::YieldToHold();
}

void thread_fun()
{
    {
        qslary::Fiber::GetThreadCurrentFiber();
        QSLARY_LOG_INFO(g_logger) << "main begin";
        qslary::Fiber::ptr fiber(new qslary::Fiber(run_in_fiber));
        fiber->swapIn();
        QSLARY_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        QSLARY_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    QSLARY_LOG_INFO(g_logger)
        << "thread fun end id is " << qslary::CurrentThread::tid();
}

int main()
{

    std::vector<qslary::Thread::ptr> threads;

    for (int i = 0; i < 3; i++)
    {
        threads.push_back(qslary::Thread::ptr(new qslary::Thread(thread_fun)));
    }

    for (int i = 0; i < 3; i++)
    {
        threads[i]->start();
    }

    for (int i = 0; i < 3; i++)
    {
        threads[i]->join();
    }

    std::cout << "all end" << std::endl;
    return 0;
}