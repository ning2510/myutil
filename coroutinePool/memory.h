#ifndef _MEMORY_H
#define _MEMORY_H

#include <memory>
#include <atomic>
#include <vector>

#include "mutex.h"

namespace util {

class Memory {
public:
    typedef std::shared_ptr<Memory> ptr;

    Memory(int block_size, int block_count);
    ~Memory();

    char *getBlock();
    bool hasBlock(const char *);
    void backBlock(const char *s);

    const char *getStart() const {
        return m_start;
    }

    const char *getEnd() const {
        return m_end;
    }

    int getRefCount() {
        return m_ref_cnt;
    }

private:
    int m_size;
    int m_block_count;
    int m_block_size;

    char *m_start;
    char *m_end;

    std::atomic_int m_ref_cnt;
    std::vector<bool> m_blocks;

    Mutex m_mutex;
};

}   // namespace util

#endif
