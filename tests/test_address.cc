#include "../qslary/net/Address.h"
#include <iostream>
#include <map>
#include <ostream>
#include <utility>
#include <vector>
using namespace std;
using namespace qslary;

void test()
{
    std::vector<IPAddress::ptr> addrs;

    addrs = IPAddress::HostNameToAddress("www.baidu.com");
    for (int i = 0; i < addrs.size(); i++)
    {
        std::cout << addrs[i]->ToString() << std::endl;
    }
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> result;
    Address::GetInterfaceAddresses(result);
    for (auto &it : result)
    {
        std::cout << it.first << "-" << it.second.first->ToString() << " "
                  << it.second.second << std::endl;
    }

    auto add = IPv4Address::Create("127.0.0.1");
    auto add2 = IPv4Address::Create("www.baidu.com");
    std::cout << add->ToString() << std::endl;
    std::cout << add2->ToString() << std::endl;
}

int main(int argc, char *argv[])
{
    test();
    return 0;
}