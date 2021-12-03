#include "qslary/qslary.h"

#include <vector>

qslary::Logger::ptr g_logger = QSLARY_LOG_ROOT();

void fun1()
{
  QSLARY_LOG_INFO(g_logger) << "name: " << qslary::CurrentThread::name()
                            << " tid " << qslary::CurrentThread::tid()
                            << " is main thread " << qslary::CurrentThread::isMainThread();
}

void fun2()
{
}

int main(int argc, char **argv)
{

  std::vector<qslary::Thread::ptr> threads;

  // for(int i=0;i<5;i++){
  //     qslary::Thread::ptr pth(new qslary::Thread(fun1,"name_"+std::to_string(i)));
  //     threads.push_back(pth);
  // }

  for (int i = 0; i < 5; i++)
  {
    qslary::Thread::ptr pth(new qslary::Thread(fun1));
    threads.push_back(pth);
  }

  for (int i = 0; i < 5; i++)
  {
    threads[i]->start();
  }

  for (int i = 0; i < 5; i++)
  {
    threads[i]->join();
  }

  QSLARY_LOG_INFO(g_logger) << " is main thread " << qslary::CurrentThread::isMainThread();
  QSLARY_LOG_INFO(g_logger) << " main thread " << qslary::CurrentThread::tid();
  QSLARY_LOG_INFO(g_logger) << " main thread " << qslary::CurrentThread::name();
  QSLARY_LOG_INFO(g_logger) << "thread test end";
  return 0;
}
