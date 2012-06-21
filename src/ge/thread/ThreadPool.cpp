// ThreadPool.cpp

#include "ge/thread/ThreadPool.h"

#include "ge/thread/Thread.h"
#include "ge/util/Locker.h"

#include <cstdio>

class ThreadPoolTask : public Runnable
{
public:
    ThreadPoolTask(ThreadPool* parent,
                   bool isMinThread) :
        m_parent(parent),
        m_minThread(isMinThread)
    {}

    ~ThreadPoolTask();

    void run()
    {
        ThreadPool::workData* work = NULL;

        do
        {
            work = m_parent->getNextWorkItem(!m_minThread);

            if (work != NULL)
            {
                work->runnable->run();

                if (work->autoDelete)
                    delete work->runnable;
                delete work;
            }

        } while (work != NULL);
    }

private:
    ThreadPool* m_parent;
    bool m_minThread;
};

ThreadPool::ThreadPool() :
    m_init(false),
    m_minThreads(0),
    m_maxThreads(0),
    m_timeout(0),
    m_queueMax(0),
    m_shutdown(false),
    m_runningThreads(0),
    m_idleThreads(0)
{
}

ThreadPool::ThreadPool(uint32 minThreads,
                       uint32 maxThreads,
                       uint32 threadTimeoutMilli,
                       uint32 queueMax) :
    m_init(true),
    m_minThreads(minThreads),
    m_maxThreads(maxThreads),
    m_timeout(threadTimeoutMilli),
    m_queueMax(queueMax),
    m_shutdown(false),
    m_runningThreads(0),
    m_idleThreads(0)
{
}

ThreadPool::~ThreadPool()
{
    shutdown();

    while (m_workQueue.size() > 0)
    {
        workData* data = m_workQueue.front();
        m_workQueue.popFront();

        if (data->autoDelete)
            delete data->runnable;
        delete data;
    }
}

void ThreadPool::init(uint32 minThreads,
                      uint32 maxThreads,
                      uint32 threadTimeoutMilli,
                      uint32 queueMax)
{
    if (m_init)
        return;

    m_init = true;
    m_minThreads = minThreads;
    m_maxThreads = maxThreads;
    m_timeout = threadTimeoutMilli;
    m_queueMax = queueMax;
}

bool ThreadPool::execute(Runnable* runnable, bool autoDelete)
{
    bool ret = false;

    Locker<Condition> locker(m_cond);

    if (m_queueMax != 0)
    {
        while (!m_shutdown &&
               m_workQueue.size() == m_queueMax)
        {
            m_cond.wait();
        }
    }

    if (!m_shutdown)
    {
        workData* newWorkData = new workData();
        newWorkData->runnable = runnable;
        newWorkData->autoDelete = autoDelete;
        m_workQueue.addBack(newWorkData);

        if (m_idleThreads == 0 &&
            m_runningThreads < m_maxThreads)
        {
            bool minThread = (m_runningThreads < m_minThreads);
            Thread thread;
            ThreadPoolTask* poolTask = new ThreadPoolTask(this, minThread);
            m_runningThreads++;
            thread.start(poolTask, true);
        }
        else
        {
            m_cond.signalAll();
        }
    }

    return ret;
}

uint32 ThreadPool::threadCount()
{
    Locker<Condition> locker(m_cond);
    return m_runningThreads;
}

size_t ThreadPool::queueSize()
{
    Locker<Condition> locker(m_cond);
    return (uint32)m_workQueue.size();
}

void ThreadPool::shutdown()
{
    Locker<Condition> locker(m_cond);

    m_shutdown = true;
    m_cond.signalAll();

    while (m_runningThreads > 0)
        m_cond.wait();
}

void ThreadPool::shutdownWhenEmpty()
{
    Locker<Condition> locker(m_cond);

    while (m_workQueue.size() > 0)
    {
        m_cond.wait();
    }

    m_shutdown = true;
    m_cond.signalAll();

    while (m_runningThreads > 0)
        m_cond.wait();
}

ThreadPool::workData* ThreadPool::getNextWorkItem(bool forbidTimeout)
{
    uint32 timeLeft = 1;
    workData* ret = NULL;

    Locker<Condition> locker(m_cond);

    if (m_timeout == 0 ||
        forbidTimeout)
    {
        while (!m_shutdown &&
                m_workQueue.size() == 0)
        {
            m_idleThreads++;
            m_cond.wait();
            m_idleThreads--;
        }
    }
    else
    {
        timeLeft = m_timeout;

        while (!m_shutdown &&
                m_workQueue.size() == 0 &&
                timeLeft > 0)
        {
            m_idleThreads++;
            uint32 waited = m_cond.wait((uint32)timeLeft);
            m_idleThreads--;

            if (waited > timeLeft)
            {
                timeLeft = 0;
            }
            else
            {
                timeLeft -= waited;
            }
        }
    }

    if (!m_shutdown &&
        timeLeft > 0)
    {
        ret = m_workQueue.front();
        m_workQueue.popFront();
    }
    else
    {
        m_runningThreads--;
    }

    m_cond.signalAll();

    return ret;
}
