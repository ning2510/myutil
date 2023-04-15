#include <sstream>
#include <algorithm>

#include "log.h"
#include "httpCodec.h"
#include "httpResponse.h"

namespace util {

void HttpCodec::encode(Buffer *buf, AbstractData *data) {
    LOG_DEBUG << "HttpCodec::encode start";
    HttpResponse *response = dynamic_cast<HttpResponse *>(data);
    response->encode_succ = false;

    std::stringstream ss;
    ss << response->m_res_version << " " << response->m_res_code << " "
       << response->m_res_info << "\r\n" << response->m_res_header.toHttpString()
       << "\r\n" << response->m_res_body;

    std::string http_res = ss.str();
    LOG_DEBUG << "encode http response is:  " << http_res;

    buf->append(http_res.c_str(), http_res.length());
    response->encode_succ = true;

    LOG_DEBUG << "HttpCodec::encode end";
}

void HttpCodec::decode(Buffer *buf, AbstractData *data) {
    LOG_DEBUG << "HttpCodec::decode start";
    std::string strs = "";
    if(!buf || !data) {
        LOG_ERROR << "decode error! buf or data nullptr";
        return ;
    }

    HttpRequest *request = dynamic_cast<HttpRequest *>(data);
    if(!request) {
        LOG_ERROR << "not httprequest type";
        return ;
    }
    strs = buf->retrieveAllAsString();

    bool is_parse_req_line = false;
    bool is_parse_req_header = false;
    bool is_parse_req_content = false;
    
    int read_size = 0;
    std::string tmp(strs);
    LOG_DEBUG << "pending to parse str:" << tmp << ", total size = " << tmp.size();
    int len = tmp.size();
    while(1) {
        if(!is_parse_req_line) {
            size_t i = tmp.find(g_CRLF);
            if(i == tmp.npos) {
                LOG_ERROR << "not found CRLF in buffer";
                return ;
            }

            if(i == tmp.size() - 2) {
                LOG_DEBUG << "need to read more data";
                break ;
            }

            is_parse_req_line = parseHttpRequestLine(request, tmp.substr(0, i));
            if(!is_parse_req_line) {
                return ;
            }

            tmp = tmp.substr(i + 2, len - 2 - i);
            len = tmp.size();
            read_size = read_size + i + 2;
        }

        if(!is_parse_req_header) {
            size_t j = tmp.find(g_CRLF_DOUBLE);
            if(j == tmp.npos) {
                LOG_ERROR << "not found CRLF_DOUBLE in buffer";
                return ;
            }

            is_parse_req_header = parseHttpRequestHeader(request, tmp.substr(0, j));
            if(!is_parse_req_header) {
                return ;
            }
            tmp = tmp.substr(j + 4, len - 4 - j);
            len = tmp.size();
            read_size = read_size + j + 4;
        }

        if(!is_parse_req_content) {
            int content_len = std::atoi(request->m_req_header.m_maps["Content-Length"].c_str());
            if ((int)strs.length() - read_size < content_len) {
                LOG_DEBUG << "need to read more data";
                return ;
            }

            if(request->m_req_method == POST && content_len != 0) {
                is_parse_req_content = parseHttpRequestContent(request, tmp.substr(0, content_len));
                if(!is_parse_req_content) {
                    return ;
                }

                read_size = read_size + content_len;
            } else {
                is_parse_req_content = true;
            }
        }

        if(is_parse_req_line && is_parse_req_header && is_parse_req_content) {
            LOG_DEBUG << "parse http request success, read size is " << read_size << " bytes";
            break ;
        }
    }

    request->decode_succ = true;
    data = request;

    LOG_DEBUG << "HttpCodec::decode end";
}

bool HttpCodec::parseHttpRequestLine(HttpRequest *request, const std::string &str) {
    size_t s1 = str.find_first_of(" ");
    size_t s2 = str.find_last_of(" ");

    if(s1 == str.npos || s2 == str.npos || s1 == s2) {
        LOG_ERROR << "parse http request line error, not found two spaces";
        return false;
    }

    std::string method = str.substr(0, s1);
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    if(method == "GET") {
        request->m_req_method = HttpMethod::GET;
    } else if(method == "POST") {
        request->m_req_method = HttpMethod::POST;
    } else {
        LOG_ERROR << "parse http request line error, not support method: " << method;
        return false;
    }

    std::string version = str.substr(s2 + 1, str.size() - s2 - 1);
    std::transform(version.begin(), version.end(), version.begin(), ::toupper);
    if(version != "HTTP/1.1" && version != "HTTP/1.0") {
        LOG_ERROR << "parse http request line error, not support version: " << version;
        return false;
    }
    request->m_req_version = version;

    std::string url = str.substr(s1 + 1, s2 - s1 - 1);
    size_t i = url.find("://");
    if (i != url.npos && i + 3 >= url.length()) {
        LOG_ERROR << "parse http request request line error, bad url:" << url;
        return false;
    }

    int len = 0;
    if(i == url.npos) {
        LOG_DEBUG << "url only have path, url = " << url;
    } else {
        url = url.substr(i + 3, s2 - s1 - i - 4);
        LOG_DEBUG << "delete http prefix, url = " << url;

        i = url.find_first_of("/");
        len = url.size();
        if(i == url.npos || i == len - 1) {
            LOG_DEBUG << "http request root path, and query is empty";
            return true;
        }
        url = url.substr(i + 1, len - i - 1);
    }

    len = url.size();
    i = url.find_first_of("?");
    if(i == url.npos) {
        request->m_req_path = url;
        LOG_DEBUG << "http request path:" << request->m_req_path << "and query is empty";
        return true;
    }

    request->m_req_path = url.substr(0, i);
    request->m_req_query = url.substr(i + 1, len - i - 1);
    LOG_DEBUG << "http request path:" << request->m_req_path << ", and query:" << request->m_req_query;

    SplitStrToMap(request->m_req_query, "&", "=", request->m_query_maps);
    return true;
}

bool HttpCodec::parseHttpRequestHeader(HttpRequest *request, const std::string &str) {
    if(str.empty() || str.size() < 4 || str == "\r\n\r\n") {
        return true;
    }

    std::string tmp = str;
    SplitStrToMap(tmp, g_CRLF, ":", request->m_req_header.m_maps);
    return true;
}

bool HttpCodec::parseHttpRequestContent(HttpRequest *request, const std::string &str) {
    if(str.empty()) {
        return true;
    }

    request->m_req_body = str;
    return true;
}

void SplitStrToMap(const std::string str, const std::string &split_str,
                   const std::string &joiner, std::map<std::string, std::string> &res) {

    if (str.empty() || split_str.empty() || joiner.empty()) {
        LOG_DEBUG << "str or split_str or joiner_str is empty";
        return ;
    }

    std::string tmp = str;
    std::vector<std::string> vec;
    SplitStrToVector(tmp, split_str, vec);
    for (auto i : vec) {
        if (!i.empty()) {
        size_t j = i.find_first_of(joiner);
        if (j != i.npos && j != 0) {
            std::string key = i.substr(0, j);
            std::string value = i.substr(j + joiner.length(), i.length() - j - joiner.length());
            LOG_DEBUG << "insert key = " << key << ", value=" << value;
            res[key.c_str()] = value;
        }
        }
    }
}

void SplitStrToVector(const std::string &str, const std::string &split_str,
                      std::vector<std::string> &res) {

    if (str.empty() || split_str.empty()) {
        LOG_DEBUG << "str or split_str is empty";
        return ;
    }

    std::string tmp = str;
    if (tmp.substr(tmp.length() - split_str.length(), split_str.length()) != split_str) {
        tmp += split_str;
    }

    while (1) {
        size_t i = tmp.find_first_of(split_str);
        if (i == tmp.npos) {
            return ;
        }

        int l = tmp.length();
        std::string x = tmp.substr(0, i);
        tmp = tmp.substr(i + split_str.length(), l - i - split_str.length());
        
        if (!x.empty()) {
            res.push_back(std::move(x));
        }
    }

}

}   // namespace util