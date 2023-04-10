#ifndef _COROUTINEPOOL_H
#define _COROUTINEPOOL_H

#include "memory.h"
#include "coroutine.h"

namespace util {

class CoroutinePool {
public:
    
    CoroutinePool(int pool_size, int stack_size);
    ~CoroutinePool();

    Coroutine::ptr getCoroutineInstance();

    void backCoroutine(Coroutine::ptr cor);

private:
    int m_pool_size;
    int m_stack_size;

    Mutex m_mutex;
    
    std::vector<Memory::ptr> m_memory_pool;
    std::vector<std::pair<Coroutine::ptr, bool>> m_free_cors;
};

/* pool_size = 100      stack_size = 256 KB */
CoroutinePool *GetCoroutinePool(int pool_size = 100, int stack_size = 1024 * 256);

}   // namespace util

#endif
