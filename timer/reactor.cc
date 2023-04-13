#include "log.h"
#include "timer.h"
#include "reactor.h"
#include "coroutineHook.h"

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <sys/epoll.h>
#include <sys/eventfd.h>

extern read_fun_ptr_t g_sys_read_fun;
extern write_fun_ptr_t g_sys_write_fun;

namespace util {

static thread_local int t_max_epoll_timeout = 10000;
static thread_local Reactor *t_reactor_ptr = nullptr;
static CoroutineTaskQueue *t_coroutine_task_queue = nullptr;

Reactor *Reactor::GetReactor() {
    if(t_reactor_ptr == nullptr) {
        t_reactor_ptr = new Reactor();
    }
    return t_reactor_ptr;
}

Reactor::Reactor()
    : m_stop_flag(false),
      m_is_looping(false),
      is_init_timer(false),
      m_timer(nullptr) {

    if(t_reactor_ptr != nullptr) {
        LOG_ERROR << "this thread has already create a reactor";
        Exit(0);
    }

    m_tid = gettid();

    t_reactor_ptr = this;

    if((m_epfd = ::epoll_create(1)) <= 0) {
        LOG_ERROR << "start server error. epoll_create error, sys error=" << strerror(errno);
        Exit(0);
    }

    if((m_wake_fd = eventfd(0, EFD_NONBLOCK)) <= 0) {
        LOG_ERROR << "start server error. event_fd error, sys error=" << strerror(errno);
        Exit(0);
    }

    LOG_DEBUG << "m_epfd = " << m_epfd << ", m_wake_fd = " << m_wake_fd;
    
    addWakeupFd();
}

Reactor::~Reactor() {
    ::close(m_epfd);
    if(m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
    t_reactor_ptr = nullptr;
}

void Reactor::addEvent(int fd, epoll_event event, bool is_wakeup /*= true*/) {
    if(fd == -1) {
        LOG_ERROR << "Reactor::addEvent: fd is invalid";
        return ;
    }

    if(isInLoopThread()) {
        addEventInLoopThread(fd, event);
        return ;
    }

    {
        Mutex::Lock lock(m_mutex);
        m_pending_add_fds.insert(std::pair<int, epoll_event>(fd, event));
    }

    if(is_wakeup) wakeup();
}

void Reactor::delEvent(int fd, bool is_wakeup /*= true*/) {
    if(fd == -1) {
        LOG_ERROR << "Reactor::delEvent: fd is invalid";
        return ;
    }

    if(isInLoopThread()) {
        delEventInLoopThread(fd);
        return ;
    }

    {
        Mutex::Lock lock(m_mutex);
        m_pending_del_fds.push_back(fd);
    }

    if(is_wakeup) wakeup();
}

void Reactor::addTask(std::function<void()> task, bool is_wakeup /*= true*/) {
    {
        Mutex::Lock lock(m_mutex);
        m_pending_tasks.push_back(task);
    }

    if(is_wakeup) wakeup();
}

void Reactor::addTask(std::vector<std::function<void()>> task, bool is_wakeup /*= true*/) {
    {
        Mutex::Lock lock(m_mutex);
        m_pending_tasks.insert(m_pending_tasks.begin(), task.begin(), task.end());
    }

    if(is_wakeup) wakeup();
}

void Reactor::addCoroutine(Coroutine::ptr cor, bool is_wakeup /*= true*/) {
    auto func = [cor]() {
        Coroutine::Resume(cor.get());
    };

    addTask(func);
}

void Reactor::wakeup() {
    if(!m_is_looping) {
        return ;
    }

    uint64_t tmp = 1;
    if(g_sys_write_fun(m_wake_fd, &tmp, 8) != 8) {
        LOG_ERROR << "write wakeupfd[" << m_wake_fd << "] error";
    }
}

void Reactor::loop() {
    LOG_DEBUG << "reactor loop start";
    assert(isInLoopThread());
    assert(!m_is_looping);

    m_is_looping = true;
    m_stop_flag = false;
    
    Coroutine *fir_cor = nullptr;
    while(!m_stop_flag) {
        const int MAX_EVENTS = 10;
        epoll_event re_events[MAX_EVENTS];

        if(fir_cor) {
            Coroutine::Resume(fir_cor);
            fir_cor = nullptr;
        }

        if(m_reactor_type != MainReactor) {
            FdEvent *ptr = nullptr;
            while(1) {
                ptr = CoroutineTaskQueue::GetCoroutineTaskQueue()->pop();
                if(ptr) {
                    ptr->setReactor(this);
                    Coroutine::Resume(ptr->getCoroutine()); 
                } else {
                    break;
                }
            }
        }

        Mutex::Lock lock(m_mutex);
        std::vector<std::function<void()>> tmp_tasks;
        tmp_tasks.swap(m_pending_tasks);
        lock.unlock();

        for(size_t i = 0; i < tmp_tasks.size(); i++) {
            if(tmp_tasks[i]) tmp_tasks[i]();
        }

        int rt = ::epoll_wait(m_epfd, re_events, MAX_EVENTS, t_max_epoll_timeout);
        LOG_INFO << "epoll_wait rt = " << rt;
        if(rt < 0) {
            LOG_ERROR << "epoll_wait error, skip, errno=" << strerror(errno);
        } else {
            for(int i = 0; i < rt; i++) {
                epoll_event event = re_events[i];
                if(event.data.fd == m_wake_fd && (event.events & READ)) {
                    LOG_DEBUG << "epoll wake up, fd=[" << m_wake_fd << "]";
                    char buf[8];
                    while(1) {
                        if((g_sys_read_fun(m_wake_fd, buf, 8) == -1) && errno == EAGAIN) {
                            break;
                        }
                    }
                } else {
                    FdEvent *ptr = (FdEvent *)event.data.ptr;
                    if(ptr != nullptr) {
                        int fd = ptr->getFd();
                        if(!(event.events & EPOLLIN && !(event.events & EPOLLOUT))) {
                            LOG_ERROR << "socket [" << fd << "] occur other unknow event:[" << event.events << "], need unregister this socket";
                            delEventInLoopThread(fd);
                        } else {
                            if(ptr->getCoroutine()) {
                                if(!fir_cor) {
                                    LOG_DEBUG << "fir_cor is null, so fir_cor execute";
                                    fir_cor = ptr->getCoroutine();
                                    continue;
                                }

                                if(m_reactor_type == SubReactor) {
                                    LOG_DEBUG << "reactor type is SubReactor";
                                    delEventInLoopThread(fd);
                                    ptr->setReactor(nullptr);
                                    CoroutineTaskQueue::GetCoroutineTaskQueue()->push(ptr);
                                } else {
                                    LOG_DEBUG << "reactor type is MainReactor";
                                    Coroutine::Resume(ptr->getCoroutine());
                                    if(fir_cor) {
                                        fir_cor = nullptr;
                                    }
                                }

                            } else {
                                LOG_DEBUG << "epoll timer event";
                                std::function<void()> read_callback;
                                std::function<void()> write_callback;
                                read_callback = ptr->getCallBack(READ);
                                write_callback = ptr->getCallBack(WRITE);
                                LOG_DEBUG << "fd = " << fd << ", m_timer_fd = " << m_timer_fd << ", now time = " << getNowMs();
                                if(fd == m_timer_fd) {
                                    read_callback();
                                    continue;
                                }

                                if (event.events & EPOLLIN) {
                                    Mutex::Lock lock(m_mutex);
                                    m_pending_tasks.push_back(read_callback);
                                    lock.unlock();					
                                }
                                if (event.events & EPOLLOUT) {
                                    Mutex::Lock lock(m_mutex);
                                    m_pending_tasks.push_back(write_callback);
                                    lock.unlock();						
                                }
                            }
                        }
                    } 
                }
            }
        }

        std::map<int, epoll_event> tmp_add;
        std::vector<int> tmp_del;

        {
            Mutex::Lock lock(m_mutex);
            tmp_add.swap(m_pending_add_fds);
            tmp_del.swap(m_pending_del_fds);
        }

        for(auto it = tmp_add.begin(); it != tmp_add.end(); it++) {
            addEventInLoopThread(it->first, it->second);
        }
        for(auto it = tmp_del.begin(); it != tmp_del.end(); it++) {
            delEventInLoopThread(*it);
        }
    }

    LOG_DEBUG << "reactor loop end";
    m_is_looping = false;
}

void Reactor::stop() {
    if(!m_stop_flag && m_is_looping) {
        m_stop_flag = true;
        wakeup();
    }
}

Timer *Reactor::getTimer() {
    if(m_timer == nullptr) {
        m_timer = new Timer(this);
        m_timer_fd = m_timer->getFd();
    }
    return m_timer;
}

pid_t Reactor::getTid() {
    return m_tid;
}

void Reactor::setReactorType(ReactorType type) {
    m_reactor_type = type;
}

void Reactor::addWakeupFd() {
    int op = EPOLL_CTL_ADD;
    epoll_event event;
    event.data.fd = m_wake_fd;
    event.events = EPOLLIN;

    if(::epoll_ctl(m_epfd, op, m_wake_fd, &event) != 0) {
        LOG_ERROR << "epoo_ctl error, fd[" << m_wake_fd << "], errno=" << errno << ", err=" << strerror(errno);
    }

    m_fds.push_back(m_wake_fd);
}

bool Reactor::isInLoopThread() const {
    return m_tid == gettid();
}

void Reactor::addEventInLoopThread(int fd, epoll_event event) {
    assert(isInLoopThread());

    int op = EPOLL_CTL_ADD;
    int isExist = false;

    auto it = std::find(m_fds.begin(), m_fds.end(), fd);
    if(it != m_fds.end()) {
        isExist = true;
        op = EPOLL_CTL_MOD;
    }

    if(::epoll_ctl(m_epfd, op, fd, &event) != 0) {
        LOG_ERROR << "epoll_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno);
    }

    if(!isExist) {
        m_fds.push_back(fd);
    }

    LOG_DEBUG << "epoll_ctl add succ, fd[" << fd << "]"; 
}

void Reactor::delEventInLoopThread(int fd) {
    assert(isInLoopThread());

    int op = EPOLL_CTL_DEL;
    auto it = std::find(m_fds.begin(), m_fds.end(), fd);
    if(it == m_fds.end()) {
        LOG_ERROR << "fd[" << fd << "] not in this loop";
        return ;
    }

    if(::epoll_ctl(m_epfd, op, fd, nullptr) != 0) {
        LOG_ERROR << "epoo_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno);
    }

    m_fds.erase(it);

    LOG_DEBUG << "del succ, fd[" << fd << "]"; 
}


CoroutineTaskQueue *CoroutineTaskQueue::GetCoroutineTaskQueue() {
    if(t_coroutine_task_queue == nullptr) {
        t_coroutine_task_queue = new CoroutineTaskQueue();
    }
    return t_coroutine_task_queue;
}

void CoroutineTaskQueue::push(FdEvent *fd) {
    Mutex::Lock lock(m_mutex);
    m_task.push(fd);
    lock.unlock();
}

FdEvent *CoroutineTaskQueue::pop() {
    FdEvent *it = nullptr;

    Mutex::Lock lock(m_mutex);
    if(!m_task.empty()) {
        it = m_task.front();
        m_task.pop();
    }
    lock.unlock();

    return it;
}


}   // namespace util