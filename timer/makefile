timer:
	g++ main.cc timer.cc fdEvent.cc ../net/reactor.cc ../logger/log.cc ../coroutinePool/coroutine.cc ../coroutinePool/coctx_swap.S ../coroutineHook/coroutineHook.cc -I. -I../mutex -I../logger -I../coroutineHook -I../coroutinePool -I../tcp -I../net -ldl -lpthread -g

clean:
	rm a.out