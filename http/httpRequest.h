#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H

#include <map>
#include <string>
#include <memory>

#include "httpDefine.h"
#include "abstractData.h"

namespace util {

class HttpRequest : public AbstractData {
public:
    typedef std::shared_ptr<HttpRequest> ptr;

    HttpMethod m_req_method;
    std::string m_req_path;
    std::string m_req_query;
    std::string m_req_version;

    HttpRequestHeader m_req_header;

    std::string m_req_body;

    std::map<std::string, std::string> m_query_maps;
};


}   // namespace util

#endif
