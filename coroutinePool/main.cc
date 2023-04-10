#include <iostream>
#include <unistd.h>

#include "coroutinePool.h"
#include "log.h"

using namespace std;
using namespace util;

Coroutine::ptr cor1;
Coroutine::ptr cor2;

void func1() {
    cout << "func1 start !" << endl;
    Coroutine::Yield();
    cout << "func1 end !" << endl;
}

void func2() {
    cout << "func2 start !" << endl;
    Coroutine::Yield();
    cout << "func2 end !" << endl;
}

/* test coroutine */
void testCoroutine() {
    int stack_size = 128 * 1024;
    char* sp = reinterpret_cast<char*>(malloc(stack_size));
    cor1 = std::make_shared<Coroutine>(stack_size, sp, func1);


    char* sp2 = reinterpret_cast<char*>(malloc(stack_size));
    cor2 = std::make_shared<Coroutine>(stack_size, sp2, func2);

    Coroutine::Resume(cor1.get());
    Coroutine::Resume(cor2.get());
}

/* test coroutinePool、coroutine、memory */
void testCoroutinePool() {
    cor1 = GetCoroutinePool()->getCoroutineInstance();
    cor1->setCallback(std::bind(func1));

    cor2 = GetCoroutinePool()->getCoroutineInstance();
    cor2->setCallback(std::bind(func2));

    Coroutine::Resume(cor1.get());
    Coroutine::Resume(cor2.get());
}

int main() {
    initLog("test_log");
    // LOG_INFO << "main start !";
    cout << "=== main start" << endl;

    /* testCoroutine(); */
    testCoroutinePool();    
    cout << "=== back to main" << endl;
    
    Coroutine::Resume(cor1.get());
    Coroutine::Resume(cor2.get());

    cout << "=== main end" << endl;
    // LOG_INFO << "main end ~";
    return 0;
}