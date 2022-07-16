#include "qslary/net/IOManager.h"
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

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "36.152.44.96", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {}
    else if (errno == EINPROGRESS)
    {
        QSLARY_LOG_INFO(g_logger)
            << "add event errno=" << errno << " " << strerror(errno);
        qslary::IOManager::GetIOManager()->AddReadEvent(
            sock, []() { QSLARY_LOG_INFO(g_logger) << "read callback"; });
        qslary::IOManager::GetIOManager()->AddWriteEvent(sock, []() {
            QSLARY_LOG_INFO(g_logger) << "write callback";
            // close(sock);
            close(sock);
        });
    }
    else
    {
        QSLARY_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void timerFunc() { std::cout << " test iomanager timerFunc" << std::endl; }

void test1()
{
    std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
    qslary::IOManager iom(2, "Default IoManger");
    iom.scheduler(&test_fiber);
    sleep(5);
    std::cout << "end test1" << std::endl;
}

void test_timer()
{
    qslary::IOManager iom(2);
    QSLARY_LOG_DEBUG(g_logger) << "即将添加timer";
    iom.runEvery(3.0, timerFunc);
    sleep(15);
}

int main(int argc, char **argv)
{
    // test1();
    test_timer();
    QSLARY_LOG_DEBUG(g_logger) << "==========tist_timer() end=========";
    sleep(7);
    return 0;
}
