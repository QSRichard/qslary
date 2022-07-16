#include "qslary/http/HttpParser.h"
#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>

using namespace qslary::http;

bool HttpRequestParser::IsVaild(size_t mSize) const
{
    return mDecodeState != HttpRequestDecodeState::INVALID &&
           mDecodeState != HttpRequestDecodeState::INVALID_METHOD &&
           mDecodeState != HttpRequestDecodeState::INVALID_URI &&
           mDecodeState != HttpRequestDecodeState::INVALID_VERSION &&
           mDecodeState != HttpRequestDecodeState::INVALID_HEADER &&
           mDecodeState != HttpRequestDecodeState::COMPLEIE &&
           mNextPosition < mSize;
}
HttpRequest::ptr HttpRequestParser::TryDecode(const char *mData, size_t mSize)
{
    HttpRequest::ptr request(new HttpRequest());
    mDecodeState = HttpRequestDecodeState::START;
    int bodyLength = 0;
    std::string paramKey, paramVal;
    std::string headerKey, headerVal;
    char *ptr = const_cast<char *>(mData);
    char *strbegin = ptr;
    while (IsVaild(mSize))
    {
        char ch = *ptr;
        mNextPosition++;
        switch (mDecodeState)
        {
        case HttpRequestDecodeState::START:
            if (ch == CR || ch == LF || std::isblank(ch))
            {
                break;
            }
            if (std::isupper(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::METHOD;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID;
            }
            break;
        case HttpRequestDecodeState::METHOD:
            if (std::isupper(ch))
            {
                break;
            }
            // method 解析结束
            if (std::isblank(ch))
            {
                std::string method(strbegin, ptr - strbegin);
                request->setMethod(StringToHttpMethod(method));
                mDecodeState = HttpRequestDecodeState::BEFORE_URI;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID_METHOD;
            }
            break;
        case HttpRequestDecodeState::BEFORE_URI:
            if (isblank(ch))
            {
                break;
            }
            if (ch == '/')
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::IN_URI;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID_URI;
            }
            break;
        case HttpRequestDecodeState::IN_URI:
            if (std::isblank(ch))
            {
                string URL(strbegin, ptr - strbegin);
                request->SetURL(URL);
                mDecodeState = HttpRequestDecodeState::BEFORE_PROTOCAL;
                break;
            }
            if (ch == '?')
            {
                string URL(strbegin, ptr - strbegin);
                request->SetURL(URL);
                mDecodeState = HttpRequestDecodeState::BEFORE_URI_PARAM_KEY;
            }
            break;
        case HttpRequestDecodeState::BEFORE_URI_PARAM_KEY:
            if (std::isblank(ch) || ch == CR || ch == LF)
            {
                mDecodeState = HttpRequestDecodeState::INVALID_URI;
                break;
            }
            else
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::URI_PARAM_KEY;
            }
            break;
        case HttpRequestDecodeState::URI_PARAM_KEY:
            if (std::isblank(ch))
            {
                mDecodeState = HttpRequestDecodeState::INVALID_URI;
                break;
            }
            if (ch == '=')
            {
                paramKey = string(strbegin, ptr - strbegin);
                mDecodeState = HttpRequestDecodeState::BEFORE_URI_PARAM_VALUE;
                break;
            }
            break;
        case HttpRequestDecodeState::BEFORE_URI_PARAM_VALUE:
            if (isblank(ch) || ch == LF || ch == CR)
            {
                mDecodeState = HttpRequestDecodeState::INVALID_URI;
            }
            else
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::URI_PARAM_VALUE;
            }
            break;
        case HttpRequestDecodeState::URI_PARAM_VALUE:
            if (ch == '&')
            {

                paramVal = string(strbegin, ptr - strbegin);
                request->setParmaEntry(paramKey, paramVal);
                mDecodeState = HttpRequestDecodeState::BEFORE_URI_PARAM_KEY;
            }
            else if (isblank(ch))
            {
                paramVal = string(strbegin, ptr - strbegin);
                request->setParmaEntry(paramKey, paramVal);
                mDecodeState = HttpRequestDecodeState::BEFORE_PROTOCAL;
            }
            else
            {}
            break;
        case HttpRequestDecodeState::BEFORE_PROTOCAL:
            if (isblank(ch))
            {}
            else
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::PROTOCAL;
            }
            break;
        case HttpRequestDecodeState::PROTOCAL:
            if (ch == '/')
            {
                request->SetProtocl(string(strbegin, ptr - strbegin));
                mDecodeState = HttpRequestDecodeState::BEFORE_VERSION;
            }
            break;
        case HttpRequestDecodeState::BEFORE_VERSION:
            if (std::isdigit(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::VERSION;
                break;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID_VERSION;
                break;
            }

        case HttpRequestDecodeState::VERSION:
            if (ch == '.')
            {
                mDecodeState = HttpRequestDecodeState::VERSION_SPLIT;
                break;
            }
            if (ch == CR)
            {
                std::string version(strbegin, ptr - strbegin);
                mDecodeState = HttpRequestDecodeState::WHEN_CR;
            }
            else if (isdigit(ch))
            {
                break;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID;
            }
            break;
        case HttpRequestDecodeState::VERSION_SPLIT:
            if (isdigit(ch))
            {
                mDecodeState = HttpRequestDecodeState::VERSION;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID_VERSION;
            }
            break;
        case HttpRequestDecodeState::HEADER_KEY:
            if (isblank(ch))
            {
                headerKey = string(strbegin, ptr - strbegin);
                mDecodeState = HttpRequestDecodeState::HEADER_BEFORE_COLON;
            }
            else if (ch == ':')
            {
                headerKey = string(strbegin, ptr - strbegin);
                mDecodeState = HttpRequestDecodeState::HEADER_AFTER_COLON;
            }
            break;
        case HttpRequestDecodeState::HEADER_BEFORE_COLON:
            if (isblank(ch))
            {
                break;
            }
            if (ch == ':')
            {
                mDecodeState = HttpRequestDecodeState::HEADER_AFTER_COLON;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID_HEADER;
            }
            break;
        case HttpRequestDecodeState::HEADER_AFTER_COLON:
            if (isblank(ch))
            {
                break;
            }
            else
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::HEADER_VALUE;
            }
            break;
        case HttpRequestDecodeState::HEADER_VALUE:
            if (ch == CR)
            {
                headerVal = string(strbegin, ptr - strbegin);
                request->setHeaderEntry(headerKey, headerVal);
                mDecodeState = HttpRequestDecodeState::WHEN_CR;
            }
            break;
        case HttpRequestDecodeState::WHEN_CR:
            if (ch == LF)
            {
                mDecodeState = HttpRequestDecodeState::CR_LF;
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID;
            }
            break;
        case HttpRequestDecodeState::CR_LF:
            if (ch == CR)
            {
                mDecodeState = HttpRequestDecodeState::CR_LF_CR;
            }
            else if (isblank(ch))
            {
                mDecodeState = HttpRequestDecodeState::INVALID;
            }
            else
            {
                strbegin = ptr;
                mDecodeState = HttpRequestDecodeState::HEADER_KEY;
            }
            break;
        case HttpRequestDecodeState::CR_LF_CR:
            if (ch == LF)
            { // conent length
                if (request->HasHeaderEntry("Content-Length"))
                {
                    request->getHeaderEntryAs("Content-Length", bodyLength);
                    if (bodyLength > 0)
                    {
                        strbegin = ptr + 1;
                        mDecodeState = HttpRequestDecodeState::BODY;
                    }
                    else
                    {
                        mDecodeState = HttpRequestDecodeState::COMPLEIE;
                    }
                }
                else
                {
                    mDecodeState = HttpRequestDecodeState::COMPLEIE;
                }
            }
            else
            {
                mDecodeState = HttpRequestDecodeState::INVALID_HEADER;
            }
            break;
        case HttpRequestDecodeState::BODY:
        {
            request->setBody(string(strbegin, bodyLength));
            mDecodeState = HttpRequestDecodeState::COMPLEIE;
        }
        default:
            break;
        }
        ptr++;
    }
    return mDecodeState == HttpRequestDecodeState::COMPLEIE ? request : nullptr;
}

