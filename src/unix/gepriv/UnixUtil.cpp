// UnixUtil.cpp

#include "gepriv/UnixUtil.h"

#include <errno.h> // errno/error values
#include <fcntl.h> // open
#include <string.h> // memcpy, memset
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

Error UnixUtil::getError(int errorNumber,
                         const char* context,
                         const char* systemCall)
{
    CommonError commonErr = err_unknown;

    switch (errorNumber)
    {
        // Generic errors
        case EINVAL:
        case ENOPROTOOPT:
            commonErr = err_invalid_argument; break;
        case EBADF:
        case EBADFD:
            commonErr = err_invalid_handle; break;
        case ENFILE:
        case EMFILE:
            commonErr = err_too_many_handles; break;
        case EFAULT:
            commonErr = err_bad_address; break;
        case ENOSYS:
            commonErr = err_system_call_not_supported; break;
        case EIDRM:
            commonErr = err_identifier_removed; break;
        case ENOMEM:
            commonErr = err_not_enough_memory; break;
        case ENOTSUP:
        case EOPNOTSUPP:
            commonErr = err_not_supported; break;
        case EACCES:
        case EPERM:
        case EROFS:
            commonErr = err_access_denied; break;
        case ETIMEDOUT:
        case ETIME:
            commonErr = err_timed_out; break;

        // Process errors
        case ENOEXEC:
            commonErr = err_invalid_executable; break;

        // Generic IO errors
        case EPIPE:
            commonErr = err_broken_pipe; break;
        case EIO:
            commonErr = err_io_error; break;
        case ENOLCK:
            commonErr = err_no_lock_available; break;
        case ECANCELED:
            commonErr = err_io_canceled; break;

        // File IO errors
        case ENOENT:
            commonErr = err_file_not_found; break;
        case EEXIST:
            commonErr = err_file_exists; break;
        case EFBIG:
            commonErr = err_file_too_large; break;
        case EBUSY:
        case ETXTBSY:
            commonErr = err_file_in_use; break;
        case ENAMETOOLONG:
            commonErr = err_filename_too_long; break;
        case EISDIR:
            commonErr = err_is_a_directory; break;
        case ENOTDIR:
            commonErr = err_not_a_directory; break;
        case ENOTEMPTY:
            commonErr = err_directory_not_empty; break;
        case ESPIPE:
            commonErr = err_invalid_seek; break;
        case ENOSPC:
            commonErr = err_no_space_on_device; break;
        case ENXIO:
        case ENODEV:
            commonErr = err_no_such_device; break;

        // File link errors
        case EXDEV:
            commonErr = err_cross_device_link; break;
        case EMLINK:
            commonErr = err_too_many_links; break;
        case ELOOP:
            commonErr = err_too_many_synbolic_link_levels; break;

        // INet errors
        case ENOBUFS:
            commonErr = err_no_buffer_space; break;
        case EAFNOSUPPORT:
            commonErr = err_address_not_supported; break;
        case EADDRINUSE:
            commonErr = err_address_in_use; break;
        case EADDRNOTAVAIL:
            commonErr = err_address_not_available; break;
        case EISCONN:
            commonErr = err_already_connected; break;
        case E2BIG:
            commonErr = err_argument_list_too_long; break;
        case ECONNABORTED:
            commonErr = err_connection_aborted; break;
        case EALREADY:
            commonErr = err_connection_already_in_progress; break;
        case ECONNREFUSED:
            commonErr = err_connection_refused; break;
        case ECONNRESET:
            commonErr = err_connection_reset; break;
        case ESHUTDOWN:
            commonErr = err_connection_shutdown; break;
        case ENOTCONN:
            commonErr = err_not_connected; break;
        case EHOSTUNREACH:
            commonErr = err_host_unreachable; break;
        case ENETDOWN:
            commonErr = err_network_down; break;
        case ENETRESET:
            commonErr = err_network_reset; break;
        case ENETUNREACH:
            commonErr = err_network_unreachable; break;
        case EDESTADDRREQ:
            commonErr = err_destination_address_required; break;
        case EMSGSIZE:
            commonErr = err_message_too_long; break;
        case EPROTO:
        case EPROTONOSUPPORT:
            commonErr = err_protocol_error; break;

        // Text errors
        case EILSEQ:
            commonErr = err_illegal_byte_sequence; break;
    }

    return Error(commonErr,
                 errorNumber,
                 context,
                 systemCall);
}

String UnixUtil::getLastErrorMessage()
{
    return getErrorMessage(errno);
}

String UnixUtil::getErrorMessage(int errorNumber)
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

int UnixUtil::sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    int ret;

    do
    {
        ret = ::accept(sockfd, addr, addrlen);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int UnixUtil::sys_dup2(int oldfd, int newfd)
{
    int ret;

    do
    {
        ret = ::dup2(oldfd, newfd);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int UnixUtil::sys_ftruncate(int fd, size_t length)
{
    int ret;

    do
    {
        ret = ::ftruncate(fd, length);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int UnixUtil::sys_open(const char* pathname, int flags, int mode)
{
    int ret;

    do
    {
        ret = ::open(pathname, flags, mode);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

int UnixUtil::sys_poll(struct pollfd *fds, nfds_t nfds, int timeout)
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

ssize_t UnixUtil::sys_read(int fd, void* buffer, size_t count)
{
    ssize_t ret;

    do
    {
        ret = ::read(fd, buffer, count);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t UnixUtil::sys_recv(int sockfd, void* buf, size_t len, int flags)
{
    int32 ret;

    do
    {
        ret = ::recv(sockfd, buf, len, flags);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t UnixUtil::sys_send(int sockfd, const void* buf, size_t len, int flags)
{
    ssize_t ret;

    do
    {
        ret = ::send(sockfd, buf, len, flags);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t UnixUtil::sys_write(int fd, const void* buffer, size_t count)
{
    int32 ret;

    do
    {
        ret = ::write(fd, buffer, count);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

pid_t UnixUtil::sys_waitpid(pid_t pid, int* status, int options)
{
    pid_t ret;

    do
    {
        ret = ::waitpid(pid, status, options);
    } while (ret == -1 && errno == EINTR);

    return ret;
}
