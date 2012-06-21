// Runnable.h

#ifndef RUNNABLE_H
#define RUNNABLE_H

/*
 * This is an abstract class for a task that can be run by a Thread,
 * ThreadPool or any other class following a similar pattern.
 *
 * The idea is that generally an activity that needs to be done in
 * another thread has some local data and a function that needs to be
 * run. When you have data and a method, you have a class.
 */
class EXPORT Runnable
{
public:
        virtual void run() = 0;
};

#endif // RUNNABLE_H

