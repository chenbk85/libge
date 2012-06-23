// Condition.cpp

#include <ge/thread/Condition.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <sys/time.h>

#if defined(CLOCK_MONOTONIC_COARSE)
#define PREFERRED_CLOCK CLOCK_MONOTONIC_COARSE
#else
#define PREFERRED_CLOCK CLOCK_MONOTONIC
#endif

Condition::Condition()
{
    int res;
    pthread_mutexattr_t attr;

    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    res = ::pthread_mutex_init(&m_mutex, &attr);

    if (res != 0)
    {
        if (errno == ENOMEM)
        {
            throw std::bad_alloc();
        }

        ::fprintf(stderr, "pthread_mutex_init failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }

    res = ::pthread_cond_init(&m_cond, NULL);

    if (res != 0)
    {
        if (errno == ENOMEM)
        {
            throw std::bad_alloc();
        }

        ::fprintf(stderr, "pthread_cond_init failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

Condition::~Condition()
{
    int cond_res = ::pthread_cond_destroy(&m_cond);

    if (cond_res != 0)
    {
        ::fprintf(stderr, "pthread_cond_destroy failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }

    int mutex_res = ::pthread_mutex_destroy(&m_mutex);

    if (mutex_res != 0)
    {
        ::fprintf(stderr, "pthread_mutex_destroy failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

void Condition::lock()
{
    int res = ::pthread_mutex_lock(&m_mutex);

    if (res != 0)
    {
        ::fprintf(stderr, "pthread_mutex_lock failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

void Condition::unlock()
{
    int res = ::pthread_mutex_unlock(&m_mutex);

    if (res != 0)
    {
        ::fprintf(stderr, "Condition::unlock(): pthread_mutex_unlock failed "
                "with \"%s\" (%d). Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

void Condition::signal()
{
    int res = ::pthread_cond_signal(&m_cond);

    if (res != 0)
    {
        ::fprintf(stderr, "Condition::signal(): pthread_cond_signal "
                "failed with \"%s\" (%d). Aborting.\n",
                ::strerror(errno), errno);
        ::abort();
    }
}

void Condition::signalAll()
{
    int res = ::pthread_cond_broadcast(&m_cond);

    if (res != 0)
    {
        ::fprintf(stderr, "Condition::wait(): pthread_cond_broadcast failed "
                "with \"%s\" (%d). Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

void Condition::wait()
{
    int res = ::pthread_cond_wait(&m_cond, &m_mutex);

    if (res != 0)
    {
        ::fprintf(stderr, "res=%d\n", res);
        ::fprintf(stderr, "Condition::wait(): pthread_cond_wait failed "
                "with \"%s\" (%d). Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

uint32 Condition::wait(uint32 milliseconds)
{
    struct timespec startTime;
    struct timespec endTime;
    struct timespec waitTillTime;
    int res;

    ::clock_gettime(PREFERRED_CLOCK, &startTime);

    // pthread_cond_timedwait requires a time to wait till, not an amount of
    // time to wait, so we have to generate a time that is the current time
    // plus the passed milliseconds.
    waitTillTime.tv_sec = startTime.tv_sec + (milliseconds / 1000);
    waitTillTime.tv_nsec = (milliseconds % 1000) * 1000000;

    // Handle nanosecond overflow
    if (waitTillTime.tv_nsec > 1000000000)
    {
        waitTillTime.tv_sec++;
        waitTillTime.tv_nsec -= 1000000000;
    }

    // Do the actual wait.
    res = ::pthread_cond_timedwait(&m_cond, &m_mutex, &waitTillTime);

    if (res != 0 && errno != ETIMEDOUT)
    {
        ::fprintf(stderr, "Condition::wait(uint32): pthread_cond_timedwait "
                "failed with \"%s\" (%d). Aborting.\n",
                ::strerror(errno), errno);
        ::abort();
    }

    // Find out how long we waited
    ::clock_gettime(PREFERRED_CLOCK, &endTime);

    // Return the difference of endTime and startTime
    uint32 ret = (endTime.tv_sec - startTime.tv_sec) * 1000;
    ret += (endTime.tv_nsec - startTime.tv_nsec) / 1000000;
    return ret;
}
