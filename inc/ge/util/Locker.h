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
        _target(lockable)
    {
        _target.lock();
        _locked = true;
    }

    ~Locker()
    {
        if (_locked)
            _target.unlock();
    }

    void lock()
    {
        _target.lock();
        _locked = true;
    }

    void unlock()
    {
        _target.unlock();
        _locked = false;
    }

private:
    Locker(const Locker&) {}
    Locker& operator=(const Locker&) {}

    bool _locked;
    T& _target;
};

#endif // LOCKER_H
