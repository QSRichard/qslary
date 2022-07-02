#ifndef __QSALRY_ADDRESS_H_
#define __QSALRY_ADDRESS_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <ifaddrs.h>
#include <unistd.h>

namespace qslary
{
class Address
{
public:
  typedef std::shared_ptr<Address> ptr;

  static bool GetInterfaceAddress(std::multimap<std::string,std::pair<Address::ptr, uint32_t>>& result,int family=AF_UNSPEC);
  virtual ~Address(){}

  int getFamily() const;
  virtual const sockaddr* getAddr() const = 0;
  virtual  sockaddr* getAddr()=0;
  virtual const socklen_t getAddrLen() const = 0;

  virtual std::ostream& insert(std::ostream& os) const=0;
  std::string toString();

  bool operator<(const Address& rhs) const;
  bool operator==(const Address& rhs) const;
  bool operator!=(const Address& rhs) const;
};

// FIXME 修改端口为16位
class IPAddress : public Address
{
public:
  typedef std::shared_ptr<IPAddress> ptr;

  static std::vector<IPAddress::ptr> HostNameToAddress(const std::string& hostname, int family=AF_INET,int type=0,int protocol=0);


  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  virtual uint16_t getPort() const = 0;
  virtual void setPort(uint16_t port)=0;
};

class IPv4Address : public IPAddress
{
public:
  typedef std::shared_ptr<IPv4Address> ptr;

  static IPv4Address::ptr Create(const char* arg,uint16_t port=0);

  IPv4Address();
  explicit IPv4Address(uint32_t address , uint16_t port = 0);
  IPv4Address(const sockaddr_in& addr);
  ~IPv4Address();

  virtual const sockaddr* getAddr() const override;
  virtual sockaddr* getAddr() override;
  virtual const socklen_t getAddrLen() const override;
  virtual std::ostream& insert(std::ostream& os) const override;

  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  virtual IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;

  virtual uint16_t getPort() const override;
  virtual void setPort(uint16_t port) override;

private:
    sockaddr_in addr_;
};
class IPv6Address : public IPAddress
{
public:
  typedef std::shared_ptr<IPv6Address> ptr;

  static IPv6Address::ptr Create(const char* arg, uint16_t port);
  IPv6Address();
  IPv6Address(const sockaddr_in6& addr);
  IPv6Address(const u_int8_t[16], uint16_t port = 0);
  ~IPv6Address();


  virtual const sockaddr* getAddr() const override;
  virtual sockaddr* getAddr() override;
  virtual const socklen_t getAddrLen() const override;
  virtual std::ostream& insert(std::ostream& os) const override;

  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  virtual IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  virtual uint16_t getPort() const override;
  virtual void setPort(uint16_t port) override;

private:
  sockaddr_in6 addr_;
};

class UnixAddress : public Address
{
public:
  typedef std::shared_ptr<UnixAddress> ptr;

  UnixAddress();
  UnixAddress(const std::string& path);
  ~UnixAddress();

  void setAddrlen(socklen_t len);

  virtual const sockaddr* getAddr() const override;
  virtual sockaddr* getAddr() override;
  virtual const socklen_t getAddrLen() const override;
  virtual std::ostream& insert(std::ostream& os) const override;

private:
  sockaddr_un addr_;
  socklen_t length;
};

}


#endif // __QSALRY_ADDRESS_H_