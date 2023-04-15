#include "log.h"
#include "tcpServer.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "httpServlet.h"

#include <memory>
#include <functional>
#include <iostream>

using namespace std;
using namespace util;

const char* html = "<html><body><h1>Test success !</h1><p>%s</p></body></html>";

class TestHttpServlet : public HttpServlet {
 public:
  TestHttpServlet() = default;
  ~TestHttpServlet() = default;

  void handle(HttpRequest* req, HttpResponse* res) {
    LOG_DEBUG << "TestHttpServlet get request";
    setHttpCode(res, HTTP_OK);
    setHttpContentType(res, "text/html;charset=utf-8");

    std::stringstream ss;
    ss << "TestHttpServlet Echo Success!! Your id is " << req->m_query_maps["id"];
    char buf[512];
    sprintf(buf, html, ss.str().c_str());
    setHttpBody(res, std::string(buf));
    LOG_DEBUG << ss.str();
  }

  std::string getServletName() {
    return "TestHttpServlet";
  }

};

int main() {
    initLog("test_log");
    string ip = "0.0.0.0";
    uint16_t port = 9000;
    IPAddress::ptr addr = make_shared<IPAddress>(ip, port);
    TcpServer::ptr server = make_shared<TcpServer>(addr, HTTP);

    bool rt = server->registerHttpServlet("/test", make_shared<TestHttpServlet>());
    if(rt) {
        cout << "register http servlet /test success" << endl;
    } else {
        cout << "register http servlet /test fail" << endl;
    }

    server->start();

    return 0;
}