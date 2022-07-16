#include "qslary/net/Address.h"
#include "qslary/net/IOManager.h"
#include "qslary/net/TcpServer.h"

void test()
{
    auto addr = qslary::IPAddress::Create("127.0.0.1", 8099);
    qslary::TcpServer::ptr server(new qslary::TcpServer());

    server->bind(addr);

    server->start();
}

int main(int argc, char *argv[])
{
    qslary::IOManager iom(2);
    iom.scheduler(test);
    return 0;
}