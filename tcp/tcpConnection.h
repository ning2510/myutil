#ifndef _TCPCONNECTION_H
#define _TCPCONNECTION_H

#include <map>
#include <memory>
#include <functional>

#include "mutex.h"
#include "buffer.h"
#include "fdEvent.h"
#include "coroutine.h"
#include "netAddress.h"
#include "abstactSlot.h"
#include "abstractCodec.h"

namespace util {

class TcpServer;
class IOThread;

enum TcpConnectionState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
    Closed = 4
};

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    typedef std::shared_ptr<TcpConnection> ptr;

    enum ConnectionType {
        ServerConnection = 1,
        ClientConnection = 2
    };

    TcpConnection(TcpServer *tcp_server, IOThread *io_thread, int fd, 
                  int buff_size, NetAddress::ptr net_addr);
    ~TcpConnection();

    void initServer();
    void initBuffer(int size);

    void shutdownConnection();

    void registerToTimeWheel();

    void MainServerLoopCorFunc();
    void input();
    void execute();
    void output();

    void setState(const TcpConnectionState &state);
    TcpConnectionState getState();

    AbstractCodec::ptr getCodec() { return m_codec; }
    Coroutine::ptr getCoroutine() { return m_loop_cor; }
    Buffer *getReadBuffer() { return m_read_buffer.get(); }
    Buffer *getWriteBuffer() { return m_write_buffer.get(); }

    void setOverTimeFlag(bool value) { m_is_overtime = value; }
    bool getOverTimeFlag() { return m_is_overtime; }

private:
    void clearClient();

    int m_fd;
    bool m_stop;
    bool m_is_overtime;

    TcpServer *m_tcp_svr;
    IOThread *m_io_thread;
    Reactor *m_reactor;

    Buffer::ptr m_read_buffer;
    Buffer::ptr m_write_buffer;

    TcpConnectionState m_state;
    ConnectionType m_connection_type;

    AbstractCodec::ptr m_codec;
    NetAddress::ptr m_peer_addr;
    Coroutine::ptr m_loop_cor;
    FdEvent::ptr m_fd_event;

    std::weak_ptr<AbstractSlot<TcpConnection>> m_weak_slot;

    RWMutex m_mutex;

};

}   // namespace util

#endif
