// AioServer.cpp

#include "ge/aio/AioServer.h"

AioServer::AioServer()
{

}

AioServer::~AioServer()
{

}

void AioServer::startServing(uint32 desiredThreads)
{

}

void AioServer::shutdown()
{

}

void AioServer::fileRead(AioFile* aioFile,
              fileCallback callback,
              void* userData,
              uint64 pos,
              char* buffer,
              uint32 bufferLen)
{

}

void AioServer::fileWrite(AioFile* aioFile,
               fileCallback callback,
               void* userData,
               uint64 pos,
               const char* buffer,
               uint32 bufferLen)
{

}

void AioServer::socketClose(AioSocket* aioSocket,
                 connectCallback callback,
                 void* userData)
{

}

void AioServer::socketAccept(AioSocket* listenSocket,
                  AioSocket* acceptSocket,
                  acceptCallback callback,
                  void* userData)
{

}

void AioServer::socketConnect(AioSocket* aioSocket,
                   connectCallback callback,
                   void* userData,
                   const INetAddress& address,
                   int32 port)
{

}

void AioServer::socketConnect(AioSocket* aioSocket,
                   connectCallback callback,
                   void* userData,
                   const INetAddress& address,
                   int32 port,
                   int32 timeout)
{

}

void AioServer::socketRead(AioSocket* aioSocket,
                socketCallback callback,
                void* userData,
                char* buffer,
                uint32 bufferLen)
{

}

void AioServer::socketWrite(AioSocket* aioSocket,
                 socketCallback callback,
                 void* userData,
                 const char* buffer,
                 uint32 bufferLen)
{

}

void AioServer::socketSendFile(AioSocket* aioSocket,
                    socketCallback callback,
                    void* userData,
                    AioFile* aioFile,
                    uint64 pos,
                    uint32 writeLen)
{

}

Error AioServer::addFile(AioFile* aioFile,
              const char* context)
{
    return Error();
}

Error AioServer::addSocket(AioSocket* aioSocket,
                const char* context)
{
    return Error();
}

void AioServer::dropFile(AioFile* aioFile)
{

}

void AioServer::dropSocket(AioSocket* aioSocket)
{

}

bool AioServer::process()
{
    return true;
}
