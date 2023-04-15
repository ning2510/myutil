#include "log.h"
#include "httpServlet.h"

namespace util {

void HttpServlet::handleNotFound(HttpRequest *req, HttpResponse *res) {
    LOG_DEBUG << "return 404 html";

    setHttpCode(res, HTTP_NOTFOUND);
    char buf[512] = {0};
    sprintf(buf, default_html_template, std::to_string(HTTP_NOTFOUND).c_str(), httpCodeToString(HTTP_NOTFOUND));

    res->m_res_body = std::string(buf);
    res->m_res_header.m_maps["Content-Type"] = content_type_text;
    res->m_res_header.m_maps["Content-Length"] = std::to_string(res->m_res_body.size());
}

void HttpServlet::setHttpCode(HttpResponse *res, const int code) {
    res->m_res_code = code;
    res->m_res_info = std::string(httpCodeToString(code));
}

void HttpServlet::setHttpContentType(HttpResponse *res, const std::string &content_type) {
    res->m_res_header.m_maps["Content-Type"] = content_type;
}

void HttpServlet::setHttpBody(HttpResponse *res, const std::string &body) {
    res->m_res_body = body;
    res->m_res_header.m_maps["Content-Length"] = std::to_string(res->m_res_body.size());
}

void HttpServlet::setCommParam(HttpRequest *req, HttpResponse *res) {
    LOG_DEBUG << "set response version=" << req->m_req_version;
    res->m_res_version = req->m_req_version;
    res->m_res_header.m_maps["Connection"]= req->m_req_header.m_maps["Connection"];
}

void NotFoundHttpServlet::handle(HttpRequest *req, HttpResponse *res) {
    handleNotFound(req, res);
}

std::string NotFoundHttpServlet::getServletName() {
    return "NotFoundHttpServlet";
}


}   // namespace util