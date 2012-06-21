// Once.h

#ifndef ONCE_H
#define ONCE_H

#include <pthread.h>

// Typedef to make the same as the Windows version
typedef pthread_once_t Once_t;

// Define to make the same as the Windows call
#define call_once(x, y) pthread_once(&(x), (y))

#endif // ONCE_H
