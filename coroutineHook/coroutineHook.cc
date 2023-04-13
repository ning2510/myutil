#include <assert.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include "reactor.h"
#include "fdEvent.h"
#include "coroutine.h"
#include "coroutineHook.h"

#define HOOK_SYS_FUNC(name) name##_fun_ptr_t g_sys_##name##_fun = (name##_fun_ptr_t)dlsym(RTLD_NEXT, #name);

HOOK_SYS_FUNC(accept);
HOOK_SYS_FUNC(connect);
HOOK_SYS_FUNC(read);
HOOK_SYS_FUNC(write);
HOOK_SYS_FUNC(sleep);

namespace util {

static bool g_hook = true;

void SetHook(bool value) {
    g_hook = value;
}

void toEpoll(FdEvent::ptr fd_event, IOEvent events) {
    Coroutine *cor = Coroutine::GetCurrentCoroutine();
    fd_event->setCoroutine(cor);
    fd_event->addListenEvents(events);
}

int accept_hook(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if(Coroutine::IsMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys accept func";
        return g_sys_accept_fun(sockfd, addr, addrlen);
    }

    FdEvent::ptr fd_event = FdEventContainer::GetFdContainer()->getFdEvent(sockfd);
    fd_event->setReactor(Reactor::GetReactor());
    fd_event->setNonBlock();

    int n = g_sys_accept_fun(sockfd, addr, addrlen);
    if(n > 0) {
        return n;
    }

    toEpoll(fd_event, IOEvent::READ);

    LOG_DEBUG << "accept func to yield";
    Coroutine::Yield();
    LOG_DEBUG << "accept func yield back, now to call sys accept";

    fd_event->delListenEvents(IOEvent::READ);
    fd_event->clearCoroutine();
    return g_sys_accept_fun(sockfd, addr, addrlen);
}

int connect_hook(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if(Coroutine::IsMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys connect func";
        g_sys_connect_fun(sockfd, addr, addrlen);
    }

    FdEvent::ptr fd_event = FdEventContainer::GetFdContainer()->getFdEvent(sockfd);
    Reactor *reactor = Reactor::GetReactor();
    fd_event->setReactor(reactor);
    fd_event->setNonBlock();

    int n = g_sys_connect_fun(sockfd, addr, addrlen);
    if(n == 0) {
        LOG_DEBUG << "connect success";
        return n;
    } else if(errno != EINPROGRESS) {
        LOG_DEBUG << "connect error and errno is't EINPROGRESS, errno=" << errno <<  ",error=" << strerror(errno);
        return n;
    }

    toEpoll(fd_event, IOEvent::WRITE);

    Coroutine *cor = Coroutine::GetCurrentCoroutine();
    int max_connect_timeout = 75 * 1000;    // default 75s
    bool is_timeout = false;
    auto timeout_cb = [&is_timeout, cor]() {
        is_timeout = true;
        Coroutine::Resume(cor);
    };

    TimerEvent::ptr tevent = std::make_shared<TimerEvent>(max_connect_timeout, false, timeout_cb);
    Timer *timer = reactor->getTimer();
    timer->addTimerEvent(tevent);

    LOG_DEBUG << "connect func to yield";
    Coroutine::Yield();
    LOG_DEBUG << "connect func yield back, now to call sys connect";

    fd_event->delListenEvents(IOEvent::WRITE);
    fd_event->clearCoroutine();
    timer->delTimerEvent(tevent);

    n = g_sys_connect_fun(sockfd, addr, addrlen);
    if ((n < 0 && errno == EISCONN) || n == 0) {
		LOG_DEBUG << "connect succ";
		return 0;
	}

    if(is_timeout) {
        LOG_ERROR << "connect error,  timeout[ " << max_connect_timeout << "ms]";
        errno = ETIMEDOUT;
    }

    LOG_DEBUG << "connect error and errno = " << errno <<  ", error = " << strerror(errno);
    return -1;
}

ssize_t read_hook(int fd, void *buf, size_t count) {
    if(Coroutine::IsMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys read func";
        return g_sys_read_fun(fd, buf, count);
    }

    FdEvent::ptr fd_event = FdEventContainer::GetFdContainer()->getFdEvent(fd);
    fd_event->setReactor(Reactor::GetReactor());
    fd_event->setNonBlock();
    
    int n = g_sys_read_fun(fd, buf, count);
    if(n > 0) {
        return n;
    }

    toEpoll(fd_event, IOEvent::READ);

    LOG_DEBUG << "read func to yield";
    Coroutine::Yield();
    LOG_DEBUG << "read func yield back, now to call sys read";

    fd_event->delListenEvents(IOEvent::READ);
    fd_event->clearCoroutine();

    return g_sys_read_fun(fd, buf, count);
}

ssize_t write_hook(int fd, const void *buf, size_t count) {
    if(Coroutine::IsMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys write func";
        return g_sys_write_fun(fd, buf, count);
    }

    FdEvent::ptr fd_event = FdEventContainer::GetFdContainer()->getFdEvent(fd);
    fd_event->setReactor(Reactor::GetReactor());
    fd_event->setNonBlock();

    int n = g_sys_write_fun(fd, buf, count);
    if(n > 0) {
        return n;
    }

    toEpoll(fd_event, IOEvent::WRITE);

    LOG_DEBUG << "write func to yield";
    Coroutine::Yield();
    LOG_DEBUG << "write func yield back, now to call sys write";

    fd_event->delListenEvents(IOEvent::WRITE);
    fd_event->clearCoroutine();

    return g_sys_write_fun(fd, buf, count);
}

unsigned int sleep_hook(unsigned int seconds) {
    if(Coroutine::IsMainCoroutine()) {
        LOG_DEBUG << "hook disable, call sys sleep func";
        return g_sys_sleep_fun(seconds);
    }

    Coroutine *cor = Coroutine::GetCurrentCoroutine();
    bool is_timeout = false;
    auto timeout_cb = [&is_timeout, cor]() {
        is_timeout = true;
        Coroutine::Resume(cor);
    };

    TimerEvent::ptr event = std::make_shared<TimerEvent>(1000 * seconds, false, timeout_cb);
    Reactor::GetReactor()->getTimer()->addTimerEvent(event);

    LOG_DEBUG << "now to yield sleep";
    while(!is_timeout) {
        Coroutine::Yield();
    }
    LOG_DEBUG << "now resume sleep cor";

    return 0;
}

}   // namespace util


extern "C" {

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if(!util::g_hook) {
        return g_sys_accept_fun(sockfd, addr, addrlen);
    } else {
        return util::accept_hook(sockfd, addr, addrlen);
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if(!util::g_hook) {
        return g_sys_connect_fun(sockfd, addr, addrlen);
    } else {
        return util::connect_hook(sockfd, addr, addrlen);
    }
}

ssize_t read(int fd, void *buf, size_t count) {
    if(!util::g_hook) {
        return g_sys_read_fun(fd, buf, count);
    } else {
        return util::read_hook(fd, buf, count);
    }
}

ssize_t write(int fd, const void *buf, size_t count) {
    if(!util::g_hook) {
        return g_sys_write_fun(fd, buf, count);
    } else {
        return util::write_hook(fd, buf, count);
    }
}

unsigned int sleep(unsigned int seconds) {
    if(!util::g_hook) {
        return g_sys_sleep_fun(seconds);
    } else {
        return util::sleep_hook(seconds);
    }
}

}