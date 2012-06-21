// UnixUtil.h

#ifndef UNIX_UTIL_H
#define UNIX_UTIL_H

#include "ge/text/String.h"

#include <poll.h> // nfds_t, pollfd, poll
#include <sys/types.h> // pid_t
#include <sys/socket.h> // sockaddr, socklen_t

/*
 * Names prefixed with "sys_" are system calls that require user code retry
 * on interrupt.
 */
namespace UnixUtil
{
    String getLastErrorMessage();

    String getErrorMessage(int errorNumber);

    int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);

    int sys_dup2(int oldfd, int newfd);

    int sys_ftruncate(int fd, size_t length);

    int sys_open(const char* pathname, int flags, int mode);

    int sys_poll(struct pollfd *fds, nfds_t nfds, int timeout);

    ssize_t sys_read(int fd, void* buffer, size_t count);

    ssize_t sys_recv(int sockfd, void* buf, size_t len, int flags);

    ssize_t sys_send(int sockfd, const void* buf, size_t len, int flags);

    ssize_t sys_write(int fd, const void* buffer, size_t count);

    pid_t sys_waitpid(pid_t pid, int* status, int options);
}

#endif // UNIX_UTIL_H
