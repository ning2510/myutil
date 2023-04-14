#ifndef _REACTOR_H
#define _REACTOR_H

#include <map>
#include <queue>
#include <atomic>
#include <vector>
#include <functional>
#include <sys/socket.h>
#include <sys/types.h>

#include "mutex.h"
#include "fdEvent.h"
#include "coroutine.h"
#include "timer.h"

namespace util {

enum ReactorType {
    MainReactor = 1,
    SubReactor = 2
};

class Reactor {
public:
    std::shared_ptr<Reactor> ptr;

    explicit Reactor();
    ~Reactor();

    void addEvent(int fd, epoll_event event, bool is_wakeup = true);
    void delEvent(int fd, bool is_wakeup = true);

    void addTask(std::function<void()> task, bool is_wakeup = true);
    void addTask(std::vector<std::function<void()>> task, bool is_wakeup = true);
    void addCoroutine(Coroutine::ptr cor, bool is_wakeup = true);

    void wakeup();
    void loop();
    void stop();

    Timer *getTimer();
    pid_t getTid();

    void setReactorType(ReactorType type);

    static Reactor *GetReactor();

private:
    void addWakeupFd();
    bool isInLoopThread() const;

    void addEventInLoopThread(int fd, epoll_event event);
    void delEventInLoopThread(int fd);

    int m_epfd;
    int m_wake_fd;
    int m_timer_fd;
    bool m_stop_flag;
    bool m_is_looping;
    bool is_init_timer;

    pid_t m_tid;
    Mutex m_mutex;

    std::vector<int> m_fds;
    std::atomic_int m_fd_size;

    std::map<int, epoll_event> m_pending_add_fds;   // <fd, epoll_event>
    std::vector<int> m_pending_del_fds;
    std::vector<std::function<void()>> m_pending_tasks;

    Timer *m_timer;
    ReactorType m_reactor_type;    
};

class CoroutineTaskQueue {
public:
    static CoroutineTaskQueue *GetCoroutineTaskQueue();

    void push(FdEvent *fd);
    FdEvent *pop();

private:
    std::queue<FdEvent *> m_task;
    Mutex m_mutex;
};

}   // namespace util

#endif
