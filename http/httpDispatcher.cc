#include "log.h"
#include "httpDispatcher.h"

namespace util {

void HttpDispatcher::dispatch(AbstractData *data, TcpConnection *conn) {
    HttpRequest *req = dynamic_cast<HttpRequest *>(data);
    HttpResponse res;

    std::string url_path = req->m_req_path;
    if(!url_path.empty()) {
        auto it = m_servlets.find(url_path);
        if(it == m_servlets.end()) {
            LOG_ERROR << "404, url path [" << url_path << "]";
            NotFoundHttpServlet servlet;
            servlet.setCommParam(req, &res);
            servlet.handle(req, &res);
        } else {
            it->second->setCommParam(req, &res);
            it->second->handle(req, &res);
        }
    }

    conn->getCodec()->encode(conn->getWriteBuffer(), &res);

    LOG_INFO << "end dispatch client http request";
}

void HttpDispatcher::registerServlet(const std::string &path, HttpServlet::ptr servlet) {
    auto it = m_servlets.find(path);
    if(it == m_servlets.end()) {
        LOG_DEBUG << "register servlet success to path [" << path << "]";
        m_servlets[path] = servlet;
    } else {
        LOG_ERROR << "failed to register, beacuse path [" << path << "] has already register sertlet";
    }
}

}   // namespace util