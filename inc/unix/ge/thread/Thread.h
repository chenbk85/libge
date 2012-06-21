// Thread.h

#ifndef THREAD_H
#define THREAD_H

#include <ge/common.h>
#include <ge/util/Runnable.h>

#include <pthread.h>

/*
 * Unix thread implementation. Wrapper around pthread functions.
 *
 * Currently requires an object that extends Runnable. Will probably change
 * to take a function object, but this requires a somewhat ugly template
 * parameter.
 */
class Thread
{
public:
    /*
     * Constructs the thread but does not start it.
     */
    Thread();

    /*
     * Destroying the thread object does not effect the running task.
     */
    ~Thread();

    /*
     * Starts the thread, running the passed Runnable as its activity.
     * If the Runnable needs to be deleted when the thread exits, you
     * should set the autoDelte parameter to true.
     */
    void start(Runnable* runnable, bool autoDelete);

    /*
     * Waits for the thread to finish
     */
    void join();

private:
    Thread(const Thread& other);
    Thread& operator=(const Thread& other);

private:
    bool m_wasStarted;      // Set to true if the thread was ever started
    bool m_isRunning;       // Set to true when the thread is started, false when joined
    pthread_t m_id;         // Thread id
};

#endif // UNIX_THREAD_H
