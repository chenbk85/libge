// RWLock.h

#ifndef RW_LOCK_H
#define RW_LOCK_H

#include <ge/common.h>
#include <ge/thread/Condition.h>

#include <Windows.h>

/*
 * A reader/writer lock.
 */
class RWLock
{
public:
    enum AccessTypeEnum
    {
        READER=0,
        WRITER=1
    };

    RWLock();
    ~RWLock();

    void lock(AccessTypeEnum accessType);
    bool trylock(AccessTypeEnum accessType);
    void unlock(AccessTypeEnum accessType);

private:
    RWLock(const RWLock& other);
    RWLock& operator=(const RWLock& other);

#if (NTDDI_VERSION < NTDDI_VISTA)
    Condition m_cond;
    uint32 m_readers;
    uint32 m_writers;
#else
    SRWLOCK m_lock;
#endif
};

#endif // RW_LOCK_H
