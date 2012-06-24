// Thread.cpp

#include <ge/thread/Thread.h>

#include <ge/io/Console.h>
#include <ge/SystemException.h>

#include <gepriv/WinUtil.h>

#include <process.h>

// Function with the interface required by the Windows API.
static unsigned __stdcall taskStarter(void* arg);

// Thread parameter
class threadParam
{
public:
    Runnable* runnable;
    bool autoDelete;
};

Thread::Thread() :
    m_wasStarted(false),
    m_isRunning(false),
    m_handle(INVALID_HANDLE_VALUE),
    m_id(0)
{
}


Thread::~Thread()
{
    // Close the handle to the thread
    ::CloseHandle(m_handle);
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
    if (m_wasStarted)
    {
        throw SystemException("Thread already started");
    }

    threadParam* param = new threadParam();
    param->runnable = runnable;
    param->autoDelete = autoDelete;

    m_wasStarted = true;
    m_isRunning = true;

    // Using _beginthreadex() because CreateThread has a documented memory
    // leak with the CRT due to use of thread local variables.
    m_handle = (HANDLE)::_beginthreadex(NULL, // Default security attributes
        0,				// Default stack size
        taskStarter,	// Method to call
        (void*)param,	// Method argument
        0,				// Start immediately
        &m_id);			// pointer to id (will be set)

    if (m_handle == NULL)
    {
        throw SystemException(String("Failed to start thread: ") +
            WinUtil::getLastErrorMessage());
    }
}

void Thread::join()
{
    if (!m_isRunning)
    {
        throw SystemException("Failed to join thread: Thread not running");
    }

    DWORD ret = ::WaitForSingleObject(m_handle, INFINITE);

    if (ret != WAIT_OBJECT_0)
    {
        throw SystemException(String("Failed to join thread: ") +
            WinUtil::getLastErrorMessage());
    }

    m_isRunning = false;
}

/*
 * Function with the interface required by the Windows API. Simply calls
 * the run function of the passed Runnable object.
 */
static unsigned __stdcall taskStarter(void* arg)
{
    threadParam* param = (threadParam*)arg;

    try
    {
        param->runnable->run();
    }
    catch (...)
    {
        Console::errln("Uncaught exception");
        // TODO: Log
    }

    if (param->autoDelete)
        delete param->runnable;
    delete param;

    return 0;
}
