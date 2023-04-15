server:
	g++ server.cc httpCodec.cc httpDefine.cc httpDispatcher.cc httpServlet.cc ../timeWheel/timeWheel.cc ../timer/timer.cc ../timer/fdEvent.cc ../netAddress/netAddress.cc ../logger/log.cc ../coroutinePool/memory.cc ../coroutinePool/coroutine.cc ../coroutinePool/coroutinePool.cc ../coroutinePool/coctx_swap.S ../coroutineHook/coroutineHook.cc ../net/reactor.cc ../tcp/ioThread.cc ../tcp/tcpServer.cc ../tcp/tcpConnection.cc -I. -I../mutex -I../logger -I../buffer -I../timeWheel -I../coroutineHook -I../coroutinePool -I../tcp -I../timer -I../netAddress -I../net -I../http -ldl -lpthread -g

clean:
	rm a.out