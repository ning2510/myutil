#ifndef _TIMEWHEEL_H
#define _TIMEWHEEL_H

#include <queue>
#include <vector>

#include "timer.h"
#include "reactor.h"
#include "abstactSlot.h"
#include "tcpConnection.h"

namespace util {

class TimeWheel {
public:
    typedef std::shared_ptr<TimeWheel> ptr;
    typedef AbstractSlot<TcpConnection> TcpConnectionSlot;

    TimeWheel(Reactor *reactor, int bucket_count, int interval = 10);
    ~TimeWheel();

    void fresh(TcpConnectionSlot::ptr slot);
    void loop();

private:
    Reactor *m_reactor;
    int m_bucket_count;
    int m_interval;     // second

    TimerEvent::ptr m_timer_event;
    std::queue<std::vector<TcpConnectionSlot::ptr>> m_wheel;
};

}   // namespace util

#endif
