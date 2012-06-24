// CurrentThread.h

#include <ge/common.h>
#include <ge/text/StringRef.h>

namespace CurrentThread
{
    /*
     * Makes the current thread sleep for the given number of milliseconds.
     */
    void sleep(uint32 millis);

    /*
     * Sets the name of the current thread.
     */
    void setName(const StringRef name);

    /*
     * Yields the CPU to some other thread.
     */
    void yield();
};
