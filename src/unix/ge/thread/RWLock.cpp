// RWLock.cpp

#include "ge/thread/RWLock.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

RWLock::RWLock()
{
    int res = ::pthread_rwlock_init(&m_lock, NULL);

    if (res != 0)
    {
        if (errno == ENOMEM)
        {
            throw std::bad_alloc();
        }

        ::fprintf(stderr, "pthread_rwlock_init failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

RWLock::~RWLock()
{
    int res = ::pthread_rwlock_destroy(&m_lock);

    if (res != 0)
    {
        ::fprintf(stderr, "pthread_rwlock_destroy failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}

void RWLock::lock(AccessTypeEnum accessType)
{
    int res;
    const char* fun;

    if (accessType != READER)
    {
        res = pthread_rwlock_wrlock(&m_lock);
        fun = "pthread_rwlock_wrlock";
    }
    else
    {
        res = pthread_rwlock_rdlock(&m_lock);
        fun = "pthread_rwlock_rdlock";
    }

    if (res == 0)
    {
        ::fprintf(stderr, "%s failed with \"%s\" (%d). Aborting.\n",
                fun, ::strerror(errno), errno);
        ::abort();
    }
}

bool RWLock::trylock(AccessTypeEnum accessType)
{
    int res;
    const char* fun;

    if (accessType != READER)
    {
        res = pthread_rwlock_trywrlock(&m_lock);
        fun = "pthread_rwlock_trywrlock";
    }
    else
    {
        res = pthread_rwlock_tryrdlock(&m_lock);
        fun = "pthread_rwlock_tryrdlock";
    }

    if (res == 0)
    {
        return true;
    }
    else if (errno == EBUSY)
    {
        return false;
    }

    ::fprintf(stderr, "%s failed with \"%s\" (%d). Aborting.\n",
            fun, ::strerror(errno), errno);
    ::abort();
}

void RWLock::unlock(AccessTypeEnum accessType)
{
    int res = pthread_rwlock_unlock(&m_lock);

    if (res != 0)
    {
        ::fprintf(stderr, "pthread_rwlock_unlock failed with \"%s\" (%d). "
                "Aborting.\n", ::strerror(errno), errno);
        ::abort();
    }
}
