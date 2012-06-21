// Condition.h

#ifndef CONDITION_H
#define CONDITION_H

#include <ge/common.h>

#include <pthread.h>

/*
 * Waitable condition. Should be acquired before waiting or notifying. Can
 * be recursively acquired.
 */
class Condition
{
public:
	/*
	 * Creates the condition and its associated mutex.
	 */
	Condition();

	/*
	 * Deletes the condition and its associated mutex.
	 *
	 * NOTE: Will put the program into a bad state if there are threads
	 * waiting on the condition.
	 */
	~Condition();

	/*
	 * Locks the mutex used by this condition. You must lock the mutex in
	 * order to either wait on the condition, or signal the condition.
	 */
	void lock();

	/*
	 * Unlocks the mutex used by this event.
	 */
	void unlock();

	/*
	 * Signals a thread waiting on this condition. This restarts one of the
	 * threads waiting on this condition.
	 */
	void signal();

	/*
	 * Signals all threads waiting on this condition. This restarts every
	 * thread waiting on this condition.
	 */
	void signalAll();

	/*
	 * Causes the calling thread to wait on the condition until signaled.
	 */
	void wait();

	/*
	 * Same as wait(), but with a timeout. Returns an estimate of the
	 * time waited.
	 */
	uint32 wait(uint32 milliseconds);

private:
	Condition(const Condition& other);
	Condition& operator=(const Condition& other);

	void waitImpl(uint32 milliseconds);

private:
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
};

#endif // EVENT_H
