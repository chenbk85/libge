// AioFile.h

#ifndef AIO_FILE_H
#define AIO_FILE_H

#ifdef __linux__
#include <gepriv/aio/AioFileLinux.h>
#else
#include <gepriv/aio/AioFileBlocking.h>
#endif

#endif // AIO_FILE_H
