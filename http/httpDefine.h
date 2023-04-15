#ifndef _HTTPDEFINE_H
#define _HTTPDEFINE_H

#include <map>
#include <string>

namespace util {

extern std::string g_CRLF;
extern std::string g_CRLF_DOUBLE;

extern std::string content_type_text;
extern const char* default_html_template;

enum HttpMethod {
    GET = 1,
    POST = 2
};

enum HttpCode {
    HTTP_OK = 200,
    HTTP_BADREQUEST = 400,
    HTTP_FORBIDDEN = 403,
    HTTP_NOTFOUND = 404,
    HTTP_INTERNALSERVERERROR = 500,
};

const char *httpCodeToString(const int code);

class HttpHeaderComm {
public:
    HttpHeaderComm() = default;
    ~HttpHeaderComm() = default;

    int getHeaderTotalLength();
    std::string toHttpString();

    void setKeyValue(const std::string &key, const std::string &value);
    std::string getValue(const std::string &key);

    std::map<std::string, std::string> m_maps;
};

class HttpRequestHeader : public HttpHeaderComm {};

class HttpResponseHeader : public HttpHeaderComm {};

}   // namespace util

#endif
