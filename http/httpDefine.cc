#include <sstream>

#include "httpDefine.h"

namespace util {

std::string g_CRLF = "\r\n";
std::string g_CRLF_DOUBLE = "\r\n\r\n";

std::string content_type_text = "text/html;charset=utf-8";
const char* default_html_template = "<html><body><h1>%s</h1><p>%s</p></body></html>";

const char *httpCodeToString(const int code) {
    switch(code) {
        case HTTP_OK:
            return "OK";
        case HTTP_BADREQUEST:
            return "Bad Request";
        case HTTP_FORBIDDEN:
            return "Forbidden";
        case HTTP_NOTFOUND:
            return "Not Found";
        case HTTP_INTERNALSERVERERROR:
            return "Internal Server Error";
        default:
            return "UnKonw Code";
    }
}

int HttpHeaderComm::getHeaderTotalLength() {
    int len = 0;
    for(auto it : m_maps) {
        len += it.first.size() + 1 + it.second.size() + 2;
    }
    return len;
}

std::string HttpHeaderComm::toHttpString() {
    std::stringstream ss;
    for(auto it : m_maps) {
        ss << it.first << ":" << it.second << "\r\n";
    }
    return ss.str();
}

void HttpHeaderComm::setKeyValue(const std::string &key, const std::string &value) {
    m_maps[key] = value;
}

std::string HttpHeaderComm::getValue(const std::string &key) {
    return m_maps[key];
}


}   // namespace util