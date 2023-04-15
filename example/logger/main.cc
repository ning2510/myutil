#include "log.h"

#include <iostream>

using namespace std;

int main() {
    util::initLog("test_log");

    LOG_DEBUG << "DEBUG 信息";

    LOG_INFO << "INFO 信息";

    LOG_WARN << "WARN 信息";

    LOG_ERROR << "ERROR 信息";

    LOG_DEBUG << "-------------------------------------";

    LOG_DEBUG << "DEBUG 信息";

    LOG_INFO << "INFO 信息";

    LOG_WARN << "WARN 信息";

    LOG_ERROR << "ERROR 信息";

    LOG_DEBUG << "-------------------------------------";

    LOG_DEBUG << "DEBUG 信息";

    LOG_INFO << "INFO 信息";

    LOG_WARN << "WARN 信息";

    LOG_ERROR << "ERROR 信息";
    
    return 0;
}