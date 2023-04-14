#ifndef _BUFFER_H
#define _BUFFER_H

#include <vector>
#include <memory>
#include <string.h>
#include <arpa/inet.h>

namespace util {

class Buffer {
public:
    typedef std::shared_ptr<Buffer> ptr;

    /* 默认空闲区域大小 */
    static const size_t kCheapPrepend = 8;
    /* 可读 + 可写区域大小 */
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initailSize = kInitialSize)
        : m_buffer(kCheapPrepend + initailSize),
          m_read_index(kCheapPrepend),
          m_write_index(kCheapPrepend) {}

    size_t readableBytes() const {
        return m_write_index - m_read_index;
    }

    size_t writeableBytes() const {
        return m_buffer.size() - m_write_index;
    }

    size_t prependableBytes() const {
        return m_read_index;
    }

    const char *peek() const {
        return begin() + m_read_index;
    }

    int32_t peekInt32() const {
        if(readableBytes() >= sizeof(int32_t)) {
            int32_t nw32 = 0;
            ::memcpy(&nw32, peek(), sizeof(nw32));
            int32_t host32 = ::ntohl(nw32);
            return host32;
        }
    }

    void hasWritten(size_t len) {
        if(len <= writeableBytes()) {
            m_write_index += len;
        }
    }

    void retrieve(size_t len) {
        if(len < readableBytes()) {
            m_read_index += len;
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        m_read_index = m_write_index = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWriteableBytes(size_t len) {
        if(writeableBytes() < len) {
            makeSpace(len);
        }
    }

    void append(const char *data, size_t len) {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        m_write_index += len;
    }

    void append(const void *data, size_t len) {
        append(static_cast<const char *>(data), len);
    }
    
    void appendInt32(int32_t x) {
        int32_t nw32 = ::htonl(x);
        append(&nw32, sizeof(nw32));
    }

    void prepend(const void *data, size_t len) {
        if(len <= prependableBytes()) {
            m_read_index -= len;
            const char *d = (const char *)data;
            std::copy(d, d + len, begin() + m_read_index);
        }
    }

    char *beginWrite() {
        return begin() + m_write_index;
    }

    const char *beginWrite() const {
        return begin() + m_write_index;
    }

private:
    char *begin() {
        return &*m_buffer.begin();
    }

    const char *begin() const {
        return &*m_buffer.begin();
    }

    /**
     * kCheapPrepend 是空闲字节（默认是 8），reader 是读指针，writer 是写指针
     * kCheapPrepend | reader | writer |
     * kCheapPrepend |         len         |
    */
    void makeSpace(size_t len) {
        // 可写大小 + 空闲大小 < 要写大小 + 空闲大小(默认 8 字节)
        if(writeableBytes() + prependableBytes() < len + kCheapPrepend) {
            m_buffer.resize(m_write_index + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + m_read_index, begin() + m_write_index, begin() + kCheapPrepend);
            m_read_index = kCheapPrepend;
            m_write_index = m_read_index + readable;
        }
    }

    size_t m_read_index;
    size_t m_write_index;
    std::vector<char> m_buffer;
};

}   // namespace util

#endif
