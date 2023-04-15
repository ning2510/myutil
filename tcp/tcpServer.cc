#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "log.h"
#include "tcpServer.h"
#include "httpCodec.h"
#include "coroutinePool.h"
#include "coroutineHook.h"
#include "httpDispatcher.h"

namespace util {

TcpAcceptor::TcpAcceptor(NetAddress::ptr net_addr) : m_listenfd(-1), m_local_addr(net_addr) {
    m_family = net_addr->getFamily();
}

TcpAcceptor::~TcpAcceptor() {
    if(m_listenfd != -1) {
        FdEvent::ptr fd_event = FdEventContainer::GetFdContainer()->getFdEvent(m_listenfd);
        fd_event->unregisterFromReactor();
        ::close(m_listenfd);
    }
}

void TcpAcceptor::init() {
    m_listenfd = ::socket(m_family, SOCK_STREAM, 0);
    if(m_listenfd < 0) {
        LOG_ERROR << "TcpAcceptor::init - socket error, error = " << strerror(errno);
        Exit(0);
    }
    LOG_DEBUG << "create listenfd success, listenfd = " << m_listenfd;

    int val = 1;
    if(::setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        LOG_ERROR << "TcpServer setsockopt error, error = " << strerror(errno);
    }

    socklen_t len = m_local_addr->getSockLen();
    if(::bind(m_listenfd, (sockaddr *)m_local_addr->getSockAddr(), len) != 0) {
        LOG_ERROR << "TcpServer bind error, error = " << strerror(errno);
    }

    if(::listen(m_listenfd, 10) != 0) {
        LOG_ERROR << "TcpServer listen error, error = " << strerror(errno);
        Exit(0);
    }
}

int TcpAcceptor::toAccept() {
    int acceptfd = 0;

    if(m_family == AF_INET) {
        sockaddr_in clientAddr;
        ::memset(&clientAddr, 0, sizeof(clientAddr));
        socklen_t len = sizeof(clientAddr);
        
        acceptfd = accept_hook(m_listenfd, reinterpret_cast<sockaddr *>(&clientAddr), &len);
        if(acceptfd == -1) {
            LOG_DEBUG << "TcpAcceptor::toAccept - accept_hook error, error = " << strerror(errno);
            return -1;
        }

        m_peer_addr = std::make_shared<IPAddress>(clientAddr);
    } else if(m_family == AF_UNIX) {
        sockaddr_un clientAddr;
        ::memset(&clientAddr, 0, sizeof(clientAddr));
        socklen_t len = sizeof(clientAddr);

        acceptfd = accept_hook(m_listenfd, reinterpret_cast<sockaddr *>(&clientAddr), &len);
        if(acceptfd == -1) {
            LOG_DEBUG << "TcpAcceptor::toAccept - accept_hook error, error = " << strerror(errno);
            return -1;
        }

        m_peer_addr = std::make_shared<UnixDomainAddress>(clientAddr);
    } else {
        LOG_ERROR << "TcpAcceptor::toAccept - invalid m_family";
        ::close(m_listenfd);
        return -1;
    }

    LOG_INFO << "New client accepted success, acceptfd = " << acceptfd;
    return acceptfd;
}


TcpServer::TcpServer(NetAddress::ptr addr, ProtocalType type /*= HTTP*/)
    : m_tcp_counts(0),
      m_is_stop_accept(false),
      m_main_reactor(nullptr),
      m_addr(addr) {

    if(type == HTTP) {
        m_dispatcher = std::make_shared<HttpDispatcher>();
        m_codec = std::make_shared<HttpCodec>();
        m_protocal_type = HTTP;
    } else if(type == TCP) {
        m_protocal_type = TCP;
    }

    m_main_reactor = Reactor::GetReactor();
    m_main_reactor->setReactorType(MainReactor);

    m_io_pool = std::make_shared<IOThreadPool>(10);

    m_time_wheel = std::make_shared<TimeWheel>(m_main_reactor, 10, 10);
    m_clear_client_event = std::make_shared<TimerEvent>(10000, true, std::bind(&TcpServer::ClearClientTimerFunc, this));
    m_main_reactor->getTimer()->addTimerEvent(m_clear_client_event);
}

TcpServer::~TcpServer() {
    GetCoroutinePool()->backCoroutine(m_accept_cor);
}

void TcpServer::start() {
    m_acceptor.reset(new TcpAcceptor(m_addr));
    m_acceptor->init();

    m_accept_cor = GetCoroutinePool()->getCoroutineInstance();
    m_accept_cor->setCallback(std::bind(&TcpServer::MainAcceptCorFunc, this));

    LOG_INFO << "TcpServer::start - resume accept coroutine";
    Coroutine::Resume(m_accept_cor.get());

    m_io_pool->start();
    m_main_reactor->loop();
}

void TcpServer::MainAcceptCorFunc() {
    while(!m_is_stop_accept) {
        int acceptfd = m_acceptor->toAccept();
        if(acceptfd == -1) {
            LOG_DEBUG << "TcpServer::MainAcceptCorFunc - acceptfd = -1";
            Coroutine::Yield();
            continue;
        }
        LOG_INFO << "new client, accept fd = " << acceptfd;

        IOThread *io_thread = m_io_pool->getIOThread();
        TcpConnection::ptr conn = addClient(io_thread, acceptfd);
        conn->initServer();
        m_tcp_counts++;

        LOG_INFO << "new TcpConnection create success, io thread id = " << io_thread->getPthreadId()
                 << ", io thread index = " << io_thread->getThreadIndex() << ", now tcp_counts = " << m_tcp_counts;
    }
}

TcpConnection::ptr TcpServer::addClient(IOThread *io_thread, int fd) {
    auto it = m_clients.find(fd);
    if(it != m_clients.end()) {
        it->second.reset();
        it->second = std::make_shared<TcpConnection>(this, io_thread, fd, 1024, getPeerAddr());
        return it->second;
    } else {
        TcpConnection::ptr conn = std::make_shared<TcpConnection>(this, io_thread, fd, 1024, getPeerAddr());
        m_clients.insert(std::make_pair(fd, conn));
        return conn;
    }
}

void TcpServer::freshTcpConnection(TimeWheel::TcpConnectionSlot::ptr slot) {
    auto cb = [slot, this]() mutable {
        this->getTimeWheel()->fresh(slot);
        slot.reset();
    };
    m_main_reactor->addTask(cb);
}

void TcpServer::ClearClientTimerFunc() {
    for(auto &cli : m_clients) {
        if(cli.second && cli.second.use_count() > 0 && cli.second->getState() == Closed) {
            LOG_DEBUG << "TcpConection [fd:" << cli.first << "] will delete, state=" << cli.second->getState();
            cli.second.reset();
        }
    }
}

void TcpServer::addCoroutine(Coroutine::ptr cor) {
    m_main_reactor->addCoroutine(cor);
}

NetAddress::ptr TcpServer::getPeerAddr() {
    return m_acceptor->getPeerAddr();
}

NetAddress::ptr TcpServer::getLocalAddr() {
    return m_addr;
}

TimeWheel::ptr TcpServer::getTimeWheel() {
    return m_time_wheel;
}

IOThreadPool::ptr TcpServer::getIOThreadPool() {
    return m_io_pool;
}

ProtocalType TcpServer::getProtocalType() {
    return m_protocal_type;
}

AbstractCodec::ptr TcpServer::getCodec() {
    return m_codec;
}

AbstractDispatcher::ptr TcpServer::getDispatcher() {
    return m_dispatcher;
}

}   // namespace util