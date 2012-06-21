// Thread.cpp

#include "ge/thread/Thread.h"

#include "ge/SystemException.h"
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
    m_wasStarted(false),
    m_isRunning(false)
{

}

Thread::~Thread()
{
    // Just detach the thread
    if (m_wasStarted)
    {
        ::pthread_detach(m_id);
    }
}

void Thread::start(Runnable* runnable, bool autoDelete)
{
    if (m_wasStarted)
    {
        throw SystemException("Thread already started");
    }

    threadParam* param = new threadParam();
    param->runnable = runnable;
    param->autoDelete = autoDelete;

    m_wasStarted = true;
    m_isRunning = true;

    pthread_attr_t attr;
    ::pthread_attr_init(&attr);

    int32 error = ::pthread_create(&m_id, // Thread id
            &attr, // Attributes
            taskStarter, // Function to call
            param); // Function argument

    if (error != 0)
    {
        throw SystemException(String("Failed to start thread: ") +
            UnixUtil::getErrorMessage(error));
    }
}

void Thread::join()
{
    if (!m_isRunning)
    {
        throw SystemException("Failed to join thread: Thread not running");
    }

    // Try to join the thread
    int32 error = ::pthread_join(m_id, NULL);

    if (error)
    {
        throw SystemException(String("Failed to join thread: ") +
            UnixUtil::getErrorMessage(error));
    }

    m_isRunning = false;
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
        // TODO: Log
    }

    if (param->autoDelete)
        delete param->runnable;
    delete param;

    return NULL;
}
