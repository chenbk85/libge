// SocketServiceEpoll.cpp

#ifdef __linux__

#include <gepriv/aio/SocketServiceEpoll.h>

SocketServiceEpoll::SocketServiceEpoll()
{

}

SocketServiceEpoll::~SocketServiceEpoll()
{

}

void SocketServiceEpoll::process()
{

}

void SocketServiceEpoll::shutdown()
{

}

void SocketServiceEpoll::submitClose(AioSocket* aioSocket,
                                     AioServer::connectCallback callback,
                                     void* userData)
{

}

void SocketServiceEpoll::submitAccept(AioSocket* listenSocket,
                                      AioSocket* acceptSocket,
                                      AioServer::acceptCallback callback,
                                      void* userData)
{

}

void SocketServiceEpoll::submitConnect(AioSocket* aioSocket,
                                       AioServer::connectCallback callback,
                                       void* userData,
                                       const INetAddress& address,
                                       int32 port)
{

}

void SocketServiceEpoll::submitConnect(AioSocket* aioSocket,
                                       AioServer::connectCallback callback,
                                       void* userData,
                                       const INetAddress& address,
                                       int32 port,
                                       int32 timeout)
{

}

void SocketServiceEpoll::socketRead(AioSocket* aioSocket,
                                    AioServer::socketCallback callback,
                                    void* userData,
                                    char* buffer,
                                    uint32 bufferLen)
{

}

void SocketServiceEpoll::socketWrite(AioSocket* aioSocket,
                                     AioServer::socketCallback callback,
                                     void* userData,
                                     const char* buffer,
                                     uint32 bufferLen)
{

}

void SocketServiceEpoll::socketSendFile(AioSocket* aioSocket,
                                        AioServer::socketCallback callback,
                                        void* userData,
                                        AioFile* aioFile,
                                        uint64 pos,
                                        uint32 writeLen)
{

}

#endif // #ifdef __linux__
