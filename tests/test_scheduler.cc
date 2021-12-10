#include "qslary/qslary.h"


static qslary::Logger::ptr g_logger=QSLARY_LOG_ROOT();


void test_fiber(){
  static int s_count = 5;
  QSLARY_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

  sleep(1);
  if (--s_count >= 0) {
    qslary::Scheduler::GetThreadScheduler()->scheduler(&test_fiber,qslary::CurrentThread::tid());
  }
}

int main(int argc,char* argv[]){
  QSLARY_LOG_INFO(g_logger) << "main";
  qslary::Scheduler sc(3, false, "test");
  sc.start();
  sleep(2);
  QSLARY_LOG_INFO(g_logger) << "schedule";
  sc.scheduler(&test_fiber);
  sc.stop();
  QSLARY_LOG_INFO(g_logger) << "over";
  return 0;
}