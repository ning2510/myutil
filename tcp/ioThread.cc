#include <assert.h>

#include "log.h"
#include "ioThread.h"

namespace util {

static Reactor *t_reactor_ptr = nullptr;
static IOThread *t_current_io_thread = nullptr;

IOThread::IOThread()
    : m_index(-1),
      m_tid(-1),
      m_thread(-1),
      m_reactor(nullptr) {

    int rt = ::sem_init(&m_init_sem, 0, 0);
    assert(rt == 0);

    rt = ::sem_init(&m_start_sem, 0, 0);
    assert(rt == 0);

    ::pthread_create(&m_thread, nullptr, &IOThread::main, this);

    LOG_DEBUG << "semaphore begin to wait until new thread frinish IOThread::main() to init";

    rt = ::sem_wait(&m_init_sem);
    assert(rt == 0);
    LOG_DEBUG << "semaphore wait end, finish create io thread";

    ::sem_destroy(&m_init_sem);
}

IOThread::~IOThread() {
    m_reactor->stop();
    ::pthread_join(m_thread, nullptr);
    
    if(m_reactor) {
        delete m_reactor;
        m_reactor = nullptr;
    }
}

void *IOThread::main(void *arg) {
    t_reactor_ptr = new Reactor();
    assert(t_reactor_ptr != nullptr);

    IOThread *iothread = static_cast<IOThread *>(arg);
    t_current_io_thread = iothread;

    iothread->m_tid = gettid();
    iothread->m_reactor = t_reactor_ptr;
    iothread->m_reactor->setReactorType(SubReactor);

    Coroutine::GetCurrentCoroutine();

    LOG_DEBUG << "finish iothread init, now post semaphore";
    ::sem_post(&iothread->m_init_sem);

    int rt = ::sem_wait(&iothread->m_start_sem);
    assert(rt == 0);
    ::sem_destroy(&iothread->m_start_sem);

    LOG_DEBUG << "IOThread " << iothread->m_tid << " begin to loop";
    iothread->m_reactor->loop();

    return nullptr;
}

Reactor *IOThread::getReactor() {
    return t_reactor_ptr;
}

IOThread *IOThread::GetCurrentThread() {
    return t_current_io_thread;
}


IOThreadPool::IOThreadPool(int size) : m_size(size), m_index(-1) {
    for(int i = 0; i < size; i++) {
        IOThread::ptr tmp = std::make_shared<IOThread>();
        tmp->setThreadIndex(i);
        m_io_threads.push_back(tmp);
    }
}

void IOThreadPool::start() {
    for(int i = 0; i < m_size; i++) {
        int rt = ::sem_post(m_io_threads[i]->getStartSem());
        assert(rt == 0);
    }
}

IOThread *IOThreadPool::getIOThread() {
    if(m_index == -1 || m_index == m_size) {
        m_index = 0;
    }

    return m_io_threads[m_index++].get();
}


}   // namespace util