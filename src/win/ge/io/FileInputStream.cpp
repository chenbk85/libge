// FileInputStream.cpp

#include <ge/io/FileInputStream.h>

#include <ge/data/ShortList.h>
#include <ge/io/IOException.h>
#include <ge/text/UnicodeUtil.h>
#include <ge/util/Locker.h>

#include <gepriv/WinUtil.h>


FileInputStream::FileInputStream()
{
    m_handle = INVALID_HANDLE_VALUE;
}

FileInputStream::FileInputStream(HANDLE handle)
{
    m_handle = handle;
}

FileInputStream::~FileInputStream()
{
    close();
}

FileInputStream::FileInputStream(FileInputStream&& other)
{
    m_handle = other.m_handle;
    other.m_handle = INVALID_HANDLE_VALUE;
}

FileInputStream& FileInputStream::operator=(FileInputStream&& other)
{
    if (this != &other)
    {
        new (this) FileInputStream(std::move(other));
    }

    return *this;
}

void FileInputStream::open(const String fileName)
{
    ShortList<wchar_t, 256> convList(fileName.length());

    if (m_handle != INVALID_HANDLE_VALUE)
    {
        throw IOException("File already open");
    }

    UnicodeUtil::utf8ToUtf16(fileName.data(), fileName.length(), &convList);
    convList.addBack(L'\0');

    m_handle = ::CreateFileW(convList.data(), // File name
                             GENERIC_READ, // We only need to read
                             FILE_SHARE_READ | FILE_SHARE_WRITE, // Only block deletion
                             NULL, // Default security attributes
                             OPEN_EXISTING, // Only open if it exists (don't create)
                             FILE_FLAG_SEQUENTIAL_SCAN, // Flag that we will scan sequentially
                             NULL); // No template file handle

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException(String("Failed to open \"") + fileName +
            "\" : " + WinUtil::getLastErrorMessage());
    }
}

uint32 FileInputStream::available()
{
    // If pipe, return available bytes
    DWORD fileType = ::GetFileType(m_handle);

    if (fileType == FILE_TYPE_PIPE)
    {
        DWORD bytesAvailable = 0;

        BOOL peekRet = ::PeekNamedPipe(m_handle, // Handle
                NULL, // Buffer to read into (not reading)
                0, // Buffer size
                NULL, // Bytes read
                &bytesAvailable, // Total bytes available
                NULL); // Bytes left this message

        if (!peekRet)
        {
            // Just return 0 if it fails. It's typically due to EOF.
            return 0;
        }

        return (uint32)bytesAvailable;
    }

    return 0;
}

void FileInputStream::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

int32 FileInputStream::read()
{
    char tempBuffer[1];
    int64 ret = internalRead(tempBuffer, 1);

    if (ret == -1)
        return -1;
    return tempBuffer[0];
}

int64 FileInputStream::read(void *buffer, uint32 len)
{
    return internalRead((int8*)buffer, len);
}

int64 FileInputStream::internalRead(int8* buffer, uint32 len)
{
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException("Cannot read from closed stream");
    }

    DWORD bytesRead = 1;
    uint32 readSuccess = true;

    ::SetLastError(ERROR_SUCCESS);

    readSuccess = ::ReadFile(m_handle, // The handle to read from
                             buffer, // Pointer to our buffer
                             len, // Bytes to try to read
                             &bytesRead, // ReadFile will set the number of bytes read
                             NULL); // Not using asynchronous I/O

    // If we failed to read anything, check for closed pipe
    if (!readSuccess || bytesRead == 0)
    {
        uint32 lastError = ::GetLastError();
        if (lastError == ERROR_SUCCESS ||
            lastError == ERROR_BROKEN_PIPE ||
            lastError == ERROR_HANDLE_EOF)
        {
            return -1; // pipe done - normal exit path.
        }
        else
        {
            throw IOException(String("Failed to read stream: ") +
                WinUtil::getLastErrorMessage());
        }
    }

    return bytesRead;
}
