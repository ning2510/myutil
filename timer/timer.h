#ifndef _TIMER_H
#define _TIMER_H

#include <map>
#include <functional>

#include "log.h"
#include "fdEvent.h"
#include "mutex.h"

namespace util {

/* 避免头文件相互包含 */
class Reactor;

int64_t getNowMs();

class TimerEvent {
public:
    typedef std::shared_ptr<TimerEvent> ptr;

    TimerEvent(int64_t interval, bool is_repeated, std::function<void()> task)
        : m_interval(interval),
          m_is_repeated(is_repeated),
          m_is_cancled(false),
          m_task(task) {

        m_arrive_time = getNowMs() + interval;
    }

    void resetTime() {
        m_arrive_time = getNowMs() + m_interval;
        m_is_cancled = false;
    }

    void wake() {
        m_is_cancled = false;
    }

    void cancle() {
        m_is_cancled = true;
    }

    void cancleRepeated () {
        m_is_repeated = false;
    }

    int64_t m_arrive_time;  // ms
    int64_t m_interval;     // ms

    bool m_is_repeated;
    bool m_is_cancled;

    std::function<void()> m_task;
};


class Timer : public util::FdEvent {
public:
    typedef std::shared_ptr<Timer> ptr;

    Timer(Reactor *reacor);
    ~Timer();

    void addTimerEvent(TimerEvent::ptr event, bool need_reset = true);
    void delTimerEvent(TimerEvent::ptr event);

    void onTimer();
    void resetArriveTime();

// private:
    std::multimap<int64_t, TimerEvent::ptr> m_pending_events;
    RWMutex m_event_mutex;
};

}   // namespace util

#endif