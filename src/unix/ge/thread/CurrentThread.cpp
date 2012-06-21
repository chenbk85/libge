//CurrentThread.cpp

#include "ge/thread/CurrentThread.h"

#include <unistd.h> // For usleep()

namespace CurrentThread
{

void sleep(uint32 milliseconds)
{
    ::usleep(milliseconds * 1000);
}

}
