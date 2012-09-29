// AioSocket.h

#ifndef AIO_SOCKET_H
#define AIO_SOCKET_H

#ifdef __linux__
#include <gepriv/aio/AioSocketEpoll.h>
#else
#include <gepriv/aio/AioSocketPoll.h>
#endif

#endif // AIO_SOCKET_H
