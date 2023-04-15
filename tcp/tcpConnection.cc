#include "log.h"
#include "ioThread.h"
#include "tcpServer.h"
#include "httpRequest.h"
#include "tcpConnection.h"
#include "coroutineHook.h"
#include "coroutinePool.h"

namespace util {

TcpConnection::TcpConnection(TcpServer *tcp_server, IOThread *io_thread, 
                             int fd, int buff_size, NetAddress::ptr net_addr)
                            : m_fd(fd),
                              m_stop(false),
                              m_is_overtime(false),
                              m_tcp_svr(tcp_server),
                              m_io_thread(io_thread),
                              m_reactor(nullptr),
                              m_peer_addr(net_addr)  {
    
    m_reactor = m_io_thread->getReactor();
    m_fd_event = FdEventContainer::GetFdContainer()->getFdEvent(fd);
    m_fd_event->setReactor(m_reactor);
    m_codec = m_tcp_svr->getCodec();

    initBuffer(buff_size);
    m_loop_cor = GetCoroutinePool()->getCoroutineInstance();
    m_state = Connected;

    LOG_DEBUG << "succ create tcp connection state[" << m_state << "], fd = " << fd;
}

TcpConnection::~TcpConnection() {
    if(m_connection_type == ServerConnection) {
        GetCoroutinePool()->backCoroutine(m_loop_cor);
    }
}

void TcpConnection::initServer() {
    registerToTimeWheel();
    m_loop_cor->setCallback(std::bind(&TcpConnection::MainServerLoopCorFunc, this));
    m_reactor->addCoroutine(m_loop_cor);
}

void TcpConnection::initBuffer(int size) {
    m_read_buffer = std::make_shared<Buffer>(size);
    m_write_buffer = std::make_shared<Buffer>(size);
}

void TcpConnection::shutdownConnection() {
    LOG_INFO << "TcpConnection shutdown, io thread id = " << m_io_thread->getPthreadId();
    TcpConnectionState state = getState();
    if(state == Closed || state == NotConnected) {
        LOG_DEBUG << "this client has closed";
        return ;
    }

    setState(HalfClosing);
    LOG_INFO << "shutdown conn[" << m_peer_addr->toString() << "], fd = " << m_fd;

    ::shutdown(m_fd, SHUT_RDWR);
}

void TcpConnection::registerToTimeWheel() {
    auto cb = [](TcpConnection::ptr conn) {
        conn->shutdownConnection();
    };

    TimeWheel::TcpConnectionSlot::ptr tmp = std::make_shared<AbstractSlot<TcpConnection>>(shared_from_this(), cb);
    m_weak_slot = tmp;
    m_tcp_svr->freshTcpConnection(tmp);
}

void TcpConnection::MainServerLoopCorFunc() {
    while(!m_stop) {
        LOG_DEBUG << "TcpConnection::MainServerLoopCorFunc input()";
        input();
        LOG_DEBUG << "TcpConnection::MainServerLoopCorFunc execute()";
        execute();
        LOG_DEBUG << "TcpConnection::MainServerLoopCorFunc output()";
        output();
        LOG_DEBUG << "TcpConnection::MainServerLoopCorFunc end";
    }
}

void TcpConnection::input() {
    if(m_is_overtime) {
        LOG_INFO << "over time, skip input progress";
        return ;
    }

    TcpConnectionState state = getState();
    if(state == Closed || state == NotConnected) {
        return ;
    }

    bool read_all = false;
    bool close_flag = false;
    int count = 0;

    while(!read_all) {
        int read_count = m_read_buffer->writeableBytes();
        char *write_index = m_read_buffer->peek();

        int rt = read_hook(m_fd, write_index, read_count);
        LOG_DEBUG << "read hook rt = " << rt;
        if(rt > 0) {
            m_read_buffer->hasWritten(rt);
        }
        LOG_DEBUG << "m_read_buffer size = " << m_read_buffer->getBufferSize()
                  << ", prepend size = " << m_read_buffer->prependableBytes();
    
        count += rt;
        if(m_is_overtime) {
            LOG_INFO << "over timer, now break";
            break;
        }

        if(rt <= 0) {
            if(rt < 0) 
                LOG_ERROR << "read empty while occur read event, because of peer close, fd = " << m_fd << ", sys error=" << strerror(errno) << ", now to clear tcp connection";
            close_flag = true;
            break;
        } else {
            if(rt == read_count) {
                // read again
                continue;
            } else if(rt < read_count) {
                read_all = true;
                break;
            }
        }
    }

    if(close_flag) {
        clearClient();
        LOG_DEBUG << "peer close, now yield current coroutine, wait main thread clear this TcpConnection";
        Coroutine::GetCurrentCoroutine()->setCanResume(false);
        Coroutine::Yield();
    }

    if(m_is_overtime) return ;

    if(!read_all) {
        LOG_ERROR << "not read all data in socket buffer";
    }
    LOG_INFO << "recv [" << count << "] bytes data from [" << m_peer_addr->toString() << "], fd [" << m_fd << "]";
    if(m_connection_type == ServerConnection) {
        TimeWheel::TcpConnectionSlot::ptr tmp = m_weak_slot.lock();
        if(tmp) {
            m_tcp_svr->freshTcpConnection(tmp);
        }
    }
}

void TcpConnection::execute() {
    /* Echo server */
    if(m_tcp_svr->getProtocalType() == TCP) {
        std::string msg = m_read_buffer->retrieveAllAsString();
        LOG_DEBUG << "[TCP Protocal] fd[" << m_fd << "] recv msg = " << msg;
        int rt = write_hook(m_fd, msg.c_str(), msg.size());
        if(rt <= 0) {
            LOG_ERROR << "write_hook error, err = " << strerror(errno);
        }
        return ;
    }

    while(m_read_buffer->readableBytes() > 0) {
        std::shared_ptr<AbstractData> data;
        if(m_tcp_svr->getProtocalType() == HTTP) {
            data = std::make_shared<HttpRequest>();
        }

        m_codec->decode(m_read_buffer.get(), data.get());
        if(!data->decode_succ) {
            LOG_ERROR << "it parse request error of fd = " << m_fd;
            break;
        }

        if(m_connection_type == ServerConnection) {
            m_tcp_svr->getDispatcher()->dispatch(data.get(), this);
        }
    }
}

void TcpConnection::output() {
    if(m_is_overtime) {
        LOG_INFO << "over time, skip output progress";
        return ;
    }

    while(true) {
        TcpConnectionState state = getState();
        if(state != Connected) {
            break;
        }

        if(m_write_buffer->readableBytes() == 0) {
            LOG_DEBUG << "write_buffer of fd[" << m_fd << "] no data to write, to yiled this coroutine";
            break;
        }

        int total_size = m_write_buffer->readableBytes();
        char *read_index = m_write_buffer->peek();
        int rt = write_hook(m_fd, read_index, total_size);
        if(rt <= 0) {
            LOG_ERROR << "write empey, error = " << strerror(errno);
        }

        m_write_buffer->retrieve(rt);
        LOG_INFO << "send [" << rt << "] bytes data to [" << m_peer_addr->toString() << "], fd [" << m_fd << "]";
        if(m_write_buffer->readableBytes() <= 0) {
            LOG_DEBUG << "send all data, now unregister write event and break";
            break;
        }

        if(m_is_overtime) {
            LOG_INFO << "over timer, now break write function";
            break;
        }
    }
}

void TcpConnection::setState(const TcpConnectionState &state) { 
    RWMutex::WriteLock lock(m_mutex);
    m_state = state; 
    lock.unlock();
}

TcpConnectionState TcpConnection::getState() { 
    TcpConnectionState state;
    RWMutex::ReadLock lock(m_mutex);
    state = m_state;
    lock.unlock();
    return state;
}

void TcpConnection::clearClient() {
    if(getState() == Closed) {
        LOG_DEBUG << "this client has closed";
        return ;
    }

    m_fd_event->unregisterFromReactor();
    
    m_stop = true;
    ::close(m_fd_event->getFd());
    setState(Closed);
}

}   // namespace util