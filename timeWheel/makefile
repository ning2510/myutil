server:
	g++ -o server server.cc timeWheel.cc ../timer/timer.cc ../timer/fdEvent.cc ../netAddress/netAddress.cc ../logger/log.cc ../coroutinePool/memory.cc ../coroutinePool/coroutine.cc ../coroutinePool/coroutinePool.cc ../coroutinePool/coctx_swap.S ../coroutineHook/coroutineHook.cc ../tcp/reactor.cc ../tcp/ioThread.cc ../tcp/tcpServer.cc ../tcp/tcpConnection.cc ../http/httpCodec.cc ../http/httpDefine.cc ../http/httpDispatcher.cc ../http/httpServlet.cc -I. -I../mutex -I../logger -I../buffer -I../coroutineHook -I../coroutinePool -I../tcp -I../timer -I../netAddress -I../net -I../http -ldl -lpthread -g

client:
	g++ -o client client.cc -g

clean:
	rm server client