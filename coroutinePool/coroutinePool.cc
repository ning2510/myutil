#include "coroutinePool.h"

namespace util {

static CoroutinePool *t_coroutine_container_ptr = nullptr;

CoroutinePool *GetCoroutinePool(int pool_size /*= 100*/, int stack_size/*= 1024 * 256*/) {
    if(!t_coroutine_container_ptr) {
        t_coroutine_container_ptr = new CoroutinePool(pool_size, stack_size);
    }
    return t_coroutine_container_ptr;
}

CoroutinePool::CoroutinePool(int pool_size, int stack_size)
    : m_pool_size(pool_size),
      m_stack_size(stack_size) {

    Coroutine::GetCurrentCoroutine();

    Memory::ptr fir_memory_pool = std::make_shared<Memory>(stack_size, pool_size);
    m_memory_pool.push_back(fir_memory_pool);

    for(int i = 0; i < pool_size; i++) {
        Coroutine::ptr cor_tmp = std::make_shared<Coroutine>(stack_size, fir_memory_pool->getBlock());
        cor_tmp->setIndex(i);
        m_free_cors.emplace_back(std::make_pair(cor_tmp, false));
    }

}

CoroutinePool::~CoroutinePool() {
    std::cout << "~CoroutinePool" << std::endl;
    for(auto m : m_memory_pool) {
        m.reset();
    }

    for(auto c : m_free_cors) {
        c.first.reset();
    }
}

Coroutine::ptr CoroutinePool::getCoroutineInstance() {
    Mutex::Lock lock(m_mutex);
    for(int i = 0; i < m_pool_size; i++) {
        if(!m_free_cors[i].first->getIsInCoFunc() && !m_free_cors[i].second) {
            m_free_cors[i].second = true;
            Coroutine::ptr cor = m_free_cors[i].first;
            lock.unlock();
            return cor;
        }
    }

    for(size_t i = 1; i < m_memory_pool.size(); i++) {
        char *tmp = m_memory_pool[i]->getBlock();
        if(tmp) {
            Coroutine::ptr cor = std::make_shared<Coroutine>(m_stack_size, tmp);
            lock.unlock();
            return cor;
        }
    }

    m_memory_pool.push_back(std::make_shared<Memory>(m_stack_size, m_pool_size));
    return std::make_shared<Coroutine>(m_stack_size, m_memory_pool[m_memory_pool.size() - 1]->getBlock());
}

void CoroutinePool::backCoroutine(Coroutine::ptr cor) {
    int idx = cor->getIndex();
    if(idx >= 0 && idx < m_pool_size) {
        m_free_cors[idx].second = false;
        return ;
    }

    for(int i = 1; i < m_memory_pool.size(); i++) {
        if(m_memory_pool[i]->hasBlock(cor->getStackPtr())) {
            m_memory_pool[i]->backBlock(cor->getStackPtr());
        }
    }
}


}   // namespace util