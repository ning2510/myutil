#include "log.h"
#include "netAddress.h"

namespace util {

void IPAddress::init() {
    ::memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_port = ::htons(m_port);
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());

    LOG_DEBUG << "create ipv4 address succ [" << toString() << "]";
}

IPAddress::IPAddress(const std::string &ip, uint16_t port)
    : m_ip(ip), m_port(port) {

    init();
}

IPAddress::IPAddress(uint16_t port)
    : m_ip("0.0.0.0"), m_port(port) {

    init();
}

IPAddress::IPAddress(const std::string &addr) {
    size_t i = addr.find_first_of(":");
    if(i == addr.npos) {
        LOG_ERROR << "invalid addr[" << addr << "]";
        return ;
    }
    m_ip = addr.substr(0, i);
    m_port = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());

    init();
}

IPAddress::IPAddress(sockaddr_in addr) : m_addr(addr) {
    m_ip = std::string(inet_ntoa(m_addr.sin_addr));
    m_port = ntohs(m_addr.sin_port);

    LOG_DEBUG << "create ipv4 address succ [" << toString() << "]";
}

int IPAddress::getFamily() const {
    return m_addr.sin_family;
}

std::string IPAddress::toString() {
    std::stringstream ss;
    ss << m_ip << ":" << m_port;
    return ss.str();
}

socklen_t IPAddress::getSockLen() {
    return sizeof(m_addr);
}

sockaddr *IPAddress::getSockAddr() {
    return reinterpret_cast<sockaddr *>(&m_addr);
}


UnixDomainAddress::UnixDomainAddress(const std::string &path) : m_path(path) {
    ::memset(&m_addr, 0, sizeof(m_addr));
    ::unlink(m_path.c_str());
    m_addr.sun_family = AF_UNIX;
    strcpy(m_addr.sun_path, m_path.c_str());
}

UnixDomainAddress::UnixDomainAddress(sockaddr_un addr) : m_addr(addr) {
    m_path = addr.sun_path;
}

int UnixDomainAddress::getFamily() const {
    return m_addr.sun_family;
}

std::string UnixDomainAddress::toString() {
    return m_path;
}

socklen_t UnixDomainAddress::getSockLen() {
    return sizeof(m_addr);
}

sockaddr *UnixDomainAddress::getSockAddr() {
    return reinterpret_cast<sockaddr *>(&m_addr);
}

}   // namespace util