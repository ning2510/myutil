#include "mutex.h"

#include <iostream>

using namespace std;
using namespace util;

int main() {
    Mutex mutex;
    Mutex::Lock lock(mutex);
    cout << "common mutex" << endl;
    lock.unlock();

    RWMutex rwmutex;
    RWMutex::ReadLock rlock(rwmutex);
    cout << "read mutex" << endl;
    rlock.unlock();

    RWMutex::WriteLock wlock(rwmutex);
    cout << "write mutex" << endl;
    wlock.unlock();

    return 0;
}