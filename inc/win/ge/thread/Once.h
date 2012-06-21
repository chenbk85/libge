// Once.h

#ifndef ONCE_H
#define ONCE_H

#include <ge/common.h>

/*
 * Class that is a wrapper for a volatile long. The assign in the
 * constructor should be atomic because of the volatile flag.
 */
class Once_t
{
public:
    Once_t()
    {
        control = 0;
    }

    volatile long control;
};

// Will run func one time for the same Once_t regardless of the number of calls
void call_once(Once_t& once_t, void (func)());


#endif // ONCE_H
