#include "qslary/http/Http.h"
#include "qslary/http/HttpParser.h"

using namespace qslary;
using namespace qslary::http;

void testRequest()
{
    std::string str =
        "POST /audiolibrary/music?ar=1595301089068&n=1p1 HTTP/1.1\r\n"
        "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, "
        "application/vnd.ms-excel, application/vnd.ms-powerpoint, "
        "application/msword, application/x-silverlight, "
        "application/x-shockwave-flash\r\n"
        "Referer: http://www.google.cn\r\n"
        "Accept-Language: zh-cn\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; "
        ".NET CLR 2.0.50727; TheWorld)\r\n"
        "content-length:28\r\n"
        "Host: www.google.cn\r\n"
        "Connection: Keep-Alive\r\n"
        "Cookie: "
        "PREF=ID=80a06da87be9ae3c:U=f7167333e2c3b714:NW=1:TM=1261551909:LM="
        "1261551917:S=ybYcq2wpfefs4V9g; "
        "NID=31=ojj8d-IygaEtSxLgaJmqSjVhCspkviJrB6omjamNrSm8lZhKy_"
        "yMfO2M4QMRKcH1g0iQv9u\r\n"
        "\r\n"
        "hl=zh-CN&source=hp&q=domety";
    auto parser = new HttpRequestParser;
    auto res = parser->TryDecode(str.data(), str.size());
    std::cout << res->toString() << std::endl;
}

void testResponse()
{
    std::string str = "HTTP/1.1 200 OK\r\n"
                      "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
                      "Server: Apache\r\n"
                      "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
                      "ETag: \"51-47cf7e6ee8400\"\r\n"
                      "Accept-Ranges: bytes\r\n"
                      "Content-Length: 81\r\n"
                      "Cache-Control: max-age=86400\r\n"
                      "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
                      "Connection: Close\r\n"
                      "Content-Type: text/html\r\n\r\n"
                      "<html>\r\n"
                      "<meta http-equiv=\"refresh\" "
                      "content=\"0;url=http://www.baidu.com/\">\r\n"
                      "</html>\r\n";
    auto parser = new HttpResponseParser;
    auto res = parser->TyeCode(str.data(), str.size());

    for (auto &it : res->getHeaders())
    {
        std::cout << it.first << " " << it.second << std::endl;
    }
}

int main(int argc, char *argv[])
{
    //   testRequest();

    testResponse();
    return 0;
}