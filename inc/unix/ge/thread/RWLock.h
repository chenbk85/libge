// RWLock.h

#ifndef RW_LOCK_H
#define RW_LOCK_H

#include <ge/common.h>

#include <pthread.h>

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

    pthread_rwlock_t m_lock;
};

#endif // RW_LOCK_H
