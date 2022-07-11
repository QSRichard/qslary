#ifndef __QSLARY_URI_H_
#define __QSLARY_URI_H_

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include "qslary/net/Address.h"

namespace qslary
{
class URI
{
public:
  typedef std::shared_ptr<URI> ptr;

  static URI::ptr Create(const std::string& uri);
  URI();

  const std::string& getScheme() const {return scheme_; }
  const std::string& getUserinfo() const {return userinfo_;}
  const std::string& getHost() const {return host_;}
  const std::string& getPath() const;
  const std::string& getQuery() const {return query_;}
  const std::string& getFragment() const { return fragment_; }
  int32_t GetPort() const;

  void setScheme(const std::string& scheme)  {  scheme_=scheme; }
  void setUserinfo(const std::string& userinfo)  { userinfo_=userinfo; }
  void setHost(const std::string& host)  { host_=host; }
  void setPath(const std::string& path)  { path_=path; }
  void setQuery(const std::string& query)  { query_=query; }
  void setFragment(const std::string& fragment) { fragment_ = fragment; }
  void SetPort(int32_t port) { port_ = port; }

  std::ostream& dump(std::ostream& os) const;
  std::string toString() const;

  IPAddress::ptr createAddress() const;

  bool isDefaultPort() const;
  

private:
  std::string scheme_;
  std::string userinfo_;
  std::string host_;
  std::string path_;
  std::string query_;
  std::string fragment_;
  int32_t port_;
};

} // namespace qslary
#endif