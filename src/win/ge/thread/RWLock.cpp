// RWLock.cpp

#include "ge/thread/RWLock.h"

RWLock::RWLock()
{
#if (NTDDI_VERSION < NTDDI_VISTA)
    m_readers = 0;
    m_writers = 0;
#else
    ::InitializeSRWLock(&m_lock);
#endif
}

RWLock::~RWLock()
{
    
}

void RWLock::lock(AccessTypeEnum accessType)
{
#if (NTDDI_VERSION < NTDDI_VISTA)
    m_cond.lock();

    if (accessType != READER)
    {  
        while (m_readers > 0 ||
               m_writers > 0)
        {
            m_cond.wait();
        }

        m_writers++;
    }
    else
    {   
        while (m_writers > 0)
        {
            m_cond.wait();
        }

        m_readers++;
    }

    m_cond.unlock();
#else
    if (accessType != READER)
    {  
        ::AcquireSRWLockExclusive(&m_lock);
    }
    else
    {   
        ::AcquireSRWLockShared(&m_lock);
    }
#endif
}

bool RWLock::trylock(AccessTypeEnum accessType)
{
#if (NTDDI_VERSION < NTDDI_VISTA)
    m_cond.lock();

    if (accessType != READER)
    {  
        if (m_readers > 0 ||
            m_writers > 0)
        {
            m_cond.unlock();
            return false;
        }

        m_writers++;
    }
    else
    {   
        while (m_writers > 0)
        {
            m_cond.unlock();
            return false;
        }

        m_readers++;
    }

    m_cond.unlock();

    return true;
#else
    BOOLEAN ret;

    if (accessType != READER)
    {  
        ret = ::TryAcquireSRWLockExclusive(&m_lock);
    }
    else
    {   
        ret = ::TryAcquireSRWLockShared(&m_lock);
    }
    return (ret != 0);
#endif
}

void RWLock::unlock(AccessTypeEnum accessType)
{
#if (NTDDI_VERSION < NTDDI_VISTA)
    m_cond.lock();

    if (accessType != READER)
    { 
        m_writers--;
    }
    else
    {
        m_readers--;
    }

    m_cond.signal();
    m_cond.unlock();
#else
    if (accessType != READER)
    {  
        ::ReleaseSRWLockExclusive(&m_lock);
    }
    else
    {   
        ::ReleaseSRWLockShared(&m_lock);
    }
#endif
}
