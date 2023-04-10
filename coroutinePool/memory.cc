#include "memory.h"

#include <assert.h>

namespace util {

Memory::Memory(int block_size, int block_count)
    : m_block_count(block_count),
      m_block_size(block_size),
      m_ref_cnt(0) {

    m_size = m_block_count * m_block_size;
    m_start = (char *)malloc(m_size);
    assert(m_start);
    m_end = m_start + m_size - 1;

    m_blocks.resize(m_block_count);
    for(int i = 0; i < m_block_count; i++) {
        m_blocks[i] = false;
    }
}

Memory::~Memory() {
    std::cout << "~Memory" << std::endl;
    if(!m_start || m_start == (void *)-1) {
        return ;
    }
    free(m_start);

    m_start = nullptr;
    m_end = nullptr;
}

char *Memory::getBlock() {
    char *rt = nullptr;

    for(int i = 0; i < m_block_count; i++) {
        if(!m_blocks[i]) {
            m_blocks[i] = true;
            rt = m_start + i * m_block_size;
            break;
        }
    }

    return rt;
}

bool Memory::hasBlock(const char *s) {
    return ((s >= m_start) && (s <= m_end));
}

void Memory::backBlock(const char *s) {
    if(s < m_start || s > m_end) {
        return ;
    }

    int idx = (s - m_start) / m_block_size;
    Mutex::Lock lock(m_mutex);
    m_blocks[idx] = false;
    lock.unlock();
    m_ref_cnt--;
}

}   // namespace util