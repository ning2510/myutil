#include "log.h"
#include "timeWheel.h"

namespace util {

TimeWheel::TimeWheel(Reactor *reactor, int bucket_count, int interval/* = 10*/)
    : m_reactor(reactor),
      m_bucket_count(bucket_count),
      m_interval(interval) {

    for(int i = 0; i < bucket_count; i++) {
        std::vector<TcpConnectionSlot::ptr> tmp;
        m_wheel.push(tmp);
    }

    m_timer_event = std::make_shared<TimerEvent>(interval * 1000, true, std::bind(&TimeWheel::loop, this));
    m_reactor->getTimer()->addTimerEvent(m_timer_event);
}

TimeWheel::~TimeWheel() {
    // m_timer_event's is_repeated is true, so delete it
    m_reactor->getTimer()->delTimerEvent(m_timer_event);
}

void TimeWheel::fresh(TcpConnectionSlot::ptr slot) {
    LOG_DEBUG << "TimeWheel fresh";
    m_wheel.back().emplace_back(slot);
}

void TimeWheel::loop() {
    LOG_DEBUG << "TimeWhhel loop";
    m_wheel.pop();
    std::vector<TcpConnectionSlot::ptr> tmp;
    m_wheel.push(tmp);
}

}   // namespace util