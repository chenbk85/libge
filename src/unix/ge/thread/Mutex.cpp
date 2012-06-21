// Mutex.cpp

#include <ge/thread/Mutex.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

Mutex::Mutex()
{
    pthread_mutexattr_t attr;

    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    int res = ::pthread_mutex_init(&m_mutex, &attr);

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
}

Mutex::~Mutex()
{
    int res = ::pthread_mutex_destroy(&m_mutex);

    if (res != 0)
    {
        ::fprintf(stderr, "pthread_mutex_destroy failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

void Mutex::lock()
{
    int res = ::pthread_mutex_lock(&m_mutex);

    if (res != 0)
    {
        ::fprintf(stderr, "pthread_mutex_lock failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

bool Mutex::trylock()
{
    int res = ::pthread_mutex_trylock(&m_mutex);

    if (res != 0 && res != EBUSY)
    {
        ::fprintf(stderr, "pthread_mutex_trylock failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }

    return (res != EBUSY);
}

void Mutex::unlock()
{
    int res = ::pthread_mutex_unlock(&m_mutex);

    if (res != 0)
    {
        ::fprintf(stderr, "pthread_mutex_unlock failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}
