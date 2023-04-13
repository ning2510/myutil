#ifndef _FDEVENT_H
#define _FDEVENT_H

#include <memory>
#include <vector>
#include <sys/epoll.h>
#include <functional>

#include "mutex.h"
#include "coroutine.h"

namespace util {

/* 避免头文件相互包含 */
class Reactor;

enum IOEvent {
    READ = EPOLLIN,
    WRITE = EPOLLOUT
};

class FdEvent {
public:
    typedef std::shared_ptr<FdEvent> ptr;

    FdEvent(Reactor *reactor, int fd = -1);
    FdEvent(int fd = -1);
    virtual ~FdEvent();

    void handleEvent(int flag);

    void setCallBack(IOEvent flag, const std::function<void()> &cb);
    std::function<void()> getCallBack(IOEvent flag) const;

    void addListenEvents(IOEvent event);
    void delListenEvents(IOEvent event);
    int getListenEvents() const;

    void setFd(const int fd);
    int getFd() const;

    void setNonBlock();
    bool isNonBlock();

    void setCoroutine(Coroutine *cor);
    Coroutine *getCoroutine();
    void clearCoroutine();

    void updateToReactor();
    void unregisterFromReactor();
    void setReactor(Reactor *);
    Reactor *getReactor() const;

protected:
    int m_fd;
    int m_listen_events;

    std::function<void()> m_read_callback;
    std::function<void()> m_write_callback;

    Reactor *m_reactor;
    Coroutine *m_coroutine;
};


class FdEventContainer {
public:
    FdEventContainer(int size);

    FdEvent::ptr getFdEvent(int fd);

    static FdEventContainer *GetFdContainer();

private:
    RWMutex m_mutex;
    std::vector<FdEvent::ptr> m_fds;
};


}   // namespace util

#endif
