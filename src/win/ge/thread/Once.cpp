// Once.cpp

#include <ge/thread/Once.h>

#include <Windows.h>

/*
 * This is the second simplest implementation for this logic I
 * could find. Basically this is a spin lock with three values:
 *
 * 0 - Has not yet been run
 * 1 - Have aquired the lock and need to call the function
 * 2 - The function has already been called
 *
 * Obviously, there is a simpler implementation where there are only
 * two states and the lock is always aquired, but having the third
 * state of "already called" makes the efficiency reasonable.
 */
void call_once(Once_t& once_t, void (func)())
{
    while (true)
    {
        // Atomicly check if control is 0. If it is, copy in a 1
        LONG prev = InterlockedCompareExchange(&once_t.control, 1, 0);

        if (prev == 2)
        {
            // If the previous value of control was a 2, then func
            // has already been called
            return;
        }
        else if (prev == 0)
        {
            // If prev was 0, then control is now 1 and we own the spinlock
            break;
        }
        else
        {
            // Another thread is calling func, start spinning on control
            Sleep(1); // Give up the remaining timeslot (0 will not release on Windows)
        }
    }

    // Only the first caller can reach here as it will have hit the break
    // above. We can safely call func.
    func();

    // Now that func is called, atomicly copy a 2 into control to indicate
    // that it has been called already
    InterlockedExchange(&once_t.control, 2);
}

