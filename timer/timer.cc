#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#include "timer.h"
#include "log.h"
#include "coroutineHook.h"

extern read_fun_ptr_t g_sys_read_fun;

namespace util {

int64_t getNowMs() {
    timeval val;
    ::gettimeofday(&val, nullptr);
    int64_t re = val.tv_sec * 1000 + val.tv_usec / 1000;    // ms
    return re;
}

Timer::Timer(Reactor *reacor) : FdEvent(reacor) {
    m_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    LOG_INFO << "timer fd = " << m_fd;
    if(m_fd == -1) {
        LOG_ERROR << "timerfd_create error";
    }

    m_read_callback = std::bind(&Timer::onTimer, this);
    addListenEvents(READ);
}

Timer::~Timer() {
    ::close(m_fd);
}

void Timer::addTimerEvent(TimerEvent::ptr event, bool need_reset /*= true*/) {
    LOG_DEBUG << "addTimerEvent arrive_time = " << event->m_arrive_time;
    RWMutex::WriteLock wlock(m_event_mutex);
    bool is_reset = false;
    if(m_pending_events.empty()) {
        is_reset = true;
    } else {
        auto it = m_pending_events.begin();
        if(event->m_arrive_time < it->first) {
            is_reset = true;
        }
    }

    m_pending_events.emplace(event->m_arrive_time, event);
    wlock.unlock();
    
    if(is_reset && need_reset) {
        LOG_DEBUG << "need reset timer";
        resetArriveTime();
    }
}

void Timer::delTimerEvent(TimerEvent::ptr event) {
    event->m_is_cancled = true;

    RWMutex::WriteLock wlock(m_event_mutex);
    auto begin = m_pending_events.lower_bound(event->m_arrive_time);
    auto end = m_pending_events.upper_bound(event->m_arrive_time);
    auto it = begin;
    for(it = begin; it != end; it++) {
        if(it->second == event) {
            LOG_DEBUG << "find timer event, now delete it. src arrive time=" << event->m_arrive_time;
            break;
        }
    }

    if(it != m_pending_events.end()) {
        m_pending_events.erase(it);
    }
    wlock.unlock();

    LOG_DEBUG << "del timer event succ, origin arrvite time=" << event->m_arrive_time;
}

void Timer::resetArriveTime() {
    RWMutex::ReadLock rlock(m_event_mutex);
    std::multimap<int64_t, TimerEvent::ptr> tmp = m_pending_events;
    rlock.unlock();

    if(tmp.size() == 0) {
        LOG_DEBUG << "no timer event pending";
        return ;
    }

    int64_t now = getNowMs();
    auto it = tmp.rbegin();
    if(it->first < now) {
        LOG_DEBUG << "all timer events has already expire";
        return ;
    }

    int64_t interval = it->first - now;

    itimerspec new_time;
    ::memset(&new_time, 0, sizeof(new_time));

    timespec ts;
    ::memset(&ts, 0, sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;
    new_time.it_value = ts;

    int rt = timerfd_settime(m_fd, 0, &new_time, nullptr);
    if (rt != 0) {
        LOG_ERROR << "timerfd_settime error, interval = " << interval;
    }
}

void Timer::onTimer() {
    LOG_DEBUG << "Timer::onTimer is executed";
    char buf[8];
    while(1) {
        if((g_sys_read_fun(m_fd, buf, 8) == -1) && errno == EAGAIN) {
            break;
        }
    }

    int64_t now = getNowMs();
    RWMutex::WriteLock wlock(m_event_mutex);
    auto it = m_pending_events.begin();
    std::vector<TimerEvent::ptr> tmp;
    std::vector<std::pair<int64_t, std::function<void()>>> tmp_task;
    for(it = m_pending_events.begin(); it != m_pending_events.end(); it++) {
        if(it->first <= now && !it->second->m_is_cancled) {
            tmp.push_back(it->second);
            tmp_task.push_back(std::make_pair(it->first, it->second->m_task));
        } else {
            break;
        }
    }

    m_pending_events.erase(m_pending_events.begin(), it);
    wlock.unlock();

    for(auto etmp : tmp) {
        if(etmp->m_is_repeated) {
            etmp->resetTime();
            addTimerEvent(etmp, false);
        }
    }

    resetArriveTime();

    for(auto ttmp : tmp_task) {
        ttmp.second();
    }
    LOG_DEBUG << "Timer::onTimer is end";
}

}   // namespace util