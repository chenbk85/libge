// FileServiceBlocking.cpp

#include "gepriv/aio/FileServiceBlocking.h"

FileServiceBlocking::FileServiceBlocking()
{

}

FileServiceBlocking::~FileServiceBlocking()
{

}

void FileServiceBlocking::process()
{

}

void FileServiceBlocking::shutdown()
{

}

void FileServiceBlocking::submitRead(AioFile* aioFile,
                                     AioServer::fileCallback callback,
                                     void* userData,
                                     uint64 pos,
                                     char* buffer,
                                     uint32 bufferLen)
{

}

void FileServiceBlocking::submitWrite(AioFile* aioFile,
                                      AioServer::fileCallback callback,
                                      void* userData,
                                      uint64 pos,
                                      const char* buffer,
                                      uint32 bufferLen)
{

}
