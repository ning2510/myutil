#ifndef _HTTPDISPATCHER_H
#define _HTTPDISPATCHER_H

#include <map>
#include <memory>

#include "httpServlet.h"
#include "tcpConnection.h"
#include "abstractDispatcher.h"

namespace util {

class HttpDispatcher : public AbstractDispatcher {
public:
    HttpDispatcher() = default;
    ~HttpDispatcher() = default;

    void dispatch(AbstractData *data, TcpConnection *conn);

    void registerServlet(const std::string &path, HttpServlet::ptr servlet);

    std::map<std::string, HttpServlet::ptr> m_servlets;
};

}   // namespace util

#endif
