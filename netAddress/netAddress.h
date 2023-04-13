#ifndef _NETADDRESS_H
#define _NETADDRESS_H

#include <string.h>
#include <memory>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace util {

class NetAddress {
public:
    typedef std::shared_ptr<NetAddress> ptr;

    virtual int getFamily() const = 0;
    virtual std::string toString() = 0;
    virtual socklen_t getSockLen() = 0;
    virtual sockaddr *getSockAddr() = 0;
};

class IPAddress : public NetAddress {
public:
    IPAddress(const std::string &ip, uint16_t port);
    IPAddress(const std::string &addr);
    IPAddress(uint16_t port);
    IPAddress(sockaddr_in addr);

    void init();

    int getFamily() const override;
    std::string toString() override;
    socklen_t getSockLen() override;
    sockaddr *getSockAddr() override;

    std::string getIP() const { return m_ip; }
    uint16_t getPort() const { return m_port; }

private:
    uint16_t m_port;
    std::string m_ip;
    sockaddr_in m_addr;
};

class UnixDomainAddress : NetAddress {
public:
    UnixDomainAddress(const std::string &path);
    UnixDomainAddress(sockaddr_un addr);

    int getFamily() const override;
    std::string toString() override;
    socklen_t getSockLen() override;
    sockaddr *getSockAddr() override;

    std::string getPath() const { return m_path; }

private:
    std::string m_path;
    sockaddr_un m_addr;
};

};  // namespace util

#endif
