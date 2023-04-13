#include "log.h"
#include "reactor.h"
#include "fdEvent.h"

#include <fcntl.h>

namespace util {

static FdEventContainer *g_FdContainer = nullptr;

FdEvent::FdEvent(Reactor *reactor, int fd /*= -1*/) 
    : m_reactor(reactor), 
      m_fd(fd), 
      m_listen_events(0) {}

FdEvent::FdEvent(int fd /*= -1*/)
    : m_reactor(nullptr), 
      m_fd(fd), 
      m_listen_events(0) {}

FdEvent::~FdEvent() {}

void FdEvent::handleEvent(int flag) {
    if(flag == READ) {
        m_read_callback();
    } else if(flag == WRITE) {
        m_write_callback();
    } else {
        LOG_ERROR << "FdEvent::handleEvent invalid flag";
    }
}

void FdEvent::setCallBack(IOEvent flag, const std::function<void()> &cb) {
    if(flag == READ) {
        m_read_callback = std::move(cb);
    } else if(flag == WRITE) {
        m_write_callback = std::move(cb);
    } else {
        LOG_ERROR << "FdEvent::setCallBack invalid flag";
    }
}

std::function<void()> FdEvent::getCallBack(IOEvent flag) const {
    if(flag == READ) {
        return m_read_callback;
    } else if(flag == WRITE) {
        return m_write_callback;
    } else {
        LOG_ERROR << "FdEvent::getCallBack invalid flag";
    }
}

void FdEvent::addListenEvents(IOEvent event) {
    if(m_listen_events & event) {
        LOG_INFO << "already has this event, will skip";
        return ;
    }

    m_listen_events |= event;
    updateToReactor();
}

void FdEvent::delListenEvents(IOEvent event) {
    if(!(m_listen_events & event)) {
        LOG_INFO << "this event is not exist, will skip";
        return ;
    }

    m_listen_events &= ~event;
    updateToReactor();
}

int FdEvent::getListenEvents() const {
    return m_listen_events;
}

void FdEvent::setFd(const int fd) {
    m_fd = fd;
}

int FdEvent::getFd() const {
    return m_fd;
}

void FdEvent::setNonBlock() {
    if(m_fd == -1) {
        LOG_ERROR << "FdEvent::setNonBlock: fd is not exist";
        return ;
    }

    int flag = ::fcntl(m_fd, F_GETFL, 0);
    if(flag & O_NONBLOCK) {
        LOG_DEBUG << "fd already set nonblock";
        return ;
    }

    ::fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
    flag = ::fcntl(m_fd, F_GETFL, 0);
    if(flag & O_NONBLOCK) {
        LOG_INFO << "fd set nonblock success";
    } else {
        LOG_ERROR << "fd set nonblock error";
    }
}

bool FdEvent::isNonBlock() {
    if(m_fd == -1) {
        LOG_ERROR << "FdEvent::setNonBlock: fd is not exist";
        return false;
    }
    int flag = ::fcntl(m_fd, F_GETFL, 0);
    return flag & O_NONBLOCK;
}

void FdEvent::setCoroutine(Coroutine *cor) {
    m_coroutine = cor;
}

Coroutine *FdEvent::getCoroutine() {
    return m_coroutine;
}

void FdEvent::clearCoroutine() {
    m_coroutine = nullptr;
}

void FdEvent::updateToReactor() {
    epoll_event event;
    event.data.ptr = this;
    event.events = m_listen_events;

    if(!m_reactor) {
        m_reactor = Reactor::GetReactor();
    }

    m_reactor->addEvent(m_fd, event);
}

void FdEvent::unregisterFromReactor() {
    if(!m_reactor) {
        m_reactor = Reactor::GetReactor();
    }

    m_reactor->delEvent(m_fd);
    m_listen_events = 0;
    m_read_callback = nullptr;
    m_write_callback = nullptr;
}

void FdEvent::setReactor(Reactor *reactor) {
    m_reactor = reactor;
}

Reactor *FdEvent::getReactor() const {
    return m_reactor;
}


FdEventContainer::FdEventContainer(int size) {
    for(int i = 0; i < size; i++) {
        FdEvent::ptr tmp = std::make_shared<FdEvent>(i);
        m_fds.push_back(tmp);
    }
}

FdEvent::ptr FdEventContainer::getFdEvent(int fd) {
    RWMutex::ReadLock rlock(m_mutex);
    if(fd < m_fds.size()) {
        FdEvent::ptr tmp = m_fds[fd];
        rlock.unlock();
        return tmp;
    }
    rlock.unlock();

    RWMutex::WriteLock wlock(m_mutex);
    int size = (int)(fd * 1.5);
    for(int i = m_fds.size(); i < size; i++) {
        FdEvent::ptr tmp = std::make_shared<FdEvent>(i);
        m_fds.push_back(tmp);
    }

    FdEvent::ptr tmp = m_fds[fd];
    wlock.unlock();
    return tmp;
}

FdEventContainer *FdEventContainer::GetFdContainer() {
    if(g_FdContainer == nullptr) {
        g_FdContainer = new FdEventContainer(1000);
    }
    return g_FdContainer;
}


}   // namespace util