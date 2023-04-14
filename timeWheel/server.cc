#include "log.h"
#include "tcpServer.h"
#include "timeWheel.h"

#include <memory>
#include <functional>
#include <iostream>

using namespace std;
using namespace util;

int main() {
    initLog("test_log");
    string ip = "0.0.0.0";
    uint16_t port = 9000;
    IPAddress::ptr addr = make_shared<IPAddress>(ip, port);
    TcpServer::ptr server = make_shared<TcpServer>(addr);

    server->start();


    return 0;
}