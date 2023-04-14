#ifndef _IOTHREAD_H
#define _IOTHREAD_H

#include <vector>
#include <atomic>
#include <memory>
#include <pthread.h>
#include <functional>
#include <semaphore.h>

#include "reactor.h"
#include "coroutine.h"
#include "timeWheel.h"
#include "tcpConnection.h"

namespace util {

class IOThread {
public:
    typedef std::shared_ptr<IOThread> ptr;

    IOThread();
    ~IOThread();

    void setThreadIndex(int index) { m_index = index; }
    int getThreadIndex() { return m_index; }

    pthread_t getPthreadId() { return m_tid; }
    sem_t *getStartSem() { return &m_start_sem; }

    Reactor *getReactor();
    static IOThread *GetCurrentThread();

private:
    static void *main(void *arg);

    int m_index;
    pid_t m_tid;
    pthread_t m_thread;

    Reactor *m_reactor;
    TimerEvent::ptr m_timer_event;

    sem_t m_init_sem;
    sem_t m_start_sem;
};

class IOThreadPool {
public:
    typedef std::shared_ptr<IOThreadPool> ptr;

    IOThreadPool(int size);

    IOThread *getIOThread();
    int getIOThreadPoolSize() { return m_size; }

    void start();

private:
   int m_size;
   std::atomic_int m_index;
   std::vector<IOThread::ptr> m_io_threads;
};

}   // namespace util

#endif
