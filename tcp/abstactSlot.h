#ifndef _ABSTACTSLOT_H
#define _ABSTACTSLOT_H

#include <memory>
#include <functional>

namespace util {

template <class T>
class AbstractSlot {
public:
    typedef std::shared_ptr<AbstractSlot> ptr;
    typedef std::shared_ptr<T> sharedPtr;
    typedef std::weak_ptr<T> weakPtr;

    AbstractSlot(weakPtr ptr, std::function<void(sharedPtr)> cb)
        : m_weak_ptr(ptr),
          m_callback(cb) {}

    ~AbstractSlot() {
        sharedPtr ptr = m_weak_ptr.lock();
        if(ptr) {
            m_callback(ptr);
        }
    }

private:
    weakPtr m_weak_ptr;
    std::function<void(sharedPtr)> m_callback;
};

}   // namespace util

#endif
