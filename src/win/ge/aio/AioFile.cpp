//AioFile.cpp

#include "ge/aio/AioFile.h"

#include "ge/aio/AioServer.h"
#include "ge/data/ShortList.h"
#include "ge/text/UnicodeUtil.h"
#include "gepriv/WinUtil.h"

AioFile::AioFile() :
    _handle(INVALID_HANDLE_VALUE),
    _owner(NULL)
{
}

AioFile::~AioFile()
{
    if (_owner != NULL)
    {
        // TODO: Log?
        _owner->dropFile(this);
    }

    close();
}

Error AioFile::open(StringRef fileName, OpenMode_Enum mode, int permissions)
{
    ShortList<wchar_t, 256> uniName;
    DWORD access = 0;
    DWORD creationDisposition = 0;

    UnicodeUtil::utf8ToUtf16(fileName.data(), fileName.length(), &uniName);
    uniName.addBack(L'\0');

    // It's legal, if normally pointless, to have no read or write access.
    // You can still access the file length.
    if (permissions & IO_READ_ACCESS)
    {
        access |= GENERIC_READ;
    }

    if (permissions & IO_WRITE_ACCESS)
    {
        access |= GENERIC_WRITE;
    }

    // Generate creation flag options
    switch (mode)
    {
    case OPEN_MODE_CREATE_ONLY:
        creationDisposition = CREATE_ALWAYS;
        break;
    case OPEN_MODE_CREATE_OR_OPEN:
    case OPEN_MODE_CREATE_OR_TRUNCATE: // Requires manual truncate
        creationDisposition = OPEN_ALWAYS;
        break;
    case OPEN_MODE_OPEN_ONLY:
        creationDisposition = OPEN_EXISTING;
        break;
    case OPEN_MODE_TRUNCATE_ONLY:
        creationDisposition = TRUNCATE_EXISTING;
        break;
    }

    _handle = ::CreateFileW(uniName.data(),
                            access,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            creationDisposition,
                            FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED,
                            NULL);

    DWORD err = ::GetLastError();

    if (_handle == INVALID_HANDLE_VALUE)
    {
        return WinUtil::getError(::GetLastError(),
            "CreateFileW",
            "AioFile::open");
    }

    // The OPEN_MODE_CREATE_OR_TRUNCATE requires manual truncation if we
    // open an existing file. Windows doesn't have a create or truncate
    // flag combination.
    if (mode == OPEN_MODE_CREATE_OR_TRUNCATE &&
        err == ERROR_ALREADY_EXISTS)
    {
        BOOL res = ::SetEndOfFile(_handle);

        if (!res)
        {
            return WinUtil::getError(::GetLastError(),
                "SetEndOfFile",
                "AioFile::open");
        }
    }

    // If Vista or later, change behavior to not queue IOCP entry if the
    // request completes immediately
#if (_WIN32_WINNT >= 0x0600)
    BOOL bret = ::SetFileCompletionNotificationModes(_handle,
            FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    if (!bret)
    {
        return WinUtil::getError(::GetLastError(),
                "SetFileCompletionNotificationModes",
                "AioFile::open");
    }
#endif

    return Error();
}

void AioFile::close()
{
    if (_handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(_handle);
    }

    _handle = INVALID_HANDLE_VALUE;
}
