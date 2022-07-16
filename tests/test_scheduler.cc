#include "qslary/net/Scheduler.h"

static qslary::Logger::ptr g_logger = QSLARY_LOG_ROOT();
static int s_count = 5;
void test_fiber()
{
    QSLARY_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;
    sleep(1);

    if (--s_count >= 0)
    {
        qslary::Scheduler::GetThreadScheduler()->scheduler(&test_fiber);
    }
}

int main(int argc, char *argv[])
{
    QSLARY_LOG_INFO(g_logger) << "main";
    qslary::Scheduler sc(3, "test");
    sc.start();
    sleep(2);
    QSLARY_LOG_INFO(g_logger) << "scheduler construct end";
    sc.scheduler(&test_fiber);
    sleep(10);
    std::cout << "Stop-------------" << std::endl;
    sc.stop();
    sleep(2);
    QSLARY_LOG_INFO(g_logger) << "over";
    return 0;
}