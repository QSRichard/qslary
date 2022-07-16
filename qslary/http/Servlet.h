#ifndef __QSLARY_HTTP_SERVLET_H_
#define __QSLARY_HTTP_SERVLET_H_

#include "qslary/http/HttpSession.h"
#include "qslary/http/Http.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace qslary
{
namespace http
{
class Servlet
{
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string &name) : name_(name){};
    virtual ~Servlet(){};

    virtual int32_t handle(qslary::http::HttpRequest::ptr request,
                           qslary::http::HttpResponse::ptr response,
                           qslary::http::HttpSession::ptr session) = 0;

protected:
    std::string name_;
};

class FunctionServlet : public Servlet
{

public:
    typedef std::shared_ptr<FunctionServlet> ptr;

    typedef std::function<int32_t(qslary::http::HttpRequest::ptr,
                                  qslary::http::HttpResponse::ptr,
                                  qslary::http::HttpSession::ptr)>
        callback;

    FunctionServlet(callback cb);
    virtual int32_t handle(qslary::http::HttpRequest::ptr request,
                           qslary::http::HttpResponse::ptr response,
                           qslary::http::HttpSession::ptr session) override;

private:
    callback cb_;
};

class ServletDispatch : public Servlet
{
public:
    typedef std::shared_ptr<ServletDispatch> ptr;

    ServletDispatch();

    virtual int32_t handle(qslary::http::HttpRequest::ptr request,
                           qslary::http::HttpResponse::ptr response,
                           qslary::http::HttpSession::ptr session) override;

    void setDefaultServlet(Servlet::ptr slt) { defaultServlet_ = slt; }
    Servlet::ptr getDefaultServlet() { return defaultServlet_; }

    void addServlet(const std::string &url, Servlet::ptr slt);
    void addGlobalServlet(const std::string &url, Servlet::ptr slt);

    void delServlet(const std::string &url);
    void delGlobalServlet(const std::string &url);

    Servlet::ptr getServlet(const std::string &url);
    Servlet::ptr getGlobalServlet(const std::string &url);

    Servlet::ptr getMatchServlet(const std::string &url);

private:
    std::unordered_map<std::string, Servlet::ptr> mp_data_;
    std::vector<std::pair<std::string, Servlet::ptr>> v_data_;
    Servlet::ptr defaultServlet_;
};

class NotFoundServlet : public Servlet
{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet(const std::string &name);

    virtual int32_t handle(qslary::http::HttpRequest::ptr request,
                           qslary::http::HttpResponse::ptr response,
                           qslary::http::HttpSession::ptr session) override;

private:
    std::string name_;
    std::string content_;
};

} // namespace http
} // namespace qslary

#endif