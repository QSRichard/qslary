#include "qslary/qslary.h"
#include "../qslary/logger.h"
qslary::Logger::ptr g_logger = QSLARY_LOG_ROOT();

qslary::RWMutex rw_mutex;
qslary::MutexLock mutex;

int count = 0;

void fun1()
{

  for (int i = 0; i < 1000000; i++)
  {
    // qslary::MutexLockGuard lock(mutex);
    qslary::ReadLockGuard lock(rw_mutex);
    ++count;
    --count;
  }
}

void fun2()
{
  while (true)
  {
    QSLARY_LOG_INFO(g_logger) << "+++++++++++++++++++++++++++++";
  }
}

void fun3()
{
  while (true)
  {
    QSLARY_LOG_INFO(g_logger) << "------------------------------";
  }
}

int main(int argc, char **argv)
{
  std::vector<qslary::Thread::ptr> threads;

  // for(int i=0;i<5;i++){
  //     qslary::Thread::ptr pth(new qslary::Thread(fun1,"name_"+std::to_string(i)));
  //     threads.push_back(pth);
  // }

  // for (int i = 0; i < 5; i++)
  // {
  //     qslary::Thread::ptr pth(new qslary::Thread(fun1));
  //     threads.push_back(pth);
  // }

  // for (int i = 0; i < 5; i++)
  // {
  //     threads[i]->start();
  // }

  // for (int i = 0; i < 5; i++)
  // {
  //     threads[i]->join();
  // }

  YAML::Node root = YAML::LoadFile("/home/liushui/workspace/qslary/bin/config/log2.yml");
  qslary::Config::loadFromYaml(root);

  for (int i = 0; i < 2; i++)
  {
    qslary::Thread::ptr pth1(new qslary::Thread(fun2, "name_" + std::to_string(i)));
    qslary::Thread::ptr pth2(new qslary::Thread(fun3, "name_" + std::to_string(i)));
    threads.push_back(pth1);
    threads.push_back(pth2);
  }
  for (int i = 0; i < 2; i++)
  {
    threads[i]->start();
  }
  for (int i = 0; i < 4; i++)
  {
    threads[i]->join();
  }

  QSLARY_LOG_INFO(g_logger) << "thread test end";
  QSLARY_LOG_INFO(g_logger) << " count  " << count;
}