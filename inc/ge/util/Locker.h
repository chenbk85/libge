// Locker.h

#ifndef LOCKER_H
#define LOCKER_H

/*
 * Template class for automatically locking and unlocking resources in an
 * RTTI pattern.
 *
 * Type must have void lock() and unlock() functions.
 */
template <typename T>
class Locker
{
public:
    Locker(T& lockable) :
        target(lockable)
    {
        target.lock();
    }

    ~Locker()
    {
        target.unlock();
    }

private:
    Locker(const Locker&) {}
    Locker& operator=(const Locker&) {}

    T& target;
};

#endif // LOCKER_H
