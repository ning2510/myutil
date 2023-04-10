#ifndef _COROUTINE_H
#define _COROUTINE_H

#include <iostream>
#include <memory>
#include <functional>

#include "coctx.h"

namespace util {

class Coroutine {
public:
    typedef std::shared_ptr<Coroutine> ptr;

    Coroutine(int size, char *stack_ptr);
    Coroutine(int size, char *stack_ptr, std::function<void()> cb);
    ~Coroutine();

    bool setCallback(std::function<void()> cb);

    int getCorId() const { return m_cor_id; }

    void setStackPtr(char *stack_sp) { m_stack_sp = stack_sp; }
    const char *getStackPtr() const { return m_stack_sp; }

    void setIndex(int index) { m_index = index; }
    int getIndex() const { return m_index; }

    bool setIsInCoFunc(bool value) { m_is_in_cofunc = value; }
    bool getIsInCoFunc() const { return m_is_in_cofunc; }

    void setCanResume(bool value) { m_can_resume = value; }
    bool getCanResume() const { return m_can_resume; }

    static void Yield();
    static void Resume(Coroutine *cor);

    static Coroutine *GetMainCoroutine();
    static Coroutine *GetCurrentCoroutine();

    static bool IsMainCoroutine();

    std::function<void()> m_callback;

private:
    Coroutine();

    int m_index;
    int m_cor_id;
    int m_stack_size;
    char *m_stack_sp;

    bool m_is_in_cofunc;
    bool m_can_resume;

    coctx m_coctx;
};

}   // namespace util

#endif
