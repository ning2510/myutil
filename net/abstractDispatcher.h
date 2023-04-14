#ifndef _ABSTRACTDISPATCHER_H
#define _ABSTRACTDISPATCHER_H

#include <memory>

#include "abstractData.h"
#include "tcpConnection.h"

namespace util {

class AbstractDispatcher {
public:
    typedef std::shared_ptr<AbstractDispatcher> ptr;

    AbstractDispatcher() {}
    virtual ~AbstractDispatcher() {}

    virtual void dispatch(AbstractData* data, TcpConnection* conn) = 0;

};

}   // namespace util

#endif
