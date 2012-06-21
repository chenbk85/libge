// UnixUtil.cpp

#include "gepriv/UnixUtil.h"

#include <cerrno>
#include <cstring>

#include <fcntl.h> // open
#include <unistd.h> // read, write
#include <sys/wait.h> // waitpid
#include <sys/stat.h> // open
#include <sys/time.h> // gettimeofday
#include <sys/types.h> // open

#if defined(CLOCK_MONOTONIC_COARSE)
#define PREFERRED_CLOCK CLOCK_MONOTONIC_COARSE
#else
#define PREFERRED_CLOCK CLOCK_MONOTONIC
#endif

/*
 * Differences two timespec structs
 */
static inline
void timespec_diff(struct timespec* res,
                   const struct timespec* start,
                   const struct timespec* end)
{
    res->tv_sec = end->tv_sec - start->tv_sec;
    res->tv_nsec = end->tv_nsec - start->tv_nsec;

    if (res->tv_nsec < 0)
    {
        res->tv_sec--;
        res->tv_nsec += 1000000000;
    }
}

namespace UnixUtil
{

String getLastErrorMessage()
{
    return getErrorMessage(errno);
}

String getErrorMessage(int errorNumber)
{
#if defined(__CYGWIN__) || defined(__MINGW__)
    const char* errMsg = strerror(errorNumber);
    return String(errMsg);
#else
    char buffer[512];
    strerror_r(errorNumber, buffer, sizeof(buffer));
    return String(buffer);
#endif
}

int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    int ret;

    do
    {
        ret = ::accept(sockfd, addr, addrlen);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int sys_dup2(int oldfd, int newfd)
{
    int ret;

    do
    {
        ret = ::dup2(oldfd, newfd);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int sys_ftruncate(int fd, size_t length)
{
    int ret;

    do
    {
        ret = ::ftruncate(fd, length);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int sys_open(const char* pathname, int flags, int mode)
{
    int ret;

    do
    {
        ret = ::open(pathname, flags, mode);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int sys_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    struct timespec startTime;
    struct timespec endTime;
    struct timespec deltaTime;
    int ret;

    if (timeout < 0)
    {
        do
        {
            ret = ::poll(fds, nfds, timeout);
        } while (ret == -1 && errno == EINTR);
    }
    else
    {
        ::clock_gettime(PREFERRED_CLOCK, &startTime);

        do
        {
            ret = ::poll(fds, nfds, timeout);

            if (ret == -1 && errno == EINTR)
            {
                ::clock_gettime(PREFERRED_CLOCK, &endTime);
                timespec_diff(&deltaTime, &startTime, &endTime);
                timeout -= ((deltaTime.tv_sec * 1000) + (deltaTime.tv_nsec / 1000000));
                ::memcpy(&startTime, &endTime, sizeof(struct timespec));
            }
            else
            {
                break;
            }
        } while (1);
    }

    return ret;
}

ssize_t sys_read(int fd, void* buffer, size_t count)
{
    ssize_t ret;

    do
    {
        ret = ::read(fd, buffer, count);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t sys_recv(int sockfd, void* buf, size_t len, int flags)
{
    int32 ret;

    do
    {
        ret = ::recv(sockfd, buf, len, flags);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t sys_send(int sockfd, const void* buf, size_t len, int flags)
{
    ssize_t ret;

    do
    {
        ret = ::send(sockfd, buf, len, flags);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t sys_write(int fd, const void* buffer, size_t count)
{
    int32 ret;

    do
    {
        ret = ::write(fd, buffer, count);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

pid_t sys_waitpid(pid_t pid, int* status, int options)
{
    pid_t ret;

    do
    {
        ret = ::waitpid(pid, status, options);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

} // End namespace UnixUtil
