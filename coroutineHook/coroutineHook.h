#ifndef _COROUTINEHOOK_H
#define _COROUTINEHOOK_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef int (*accept_fun_ptr_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

typedef int (*connect_fun_ptr_t)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

typedef ssize_t (*read_fun_ptr_t)(int fd, void *buf, size_t count);

typedef ssize_t (*write_fun_ptr_t)(int fd, const void *buf, size_t count);

typedef unsigned int (*sleep_fun_ptr_t)(unsigned int seconds);


namespace util {

int accept_hook(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int connect_hook(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t read_hook(int fd, void *buf, size_t count);

ssize_t write_hook(int fd, const void *buf, size_t count);

unsigned int sleep_hook(unsigned int seconds);

}   // namespace util

extern "C" {

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t read(int fd, void *buf, size_t count);

ssize_t write(int fd, const void *buf, size_t count);

unsigned int sleep(unsigned int seconds);

}

#endif
