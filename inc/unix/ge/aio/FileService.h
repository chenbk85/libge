// FileService.h

#ifndef FILE_SERVICE_H
#define FILE_SERVICE_H

/*
 * Server that allows asynchronous operations on files.
 * 
 * When the server is started, it will create a set of threads for calling OS
 * asynchronous file IO functions. When the OS indicates that IO has completed,
 * a worker thread will trigger the passed user callback.
 *
 * As not every OS implements non-blocking file IO, and some operations such
 * as file appends tend to require blocking, some worker threads may be forced
 * to block. Make sure to adjust the pool size appropriately.
 */
#ifdef __linux__
#include <gepriv/aio/FileServiceLinux.h>
#else
#include <gepriv/aio/FileServiceBlocking.h>
#endif

#endif // FILE_SERVICE_H
