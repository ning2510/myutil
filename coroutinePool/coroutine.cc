#include "coroutine.h"
#include "log.h"

#include <atomic>
#include <string.h>
#include <assert.h>

namespace util {

// // main coroutine, every io thread have a main_coroutine
// static thread_local Coroutine* t_main_coroutine = NULL;

// // current thread is runing which coroutine
// static thread_local Coroutine* t_cur_coroutine = NULL;

// // static thread_local bool t_enable_coroutine_swap = true;

// static std::atomic_int t_coroutine_count {0};

// static std::atomic_int t_cur_coroutine_id {1};

// int getCoroutineIndex() {
//   return t_cur_coroutine_id;
// }

// void CoFunction(Coroutine* co) {

//   if (co!= nullptr) {
//     co->setIsInCoFunc(true);
//     co->m_callback();
//     co->setIsInCoFunc(false);
//   }

//   Coroutine::Yield();
// }


// Coroutine::Coroutine() {
//   m_cor_id = 0;
//   t_coroutine_count++;
//   memset(&m_coctx, 0, sizeof(m_coctx));
//   t_cur_coroutine = this;
// }

// Coroutine::Coroutine(int size, char* stack_ptr) : m_stack_size(size), m_stack_sp(stack_ptr) {
//   assert(stack_ptr);

//   if (!t_main_coroutine) {
//     t_main_coroutine = new Coroutine();
//   }

//   m_cor_id = t_cur_coroutine_id++;
//   t_coroutine_count++;
// }

// Coroutine::Coroutine(int size, char* stack_ptr, std::function<void()> cb)
//   : m_stack_size(size), m_stack_sp(stack_ptr) {

//   assert(m_stack_sp);
  
//   if (!t_main_coroutine) {
//     t_main_coroutine = new Coroutine();
//   }

//   setCallback(cb);
//   m_cor_id = t_cur_coroutine_id++;
//   t_coroutine_count++;
// }

// bool Coroutine::setCallback(std::function<void()> cb) {

//   if (this == t_main_coroutine) {
//     return false;
//   }
//   if (m_is_in_cofunc) {
//     return false;
//   }

//   m_callback = cb;

//   char* top = m_stack_sp + m_stack_size;

//   top = reinterpret_cast<char*>((reinterpret_cast<unsigned long>(top)) & -16LL);

//   memset(&m_coctx, 0, sizeof(m_coctx));

//   m_coctx.regs[kRSP] = top;
//   m_coctx.regs[kRBP] = top;
//   m_coctx.regs[kRETAddr] = reinterpret_cast<char*>(CoFunction); 
//   m_coctx.regs[kRDI] = reinterpret_cast<char*>(this);

//   m_can_resume = true;

//   return true;

// }

// Coroutine::~Coroutine() {
//   t_coroutine_count--;
// }

// Coroutine* Coroutine::GetCurrentCoroutine() {
//   if (t_cur_coroutine == nullptr) {
//     t_main_coroutine = new Coroutine();
//     t_cur_coroutine = t_main_coroutine;
//   }
//   return t_cur_coroutine;
// }

// Coroutine* Coroutine::GetMainCoroutine() {
//   if (t_main_coroutine) {
//     return t_main_coroutine;
//   }
//   t_main_coroutine = new Coroutine();
//   return t_main_coroutine;
// }

// bool Coroutine::IsMainCoroutine() {
//   if (t_main_coroutine == nullptr || t_cur_coroutine == t_main_coroutine) {
//     return true;
//   }
//   return false;
// }

// void Coroutine::Yield() {
//   if (t_main_coroutine == nullptr) {
//     return;
//   }

//   if (t_cur_coroutine == t_main_coroutine) {
//     return;
//   }

//   Coroutine* co = t_cur_coroutine;
//   t_cur_coroutine = t_main_coroutine;
//   coctx_swap(&(co->m_coctx), &(t_main_coroutine->m_coctx));
// }

// void Coroutine::Resume(Coroutine* co) {
//   if (t_cur_coroutine != t_main_coroutine) {
//     return;
//   }

//   if (!t_main_coroutine) {
//     return;
//   }
//   if (!co || !co->m_can_resume) {
//     return;
//   }

//   if (t_cur_coroutine == co) {
//     return;
//   }

//   t_cur_coroutine = co;

//   coctx_swap(&(t_main_coroutine->m_coctx), &(co->m_coctx));
// }






// main coroutine, every io thread have a main_coroutine
static thread_local Coroutine* t_main_coroutine = NULL;
// current thread is runing which coroutine
static thread_local Coroutine* t_cur_coroutine = NULL;

static std::atomic_int t_coroutine_count {0};
static std::atomic_int t_cur_coroutine_id {1};


Coroutine::Coroutine()
    : m_index(-1),
      m_is_in_cofunc(false),
      m_can_resume(false) {

    m_cor_id = 0;
    t_coroutine_count++;
    memset(&m_coctx, 0, sizeof(m_coctx));
    t_cur_coroutine = this;
}

Coroutine::Coroutine(int size, char *stack_ptr)
    : m_index(-1),
      m_stack_size(size),
      m_stack_sp(stack_ptr),
      m_is_in_cofunc(false),
      m_can_resume(false) {

    assert(stack_ptr);

    if(!t_main_coroutine) {
        t_main_coroutine = new Coroutine();
    }

    m_cor_id = t_cur_coroutine_id++;
    t_coroutine_count++;
}

Coroutine::Coroutine(int size, char *stack_ptr, std::function<void()> cb)
    : m_index(-1),
      m_stack_size(size),
      m_stack_sp(stack_ptr),
      m_is_in_cofunc(false),
      m_can_resume(false) {

    assert(stack_ptr);

    if(!t_main_coroutine) {
        t_main_coroutine = new Coroutine();
    }

    setCallback(cb);
    m_cor_id = t_cur_coroutine_id++;
    t_coroutine_count++;
}

Coroutine::~Coroutine() {
    m_stack_sp = nullptr;
    t_coroutine_count--;
}


void CoFunction(Coroutine *cor) {
    if(cor != nullptr) {
        cor->setIsInCoFunc(true);
        cor->m_callback();
        cor->setIsInCoFunc(false);
    }

    Coroutine::Yield();
}

bool Coroutine::setCallback(std::function<void()> cb) {
    if(this == t_main_coroutine) {
        LOG_ERROR << "main coroutine can't set callback";
        return false;
    }

    if(m_is_in_cofunc) {
        LOG_ERROR << "this coroutine is in CoFunction";
        return false;
    }

    m_callback = cb;

    char *top = m_stack_sp + m_stack_size;
    top = reinterpret_cast<char *>((reinterpret_cast<unsigned long>(top)) & -16LL);

    memset(&m_coctx, 0, sizeof(m_coctx));

    m_coctx.regs[kRSP] = top;
    m_coctx.regs[kRBP] = top;
    m_coctx.regs[kRETAddr] = reinterpret_cast<char *>(CoFunction); 
    m_coctx.regs[kRDI] = reinterpret_cast<char  *>(this);
    m_can_resume = true;

    return true;
}

void Coroutine::Yield() {
    if(t_main_coroutine == nullptr) {
        LOG_ERROR << "main coroutine is nullptr";
        return ;
    }

    if(t_main_coroutine == t_cur_coroutine) {
        LOG_ERROR << "current coroutine is main coroutine";
        return ;
    }

    Coroutine *cor = t_cur_coroutine;
    t_cur_coroutine = t_main_coroutine;
    coctx_swap(&(cor->m_coctx), &(t_main_coroutine->m_coctx));
}

void Coroutine::Resume(Coroutine *cor) {
    if(t_main_coroutine != t_cur_coroutine) {
        LOG_ERROR << "swap error, current coroutine must be main coroutine";
        return ;
    }

    if(!t_main_coroutine) {
        LOG_ERROR << "main coroutine is nullptr";
        return ;
    }

    if(!cor || !cor->m_can_resume) {
        LOG_ERROR << "pending coroutine is nullptr or can_resume is false";
        return ;
    }

    if(t_cur_coroutine == cor) {
        LOG_DEBUG << "current coroutine is pending cor, need't swap";
        return ;
    }

    t_cur_coroutine = cor;
    coctx_swap(&(t_main_coroutine->m_coctx), &(cor->m_coctx));
}

Coroutine *Coroutine::GetMainCoroutine() {
    if(!t_main_coroutine) {
        t_main_coroutine = new Coroutine();
    }
    return t_main_coroutine;
}

Coroutine *Coroutine::GetCurrentCoroutine() {
    if(!t_cur_coroutine) {
        t_main_coroutine = new Coroutine();
        t_cur_coroutine = t_main_coroutine;
    }
    return t_cur_coroutine;
}

bool Coroutine::IsMainCoroutine() {
    if(!t_main_coroutine || t_main_coroutine == t_cur_coroutine) {
        return true;
    }
    return false;
}

}   // namespace util