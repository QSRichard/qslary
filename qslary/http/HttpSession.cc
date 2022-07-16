#include "qslary/http/HttpSession.h"
#include "qslary/http/Http.h"
#include "qslary/http/HttpParser.h"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <vector>

namespace qslary
{
namespace http
{
HttpSession::HttpSession(Socket::ptr sock, bool ower) : SockStream(sock, ower)
{}

HttpRequest::ptr HttpSession::recvRequest(HttpRequest::ptr &r)
{

    HttpRequestParser::ptr parser(new HttpRequestParser);

    int buff_size = 65532;

    std::shared_ptr<char> buffer(new char[buff_size],
                                 [](char *ptr) { delete[] ptr; });
    char *data = buffer.get();
    int offset = 0;
    do
    {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0)
        {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        std::cout << "nparse " << nparse << std::endl;
        if (parser->hasError())
        {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if (offset == (int)buff_size)
        {
            close();
            return nullptr;
        }
        if (parser->isFinished())
        {
            break;
        }
    }
    while (true);
    int64_t length = parser->getContentLength();
    parser->getData()->dump(std::cout);
    if (length > 0)
    {
        std::string body;
        body.resize(length);

        int len = 0;
        if (length >= offset)
        {
            memcpy(&body[0], data, offset);
            len = offset;
        }
        else
        {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if (length > 0)
        {
            if (readFixSize(&body[len], length) <= 0)
            {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }

    parser->getData()->init();
    r = parser->getData();
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr response)
{
    std::stringstream ss;
    ss << *response;
    std::string data = ss.str();
    writeFixSize(data.c_str(), data.size());
    return data.size();
}

} // namespace http

} // namespace qslary
