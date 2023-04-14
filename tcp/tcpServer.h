#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include <map>
#include <memory>
#include <functional>

#include "reactor.h"
#include "ioThread.h"
#include "timeWheel.h"
#include "coroutine.h"
#include "netAddress.h"
#include "tcpConnection.h"

namespace util {

class TcpAcceptor {
public:
    typedef std::shared_ptr<TcpAcceptor> ptr;

    TcpAcceptor(NetAddress::ptr net_addr);
    ~TcpAcceptor();

    void init();
    int toAccept();

    NetAddress::ptr getLocalAddr() { return m_local_addr; }
    NetAddress::ptr getPeerAddr() { return m_peer_addr; }

private:
    int m_listenfd;
    int m_family;

    NetAddress::ptr m_local_addr;
    NetAddress::ptr m_peer_addr;
};

class TcpServer {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(NetAddress::ptr addr);
    ~TcpServer();

    void start();

    void addCoroutine(Coroutine::ptr cor);
    TcpConnection::ptr addClient(IOThread *io_thread, int fd);
    
    void freshTcpConnection(TimeWheel::TcpConnectionSlot::ptr slot);

    NetAddress::ptr getPeerAddr();
    NetAddress::ptr getLocalAddr();
    TimeWheel::ptr getTimeWheel();
    IOThreadPool::ptr getIOThreadPool();

private:
    void MainAcceptCorFunc();
    void ClearClientTimerFunc();

    int m_tcp_counts;
    bool m_is_stop_accept;
    
    Reactor *m_main_reactor;

    NetAddress::ptr m_addr;
    TcpAcceptor::ptr m_acceptor;
    Coroutine::ptr m_accept_cor;

    IOThreadPool::ptr m_io_pool;

    TimeWheel::ptr m_time_wheel;

    std::map<int, std::shared_ptr<TcpConnection>> m_clients;
    
    TimerEvent::ptr m_clear_client_event;
};

}   // namespace util

#endif
