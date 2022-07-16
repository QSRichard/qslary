#ifndef __QSALRY_ADDRESS_H_
#define __QSALRY_ADDRESS_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <ifaddrs.h>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace qslary
{
class Address
{
public:
    typedef std::shared_ptr<Address> ptr;

    static bool GetInterfaceAddresses(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
        int family = AF_UNSPEC);

    static bool GetInterfaceAddresses(
        std::vector<std::pair<Address::ptr, uint32_t>> &result,
        const std::string &iface, int family = AF_UNSPEC);

    virtual ~Address() {}

    int GetFamily() const;
    virtual const sockaddr *GetAddr() const = 0;
    virtual sockaddr *GetAddr() = 0;
    virtual const socklen_t GetAddrLen() const = 0;

    virtual std::ostream &Insert(std::ostream &os) const { return os; }
    std::string ToString();

    bool operator<(const Address &rhs) const;
    bool operator==(const Address &rhs) const;
    bool operator!=(const Address &rhs) const;
};

// FIXME 修改端口为16位
class IPAddress : public Address
{
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static std::vector<IPAddress::ptr>
    HostNameToAddress(const std::string &hostname, int family = AF_UNSPEC,
                      int type = 0, int protocol = 0);
    static IPAddress::ptr LookupAnyAddr(const std::string &hostname,
                                        int family = AF_UNSPEC, int type = 0,
                                        int protocol = 0);

    static IPAddress::ptr Create(const sockaddr *addr, socklen_t addrlen);
    static IPAddress::ptr Create(const char *addr, uint16_t port = 0);

    virtual IPAddress::ptr BroadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr NetworkAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr SubnetMask(uint32_t prefix_len) = 0;

    virtual uint16_t GetPort() const = 0;
    virtual void SetPort(uint16_t port) = 0;
};

class IPv4Address : public IPAddress
{
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    static IPv4Address::ptr Create(const char *arg, uint16_t port = 0);

    IPv4Address();
    explicit IPv4Address(uint32_t address, uint16_t port = 0);
    IPv4Address(const sockaddr_in &addr);
    ~IPv4Address();

    virtual const sockaddr *GetAddr() const override;
    virtual sockaddr *GetAddr() override;
    virtual const socklen_t GetAddrLen() const override;
    virtual std::ostream &Insert(std::ostream &os) const override;

    virtual IPAddress::ptr BroadcastAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr NetworkAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr SubnetMask(uint32_t prefix_len) override;

    virtual uint16_t GetPort() const override;
    virtual void SetPort(uint16_t port) override;

private:
    sockaddr_in addr_;
};
class IPv6Address : public IPAddress
{
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    static IPv6Address::ptr Create(const char *arg, uint16_t port);
    IPv6Address();
    IPv6Address(const sockaddr_in6 &addr);
    IPv6Address(const u_int8_t[16], uint16_t port = 0);
    ~IPv6Address();

    virtual const sockaddr *GetAddr() const override;
    virtual sockaddr *GetAddr() override;
    virtual const socklen_t GetAddrLen() const override;
    virtual std::ostream &Insert(std::ostream &os) const override;

    virtual IPAddress::ptr BroadcastAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr NetworkAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr SubnetMask(uint32_t prefix_len) override;
    virtual uint16_t GetPort() const override;
    virtual void SetPort(uint16_t port) override;

private:
    sockaddr_in6 addr_;
};

class UnixAddress : public Address
{
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    UnixAddress();
    UnixAddress(const std::string &path);
    ~UnixAddress();

    void setAddrlen(socklen_t len);

    virtual const sockaddr *GetAddr() const override;
    virtual sockaddr *GetAddr() override;
    virtual const socklen_t GetAddrLen() const override;
    virtual std::ostream &Insert(std::ostream &os) const override;

private:
    sockaddr_un addr_;
    socklen_t length;
};

} // namespace qslary

#endif // __QSALRY_ADDRESS_H_