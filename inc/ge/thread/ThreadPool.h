//ThreadPool.h

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <ge/common.h>
#include <ge/data/DLinkedList.h>
#include <ge/thread/Condition.h>
#include <ge/util/Runnable.h>

class ThreadPool
{
    friend class ThreadPoolTask;

public:
    ThreadPool();
    ThreadPool(uint32 minThreads,
               uint32 maxThreads,
               uint32 threadTimeoutMilli,
               uint32 queueMax);
    ~ThreadPool();

    void init(uint32 minThreads,
              uint32 maxThreads,
              uint32 threadTimeoutMilli,
              uint32 queueMax);

    bool execute(Runnable* runnable, bool autoDelete);

    uint32 threadCount();
    uint32 queueSize();

    void shutdown();
    void shutdownWhenEmpty();

private:
    struct workData
    {
        Runnable* runnable;
        bool autoDelete;
    };

    workData* getNextWorkItem(bool forbidTimeout);

    // Set once, then read only
    bool m_init;
    uint32 m_minThreads;
    uint32 m_maxThreads;
    uint32 m_timeout;
    uint32 m_queueMax;

    // Guarded by m_cond
    Condition m_cond;
    bool m_shutdown;
    uint32 m_runningThreads;
    uint32 m_idleThreads;
    DLinkedList<workData*> m_workQueue;
};

#endif /* THREAD_POOL_H */
