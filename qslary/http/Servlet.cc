#include "qslary/http/Servlet.h"
#include <cstdint>
#include <fnmatch.h>

namespace qslary
{
namespace http
{
FunctionServlet::FunctionServlet(callback cb)
    : Servlet("FunctionServlet"), cb_(cb)
{}

int32_t FunctionServlet::handle(qslary::http::HttpRequest::ptr request,
                                qslary::http::HttpResponse::ptr response,
                                qslary::http::HttpSession::ptr session)
{
    return cb_(request, response, session);
}

ServletDispatch::ServletDispatch() : Servlet("DispatchServlet")
{
    defaultServlet_.reset(new NotFoundServlet("NotFound"));
};

void ServletDispatch::addServlet(const std::string &url, Servlet::ptr slt)
{
    mp_data_.insert({url, slt});
}
void ServletDispatch::addGlobalServlet(const std::string &url, Servlet::ptr slt)
{
    v_data_.push_back({url, slt});
}

void ServletDispatch::delServlet(const std::string &url)
{

    auto it = mp_data_.find(url);
    if (it != mp_data_.end())
        mp_data_.erase(it);
}
void ServletDispatch::delGlobalServlet(const std::string &url)
{
    for (auto it = v_data_.begin(); it != v_data_.end(); it++)
    {
        if (it->first == url)
        {
            v_data_.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::getServlet(const std::string &url)
{
    auto it = mp_data_.find(url);
    return it == mp_data_.end() ? nullptr : it->second;
}
Servlet::ptr ServletDispatch::getGlobalServlet(const std::string &url)
{
    for (auto it = v_data_.begin(); it != v_data_.end(); it++)
    {
        if (it->first == url)
            return it->second;
    }
    return nullptr;
}

Servlet::ptr ServletDispatch::getMatchServlet(const std::string &url)
{
    auto it = mp_data_.find(url);
    if (it != mp_data_.end())
        return it->second;
    for (auto pv = v_data_.begin(); pv != v_data_.end(); pv++)
    {
        if (!fnmatch(it->first.c_str(), url.c_str(), 0))
        {
            return it->second;
        }
    }
    return defaultServlet_;
}
int32_t ServletDispatch::handle(qslary::http::HttpRequest::ptr request,
                                qslary::http::HttpResponse::ptr response,
                                qslary::http::HttpSession::ptr session)
{
    auto slt = getMatchServlet(request->getPath());
    if (slt)
    {
        return slt->handle(request, response, session);
    }
    return 0;
}

NotFoundServlet::NotFoundServlet(const std::string &name)
    : Servlet("NotFoundServlet"), name_(name)
{
    content_ = "<html><head><title>404 Not Found"
               "</title></head><body><center><h1>404 Not Found</h1></center>"
               "<hr><center>" +
               name_ + "</center></body></html>";
};
int32_t NotFoundServlet::handle(qslary::http::HttpRequest::ptr request,
                                qslary::http::HttpResponse::ptr response,
                                qslary::http::HttpSession::ptr session)
{
    response->setStatus(qslary::http::HttpStatus::NOT_FOUND);
    response->setHeaderEntry("Server", "sylar/1.0.0");
    response->setHeaderEntry("Content-Type", "text/html");
    response->setBody(content_);
    return 0;
}
} // namespace http
} // namespace qslary