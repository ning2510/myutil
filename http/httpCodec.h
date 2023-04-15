#ifndef _HTTPCODEC_H
#define _HTTPCODEC_H

#include <map>
#include <string>

#include "buffer.h"
#include "httpRequest.h"
#include "abstractData.h"
#include "abstractCodec.h"

namespace util {

void SplitStrToMap(const std::string str, const std::string &split_str,
                   const std::string &joiner, std::map<std::string, std::string> &res);
void SplitStrToVector(const std::string &str, const std::string &split_str,
                      std::vector<std::string> &res);

class HttpCodec : public AbstractCodec {
public:
    HttpCodec() {}
    ~HttpCodec() {}

    void encode(Buffer *buf, AbstractData *data);
    void decode(Buffer *buf, AbstractData *data);

    ProtocalType getProtocalType() {
        return HTTP;
    }

private:
    bool parseHttpRequestLine(HttpRequest *request, const std::string &str);
    bool parseHttpRequestHeader(HttpRequest *request, const std::string &str);
    bool parseHttpRequestContent(HttpRequest *request, const std::string &str);

};

}   // namespace util

#endif
