#include "qslary/base/Mutex.h"
#include "qslary/net/Address.h"
#include "qslary/net/Endian.h"

#include <arpa/inet.h>
#include <bits/types/cookie_io_functions_t.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ifaddrs.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace qslary {
using std::make_shared;

template <typename T> static T CreateMask(uint32_t prezeroNum = 0)
{
    return (1 << (sizeof(T) * 8 - prezeroNum)) - 1;
}

template <typename T> static uint32_t CountOneNumbers(T value)
{
    uint32_t ret = 0;
    for (; value; ret++)
        value &= (value - 1);
    return ret;
}

bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
    int family)
{
    ifaddrs *interface, *next;
    if (getifaddrs(&interface))
        assert(false);

    for (next = interface; next; next = next->ifa_next)
    {
        IPAddress::ptr addr;
        uint32_t prefix_len = 0;
        if (family != AF_UNSPEC && next->ifa_addr->sa_family != family)
            continue;
        switch (next->ifa_addr->sa_family)
        {
        case AF_INET:
        {
            addr.reset(new IPv4Address(*(sockaddr_in *)next->ifa_addr));
            uint32_t netmask =
                ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
            prefix_len = CountOneNumbers(netmask);
            break;
        }
        case AF_INET6:
        {
            addr.reset(new IPv6Address(*(sockaddr_in6 *)next->ifa_addr));
            in6_addr &netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
            for (int i = 0; i < 16; i++)
            {
                prefix_len += CountOneNumbers(netmask.s6_addr[i]);
            }
            break;
        }
        default:
            break;
        }
        if (addr)
        {
            result.insert(std::make_pair(next->ifa_name,
                                         std::make_pair(addr, prefix_len)));
        }
    }
    freeifaddrs(interface);
    return !result.empty();
}

bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t>> &result,
    const std::string &iface, int family)
{
    if (iface.empty() || iface == "*")
    {
        if (family == AF_INET || family == AF_UNSPEC)
        {
            result.push_back(
                std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if (family == AF_INET6 || family == AF_UNSPEC)
        {
            result.push_back(
                std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

    if (!GetInterfaceAddresses(results, family))
    {
        return false;
    }

    auto its = results.equal_range(iface);
    for (; its.first != its.second; ++its.first)
    {
        result.push_back(its.first->second);
    }
    return !result.empty();
}

IPAddress::ptr IPAddress::Create(const sockaddr *addr, socklen_t addrlen)
{
    IPAddress::ptr res;
    switch (addr->sa_family)
    {
    case AF_INET:
      res.reset(
          new IPv4Address(*(const sockaddr_in *)addr));
        return res;
    case AF_INET6:
        res.reset(
            new IPv6Address(*(const sockaddr_in6 *)addr));
        return res;
    default:
        return nullptr;
    }
}

int Address::GetFamily() const { return GetAddr()->sa_family; }
std::string Address::ToString()
{
    std::stringstream ss;
    Insert(ss);
    return ss.str();
}

bool Address::operator<(const Address &rhs) const
{
    socklen_t minlen = std::min(GetAddrLen(), rhs.GetAddrLen());
    int result = std::memcmp(GetAddr(), rhs.GetAddr(), minlen);
    if (result != 0)
    {
        return result < 0;
    }
    return GetAddrLen() < rhs.GetAddrLen();
}

bool Address::operator==(const Address &rhs) const
{
    return GetAddrLen() == rhs.GetAddrLen() &&
           std::memcmp(GetAddr(), rhs.GetAddr(), GetAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

std::vector<IPAddress::ptr>
IPAddress::HostNameToAddress(const std::string &hostname, int family, int type,
                             int protocol)
{
    std::cout << "-----------------hostName--------------: " << hostname
              << std::endl;

    if (hostname.empty())
        assert(false);

    addrinfo hints, *results, *next;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_protocol = protocol;
    hints.ai_socktype = 0;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char *service = NULL;

    // 检查ip6address service
    if (hostname[0] == '[')
    {
        const char *endipv6 = (const char *)memchr(hostname.c_str() + 1, ']',
                                                   hostname.size() - 1);
        if (endipv6)
        {
            // TODO check out of range
            if (*(endipv6 + 1) == ':')
            {
                service = endipv6 + 2;
            }
            node = hostname.substr(1, endipv6 - hostname.c_str() - 1);
        }
    }

    if (node.empty())
    {
        service = (const char *)memchr(hostname.c_str(), ':', hostname.size());
        if (service)
        {
            if (!memchr(service + 1, ':',
                        hostname.c_str() + hostname.size() - service - 1))
            {
                node = hostname.substr(0, service - hostname.c_str());
                ++service;
            }
        }
    }

    if (node.empty())
        node = hostname;

    std::cout << "node " << node << " services: ";
    if (service)
    {
        printf("%s\n", service);
    }
    printf("\n");
    int error = getaddrinfo(node.c_str(), service, &hints, &results);

    std::cout << "error" << error << std::endl;

    if (error)
    {
        std::cout << "ttttttttttt" << error << std::endl;
        freeaddrinfo(results);
        assert(false);
    }

    std::vector<IPAddress::ptr> ret;
    next = results;
    while (next)
    {
        IPAddress::ptr temp;
        switch (next->ai_addr->sa_family)
        {
        case AF_INET:
            temp.reset(new IPv4Address(*(const sockaddr_in *)next->ai_addr));
            next = next->ai_next;
            ret.push_back(temp);
            break;
        case AF_INET6:
            temp.reset(new IPv6Address(*(const sockaddr_in6 *)next->ai_addr));
            next = next->ai_next;
            ret.push_back(temp);
            break;
        default:

            std::cout << "jjjjjjjjjj" << std::endl;
            assert(false);
        }
    }
    freeaddrinfo(results);
    return ret;
}

IPAddress::ptr IPAddress::LookupAnyAddr(const std::string &hostname, int family,
                                        int type, int protocol)
{
    auto address = HostNameToAddress(hostname, family, type, protocol);
    return address.empty() ? nullptr : address[0];
}

IPv4Address::IPv4Address()
{
    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
}

IPv4Address::IPv4Address(const sockaddr_in &addr) { addr_ = addr; }

IPv4Address::IPv4Address(uint32_t addr, uint16_t port)
{
    std::memset(&addr_, 0, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = byteswapOnLittleEndian(port);
    addr_.sin_addr.s_addr = byteswapOnLittleEndian(addr);
}

IPv4Address::~IPv4Address() {}

IPv4Address::ptr IPv4Address::Create(const char *text, uint16_t port)
{
    IPv4Address::ptr ret(new IPv4Address);
    ret->addr_.sin_port = byteswapOnLittleEndian(port);
    int temp = inet_pton(AF_INET, text, &ret->addr_.sin_addr);
    if (temp <= 0)
        assert(false);
    return ret;
}

const sockaddr *IPv4Address::GetAddr() const { return (sockaddr *)&addr_; }

sockaddr *IPv4Address::GetAddr() { return (sockaddr *)&addr_; }

const socklen_t IPv4Address::GetAddrLen() const { return sizeof(addr_); }
std::ostream &IPv4Address::Insert(std::ostream &os) const
{

    uint32_t addr = byteswapOnLittleEndian(addr_.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "." << ((addr & 0xff));
    os << ":" << byteswapOnLittleEndian(addr_.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::BroadcastAddress(uint32_t prefix_len)
{
    if (prefix_len > 32)
    {
        return nullptr;
    }
    sockaddr_in broadcastAddr(addr_);
    broadcastAddr.sin_addr.s_addr |=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(broadcastAddr));
}

IPAddress::ptr IPv4Address::NetworkAddress(uint32_t prefix_len)
{
    if (prefix_len > 32)
        return nullptr;
    sockaddr_in networkAddr(addr_);
    networkAddr.sin_addr.s_addr &=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(networkAddr));
}

IPAddress::ptr IPv4Address::SubnetMask(uint32_t prefix_len)
{
    sockaddr_in SubnetMask;
    SubnetMask.sin_family = AF_INET;
    SubnetMask.sin_addr.s_addr =
        ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(SubnetMask));
}

uint16_t IPv4Address::GetPort() const
{
    return byteswapOnLittleEndian(addr_.sin_port);
}
void IPv4Address::SetPort(uint16_t port)
{
    addr_.sin_port = byteswapOnLittleEndian(port);
}

/*            ---------------- IPv6 --------------------             */

IPv6Address::IPv6Address()
{
    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &addr) { addr_ = addr; }

IPv6Address::IPv6Address(const uint8_t addr[16], uint16_t port)
{
    std::memset(&addr_, 0, sizeof addr_);
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = byteswapOnLittleEndian(port);
    std::memcpy(&addr_.sin6_addr.s6_addr, addr, 16);
}

IPv6Address::~IPv6Address() {}

IPv6Address::ptr IPv6Address::Create(const char *arg, uint16_t port)
{
    IPv6Address::ptr ret(new IPv6Address);
    ret->addr_.sin6_port = byteswapOnLittleEndian(port);
    int temp = inet_pton(AF_INET6, arg, &ret->addr_.sin6_addr);
    if (temp <= 0)
        assert(false);
    return ret;
}

const sockaddr *IPv6Address::GetAddr() const { return (sockaddr *)&addr_; }

sockaddr *IPv6Address::GetAddr() { return (sockaddr *)&addr_; }

const socklen_t IPv6Address::GetAddrLen() const { return sizeof(addr_); }

std::ostream &IPv6Address::Insert(std::ostream &os) const
{
    os << "{";
    uint16_t *addr = (uint16_t *)addr_.sin6_addr.s6_addr;
    bool used_zeros = false;
    for (size_t i = 0; i < 8; i++)
    {
        if (addr[i] == 0 && !used_zeros)
        {
            continue;
        }
        if (i && addr[i - 1] == 0 && !used_zeros)
        {
            os << ":";
            used_zeros = true;
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]);
    }
    if (!used_zeros && addr[7] == 0)
    {
        os << "::";
    }
    os << "]:" << byteswapOnLittleEndian(addr_.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::BroadcastAddress(uint32_t prefix_len)
{
    sockaddr_in6 baddr(addr_);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for (int i = prefix_len / 8 + 1; i < 16; ++i)
    {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::NetworkAddress(uint32_t prefix_len)
{
    sockaddr_in6 addr = addr_;
    addr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    return IPv6Address::ptr(new IPv6Address(addr));
}

IPAddress::ptr IPv6Address::SubnetMask(uint32_t prefix_len)
{
    sockaddr_in6 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr.s6_addr[prefix_len / 8] =
        ~CreateMask<uint8_t>(prefix_len % 8);
    for (size_t i = 0; i < prefix_len / 8; i++)
    {
        addr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(addr));
}

uint16_t IPv6Address::GetPort() const
{
    return byteswapOnLittleEndian(addr_.sin6_port);
}

void IPv6Address::SetPort(uint16_t port)
{
    addr_.sin6_port = byteswapOnLittleEndian(port);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress()
{
    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sun_family = AF_UNIX;
    length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string &path)
{
    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sun_family = AF_UNIX;
    length = path.size() + 1;
    if (!path.empty() && path[0] == '\0')
    {
        --length;
    }
    if (length > sizeof(addr_.sun_path))
    {
        assert(false);
    }
    memcpy(addr_.sun_path, path.c_str(), length);
    length += offsetof(sockaddr_un, sun_path);
}

UnixAddress::~UnixAddress() {}

void UnixAddress::setAddrlen(socklen_t len) { length = len; }

const sockaddr *UnixAddress::GetAddr() const { return (sockaddr *)&addr_; }

sockaddr *UnixAddress::GetAddr() { return (sockaddr *)&addr_; }

const socklen_t UnixAddress::GetAddrLen() const { return length; }

std::ostream &UnixAddress::Insert(std::ostream &os) const
{
    if (length > offsetof(sockaddr_un, sun_path) && addr_.sun_path[0] == '\0')
    {
        return os << "\\0"
                  << std::string(addr_.sun_path + 1,
                                 length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << addr_.sun_path;
}

} // namespace qslary