// SocketService.h

#ifndef SOCKET_SERVICE_H
#define SOCKET_SERVICE_H

#include <ge/common.h>

/*
 * Server that allows asynchronous operations on sockets.
 * 
 * When the server is started, it will create a set of threads for calling OS
 * asynchronous file IO functions. When the OS indicates that IO has completed,
 * a worker thread will trigger the passed user callback.
 *
 * Note that on some systems the sendfile functionality may be emulated using
 * blocking io.
 */
#ifdef __linux__
#include <gepriv/aio/SocketServiceEpoll.h>
#else
#include <gepriv/aio/SocketServicePoll.h>
#endif

#endif // SOCKET_SERVICE_H
