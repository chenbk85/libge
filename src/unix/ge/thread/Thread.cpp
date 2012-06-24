// Thread.cpp

#include "ge/thread/Thread.h"

#include "ge/SystemException.h"
#include "ge/io/Console.h"
#include "gepriv/UnixUtil.h"

// Internal function required by the pthread API.
static void* taskStarter(void *);

// Thread parameter
class threadParam
{
public:
    Runnable* runnable;
    bool autoDelete;
};

Thread::Thread() :
    _wasStarted(false),
    _isRunning(false)
{

}

Thread::~Thread()
{
    // Just detach the thread
    if (_wasStarted)
    {
        ::pthread_detach(_id);
    }
}

void Thread::run()
{
    // Exciting default implementation
}

void Thread::start()
{
    start(this, false);
}

void Thread::start(Runnable* runnable, bool autoDelete)
{
    if (_wasStarted)
    {
        throw SystemException("Thread already started");
    }

    // Create the thread parameter
    threadParam* param = new threadParam();
    param->runnable = runnable;
    param->autoDelete = autoDelete;

    _wasStarted = true;
    _isRunning = true;

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);

    int pthreadErr = ::pthread_create(&_id, // Thread id
            &attr, // Attributes
            taskStarter, // Function to call
            param); // Function argument

    if (pthreadErr != 0)
    {
        _wasStarted = false;
        _isRunning = false;
        delete param;

        Error error = UnixUtil::getError(pthreadErr,
                                         "pthread_create",
                                         "Thread::start");
        throw SystemException(error);
    }
}

void Thread::join()
{
    if (!_isRunning)
    {
        throw SystemException("Failed to join thread: Thread not running");
    }

    // Try to join the thread
    int32 pthreadErr = ::pthread_join(_id, NULL);

    if (pthreadErr != 0)
    {
        Error error = UnixUtil::getError(pthreadErr,
                                         "pthread_join",
                                         "Thread::join");
        throw SystemException(error);
    }

    _isRunning = false;
}

/*
 * Internal function required by the pthread API. Simply calls the run
 * function of the passed Runnable object.
 */
static void* taskStarter(void * arg)
{
    threadParam* param = (threadParam*)arg;

    try
    {
        param->runnable->run();
    }
    catch (...)
    {
        Console::outln("Uncaught exception");
        // TODO: Log
    }

    if (param->autoDelete)
        delete param->runnable;
    delete param;

    return NULL;
}
