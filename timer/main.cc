#include "timer.h"
#include "log.h"
#include "reactor.h"

#include <memory>
#include <iostream>
#include <functional>

using namespace std;
using namespace util;

void timer3s() {
    int64_t now = getNowMs();
    cout << "timer 3s! now time = " << now << endl;

    cout << "Please enter [CTRL + C] to exit" << endl; 
}

void timer1s() {
    int64_t now = getNowMs();
    cout << "timer 1s! now time = " << now << endl;
}

int main() {
    initLog("test_log");
    
    Reactor *reactor = Reactor::GetReactor();
    reactor->setReactorType(MainReactor);
    Timer *timer = reactor->getTimer();

    int64_t now = getNowMs();
    cout << "now time(ms) = " << now << endl;
    // timer 1s
    timer->addTimerEvent(make_shared<TimerEvent>(1000, false, std::bind(timer1s)));
    // timer 3s
    timer->addTimerEvent(make_shared<TimerEvent>(3000, false, std::bind(timer3s)));

    reactor->loop();

    return 0;
}