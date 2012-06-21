// Mutex.h

#ifndef MUTEX_H
#define MUTEX_H

#include <ge/common.h>

#include <pthread.h>

/*
 * Recursive lock. Behaves like a recursive pthread_mutex_t.
 */
class Mutex
{
public:
	Mutex(void);
	~Mutex(void);

	void lock();
	bool trylock();
	void unlock();

private:
	Mutex(const Mutex& other);
	Mutex& operator=(const Mutex& other);

private:
	pthread_mutex_t m_mutex;
};

#endif // MUTEX_H
