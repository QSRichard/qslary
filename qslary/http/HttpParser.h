#include "qslary/http/Http.h"
#include <cstddef>

namespace qslary
{
namespace http
{

#define CR '\r' //回车
#define LF '\n' //换行

enum class HttpRequestDecodeState
{
    INVALID,         //无效
    INVALID_METHOD,  //无效请求方法
    INVALID_URI,     //无效的请求路径
    INVALID_VERSION, //无效的协议版本号
    INVALID_HEADER,  //无效请求头

    START,  //请求行开始
    METHOD, //请求方法

    BEFORE_URI,             //请求连接前的状态,需要'/'开头
    IN_URI,                 // url处理
    BEFORE_URI_PARAM_KEY,   // URL请求参数键之前
    URI_PARAM_KEY,          // URL请求参数键
    BEFORE_URI_PARAM_VALUE, // URL的参数值之前
    URI_PARAM_VALUE,        // URL请求参数值

    BEFORE_PROTOCAL, //协议解析之前
    PROTOCAL,        //协议

    BEFORE_VERSION, //版本开始之前
    VERSION_SPLIT,  //版本分割符号'.'
    VERSION,        //版本

    HEADER_KEY,
    HEADER_BEFORE_COLON, //请求头冒号之前
    HEADER_AFTER_COLON,  //请求头冒号之后
    HEADER_VALUE,        //值

    WHEN_CR,  //遇到一个回车
    CR_LF,    //回车换行
    CR_LF_CR, //回车换行之后

    BODY,    //请求体
    COMPLEIE //完成
};

enum class HttpResponseDecodeState
{
    INVALID,         //无效
    INVALID_VERSION, //无效的协议版本号
    INVALID_HEADER,  //无效请求头

    START, //响应行开始
    PROTOCOL,

    BEFORE_VERSION,
    VERSION,
    VERSION_SPLIT,

    BEFORE_STATUS_CODE,
    IN_STATUS_CODE,

    BEFORE_STATUS,
    IN_STATUS,

    HEADER_KEY,
    HEADER_BEFORE_COLON, //请求头冒号之前
    HEADER_AFTER_COLON,  //请求头冒号之后
    HEADER_VALUE,        //值

    WHEN_CR,  //遇到一个回车
    CR_LF,    //回车换行
    CR_LF_CR, //回车换行之后

    BODY,    //响应体
    COMPLEIE //完成
};

class HttpRequestParser
{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser() = default;

    HttpRequest::ptr TryDecode(const char *mData, size_t mSize);

    bool IsVaild(size_t mSize) const;

private:
    size_t mNextPosition = 0;
    HttpRequestDecodeState mDecodeState;
};

class HttpResponseParser
{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser() = default;

    HttpResponse::ptr TyeCode(const char *data, size_t len);
    bool IsVaild(size_t size);

private:
    HttpResponseDecodeState mDecodeState;
    size_t mNextPosition = 0;
};

} // namespace http
} // namespace qslary