bool HttpResponseParser::IsVaild(size_t size)
{
    return mDecodeState != HttpResponseDecodeState::INVALID &&
           mDecodeState != HttpResponseDecodeState::INVALID_HEADER &&
           mDecodeState != HttpResponseDecodeState::INVALID_VERSION &&
           mNextPosition < size;
}
HttpResponse::ptr HttpResponseParser::TyeCode(const char *data, size_t len)
{

    HttpResponse::ptr response(new HttpResponse);
    char *ptr = const_cast<char *>(data);
    char *strbegin;
    size_t bodyLength;
    std::string headerKey, headerVal;
    mDecodeState = HttpResponseDecodeState::START;
    while (IsVaild(len))
    {
        mNextPosition++;
        char ch = *ptr;
        switch (mDecodeState)
        {
        case HttpResponseDecodeState::START:
            if (isupper(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpResponseDecodeState::PROTOCOL;
            }
            else if (ch == CR || ch == LF || isblank(ch))
            {}
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::PROTOCOL:
            if (isupper(ch))
            {
                break;
            }
            else if (ch == '/')
            {
                mDecodeState = HttpResponseDecodeState::BEFORE_VERSION;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::BEFORE_VERSION:
            if (isdigit(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpResponseDecodeState::VERSION;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::VERSION:
            if (isdigit(ch))
            {
                break;
            }
            else if (ch == '.')
            {
                mDecodeState = HttpResponseDecodeState::VERSION_SPLIT;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::VERSION_SPLIT:
            if (isdigit(ch))
            {
                break;
            }
            else if (isblank(ch))
            {
                std::string version(strbegin, ptr - strbegin);
                response->setVersion(version);
                mDecodeState = HttpResponseDecodeState::BEFORE_STATUS_CODE;
                break;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::BEFORE_STATUS_CODE:
            if (std::isdigit(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpResponseDecodeState::IN_STATUS_CODE;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::IN_STATUS_CODE:
            if (isdigit(ch))
            {
                break;
            }
            else if (isblank(ch))
            {
                std::string statusCode(strbegin, ptr - strbegin);
                response->SetStatusCode(statusCode);
                mDecodeState = HttpResponseDecodeState::BEFORE_STATUS;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;

        case HttpResponseDecodeState::BEFORE_STATUS:
            if (isupper(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpResponseDecodeState::IN_STATUS;
                break;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
        case HttpResponseDecodeState::IN_STATUS:
            if (ch == CR)
            {
                std::string statusDesc(strbegin, ptr - strbegin);
                response->SetStatusDesc(statusDesc);
                mDecodeState = HttpResponseDecodeState::WHEN_CR;
            }
            else
            {}
            break;
        case HttpResponseDecodeState::WHEN_CR:
            if (ch == LF)
            {
                mDecodeState = HttpResponseDecodeState::CR_LF;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::CR_LF:
            if (ch == CR)
            {
                mDecodeState = HttpResponseDecodeState::CR_LF_CR;
            }
            else if (isalpha(ch))
            {
                strbegin = ptr;
                mDecodeState = HttpResponseDecodeState::HEADER_KEY;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        case HttpResponseDecodeState::HEADER_KEY:
            if (isblank(ch))
            {
                headerKey = string(strbegin, ptr - strbegin);
                mDecodeState = HttpResponseDecodeState::HEADER_BEFORE_COLON;
            }
            else if (ch == ':')
            {
                headerKey = string(strbegin, ptr - strbegin);
                mDecodeState = HttpResponseDecodeState::HEADER_AFTER_COLON;
            }
            break;
        case HttpResponseDecodeState::HEADER_BEFORE_COLON:
            if (isblank(ch))
            {
                break;
            }
            if (ch == ':')
            {
                mDecodeState = HttpResponseDecodeState::HEADER_AFTER_COLON;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID_HEADER;
            }
            break;
        case HttpResponseDecodeState::HEADER_AFTER_COLON:
            if (isblank(ch))
            {
                break;
            }
            else
            {
                strbegin = ptr;
                mDecodeState = HttpResponseDecodeState::HEADER_VALUE;
            }
            break;
        case HttpResponseDecodeState::HEADER_VALUE:
            if (ch == CR)
            {
                headerVal = string(strbegin, ptr - strbegin);
                response->setHeaderEntry(headerKey, headerVal);
                mDecodeState = HttpResponseDecodeState::WHEN_CR;
            }
            break;
        case HttpResponseDecodeState::CR_LF_CR:
            if (ch == LF)
            {
                if (response->HasHeaderEntry("Content-Length"))
                {
                    response->getHeaderEntryAs("Content-Length", bodyLength);
                    if (bodyLength)
                    {
                        strbegin = ptr + 1;
                        mDecodeState = HttpResponseDecodeState::BODY;
                    }
                    else
                    {
                        mDecodeState = HttpResponseDecodeState::COMPLEIE;
                    }
                }
                else
                {
                    mDecodeState = HttpResponseDecodeState::INVALID;
                }
                break;
            }
            break;
        case HttpResponseDecodeState::BODY:
            if (bodyLength < len - mNextPosition)
            {
                std::string body(strbegin, bodyLength);
                response->setBody(body);
                mDecodeState = HttpResponseDecodeState::COMPLEIE;
            }
            else
            {
                mDecodeState = HttpResponseDecodeState::INVALID;
            }
            break;
        default:
            break;
        }
        ptr++;
    }
    return mDecodeState == HttpResponseDecodeState::COMPLEIE ? response
                                                             : nullptr;
}