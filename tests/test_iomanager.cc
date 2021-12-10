#include "qslary/qslary.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

qslary::Logger::ptr g_logger = QSLARY_LOG_ROOT();

int sock = 0;

void test_fiber()
{
  QSLARY_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

  // sleep(3);

  // close(sock);
  // qslary::IOManager::GetThis()->cancelAll(sock);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "36.152.44.96", &addr.sin_addr.s_addr);

  if (!connect(sock, (const sockaddr*)&addr, sizeof(addr)))
  {
  }
  else if (errno == EINPROGRESS)
  {
    QSLARY_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
    qslary::IOManager::GetIOManager()->addEvent(sock, qslary::IOManager::READ,
                                          []() { QSLARY_LOG_INFO(g_logger) << "read callback"; });
    qslary::IOManager::GetIOManager()->addEvent(sock, qslary::IOManager::WRITE, []() {
      QSLARY_LOG_INFO(g_logger) << "write callback";
      // close(sock);
      qslary::IOManager::GetIOManager()->cancelEvent(sock, qslary::IOManager::READ);
      close(sock);
    });
  }
  else
  {
    QSLARY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
  }
}

void test1()
{
  std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
  qslary::IOManager iom(2, false);
  iom.scheduler(&test_fiber);
}

qslary::Timer::ptr s_timer;
void test_timer()
{
  qslary::IOManager iom(2);
  QSLARY_LOG_DEBUG(g_logger)<<"即将添加timer";
  s_timer = iom.addTimer(
      1000,
      []() {
        static int i = 0;
        QSLARY_LOG_INFO(g_logger) << "=======hello timer i=======" << i;
        if (++i == 3)
        {
          s_timer->cancel();
          // s_timer->cancel();
        }
      },
      true);
    QSLARY_LOG_DEBUG(g_logger)<<"test_timer()中addTimer()执行完成";
}

int main(int argc, char** argv)
{
  // test1();
  test_timer();
  QSLARY_LOG_DEBUG(g_logger)<<"==========tist_timer() end=========";
  return 0;
}
