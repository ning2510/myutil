#ifndef _HTTPRESPONSE_H
#define _HTTPRESPONSE_H

#include <string>
#include <memory>

#include "httpDefine.h"
#include "abstractData.h"

namespace util {

class HttpResponse : public AbstractData {
public:
    typedef std::shared_ptr<HttpResponse> ptr;

    std::string m_res_version;
    int m_res_code;
    std::string m_res_info;

    HttpResponseHeader m_res_header;

    std::string m_res_body;
};

}   // namespace util

#endif
