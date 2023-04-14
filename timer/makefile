timer:
	g++ main.cc timer.cc fdEvent.cc ../tcp/reactor.cc ../logger/log.cc ../coroutinePool/coroutine.cc ../coroutinePool/coctx_swap.S ../coroutineHook/coroutineHook.cc -I. -I../mutex -I../logger -I../coroutineHook -I../coroutinePool -I../tcp -ldl -lpthread -g

clean:
	rm a.out