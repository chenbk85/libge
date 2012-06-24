//CurrentThread.cpp

#include "ge/thread/CurrentThread.h"

#include <unistd.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <pthread.h>


void CurrentThread::sleep(uint32 milliseconds)
{
    ::usleep(milliseconds * 1000);
}

void CurrentThread::setName(const StringRef name)
{
#ifdef __linux__
    // Can set the name with prctl. Supports 16 byte names.
    char nameBuffer[17];
    size_t len = name.length();

    if (len > 16)
        len = 16;

    ::memcpy(nameBuffer, name.data(), len);
    nameBuffer[len] = '\0';

    ::prctl(PR_SET_NAME, nameBuffer, 0, 0, 0);
#endif
}

void CurrentThread::yield()
{
    ::pthread_yield();
}
