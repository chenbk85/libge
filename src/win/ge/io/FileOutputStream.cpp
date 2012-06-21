// FileOutputStream.cpp

#include <ge/io/FileOutputStream.h>

#include <ge/data/ShortList.h>
#include <ge/io/IOException.h>
#include <ge/text/UnicodeUtil.h>
#include <ge/util/Locker.h>

#include <gepriv/WinUtil.h>


FileOutputStream::FileOutputStream()
{
    m_handle = INVALID_HANDLE_VALUE;
    m_append = false;
}

FileOutputStream::FileOutputStream(HANDLE handle)
{
    m_handle = handle;
    m_append = false; // Cannot inherit the append mode
}

FileOutputStream::~FileOutputStream()
{
    close();
}

FileOutputStream::FileOutputStream(FileOutputStream&& other)
{
    m_handle = other.m_handle;
    m_append = other.m_append;
    ::memcpy(&m_overlapped, &other.m_overlapped, sizeof(OVERLAPPED));

    other.m_handle = INVALID_HANDLE_VALUE;
    ::memset(&other.m_overlapped, 0, sizeof(OVERLAPPED));
}

FileOutputStream& FileOutputStream::operator=(FileOutputStream&& other)
{
    if (this != &other)
    {
        new (this) FileOutputStream(std::move(other));
    }

    return *this;
}

void FileOutputStream::open(const String fileName)
{
    open(fileName, false);
}

void FileOutputStream::open(const String fileName, bool append)
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        throw IOException("File already open");
    }

    m_append = append;

    ShortList<wchar_t, 256> convList(fileName.length());

    UnicodeUtil::utf8ToUtf16(fileName.data(), fileName.length(), &convList);
    convList.addBack(L'\0');

    // Change how we open the file depending on append flag
    if (append)
    {
        m_handle = ::CreateFileW(convList.data(), // File name
                                 GENERIC_WRITE, // We only need to write
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, // Only block deletion
                                 NULL, // Default security attributes
                                 OPEN_ALWAYS, // Creates or opens the existing file
                                 FILE_FLAG_OVERLAPPED, // Writing with overlapped struct
                                 NULL); // No template file handle

        if (m_handle == INVALID_HANDLE_VALUE)
        {
            throw IOException(String("Failed to open file: ") + WinUtil::getLastErrorMessage());
        }

        // Create an OVERLAPPED struct
        OVERLAPPED overlapped;
        ::memset(&overlapped, 0, sizeof(OVERLAPPED));

        // Setting both Offset and OffsetHigh to 0xFFFFFFFF means "append"
        overlapped.Offset = 0xFFFFFFFF;
        overlapped.OffsetHigh = 0xFFFFFFFF;

        // Create an Event for the OVERLAPPED object to signal completion
        overlapped.hEvent = ::CreateEventW(NULL, // Default security
                                           TRUE, // Manual reset event (required for WriteFile)
                                           FALSE, // Start unsignaled (required for WriteFile)
                                           NULL); // Unnamed event

        if (overlapped.hEvent == NULL)
        {
            uint32 lastError = ::GetLastError();
            throw IOException(WinUtil::getErrorMessage(lastError));
        }
    }
    else
    {
        m_handle = ::CreateFileW(convList.data(), // File name
                                 GENERIC_WRITE, // We only need to write
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, // Only block deletion
                                 NULL, // Default security attributes
                                 OPEN_ALWAYS, // Creates or opens the existing file
                                 FILE_ATTRIBUTE_NORMAL, // Normal synchronous writes
                                 NULL); // No template file handle

        if (m_handle == INVALID_HANDLE_VALUE)
        {
            throw IOException(String("Failed to open file: ") + WinUtil::getLastErrorMessage());
        }
    }
}

void FileOutputStream::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;

        if (m_append)
        {
            ::CloseHandle(m_overlapped.hEvent);
        }
    }
}

int32 FileOutputStream::write(int32 byte)
{
    char tempBuffer[1];
    tempBuffer[0] = (char)(byte & 0x000000ff);

    if (m_append)
        return (int32)writeAppend(tempBuffer, 1);
    return (int32)writeNormal(tempBuffer, 1);
}

int64 FileOutputStream::write(const void* buffer, uint32 maxlen)
{
    if (m_append)
        return writeAppend(buffer, maxlen);
    return writeNormal(buffer, maxlen);
}

int64 FileOutputStream::writeNormal(const void* buffer, uint32 maxlen)
{
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException("Cannot write to closed stream");
    }

    DWORD bytesWritten = 0;
    uint32 writeSuccess;

    ::SetLastError(ERROR_SUCCESS);

    writeSuccess = ::WriteFile(m_handle, // The handle to write to
                               buffer, // The buffer of bytes to write
                               maxlen, // The number of bytes in the buffer
                               &bytesWritten, // WriteFile will set the number of bytes written
                               NULL); // Not using asynchronous I/O

    // If we failed to write anything, check for closed pipe
    if (!writeSuccess)
    {
        uint32 lastError = ::GetLastError();
        if (lastError == ERROR_SUCCESS ||
            lastError == ERROR_BROKEN_PIPE)
        {
            return -1;
        }
        else
        {
            throw IOException(String("Failed to write to stream: ") +
                WinUtil::getLastErrorMessage());
        }
    }
    return bytesWritten;
}

/*
 * Doing synchronous "atomic" appends to a file is a bit of a pain in
 * Windows. A bit of research showed that Java uses an undocumented method
 * of specifying FILE_APPEND_DATA access without FILE_WRITE_DATA
 * (see io_util_md.c) but this created issues because the removal of
 * FILE_WRITE_DATA permissions caused file locking and file truncation to
 * fail.
 *
 * Instead of trying some funky undocumented method of appending
 * synchronously, we use a documented method with asynchronous IO where we
 * ask it to append to the file by specifying an offset of 0xFFFFFFFFFFFFFFFF.
 * We then call GetOverlappedResult with a blocking flag so the function will
 * not return until the IO is complete.
 */
int64 FileOutputStream::writeAppend(const void* buffer, uint32 maxlen)
{
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException("Cannot write to closed stream");
    }

    ::SetLastError(ERROR_SUCCESS);

    // Start the asynchronous write to the file
    uint32 writeSuccess;

    writeSuccess = ::WriteFile(m_handle, // The handle to write to
                               buffer, // The buffer of bytes to write
                               maxlen, // The number of bytes in the buffer
                               NULL, // Can't get bytes written with asynchronous IO, see below
                               &m_overlapped); // Reference to OVERLAPPED struct

    if (!writeSuccess)
    {
        uint32 lastError = ::GetLastError();

        // WriteFile sometimes completes before returning and sometimes returns 0 and
        // sets the last error of the calling thread. ERROR_IO_PENDING indicates the
        // write is happily proceeding asynchronously.

        if (lastError != ERROR_IO_PENDING)
        {
            // ERROR_HANDLE_EOF just indicates we are at end of file.
            if (lastError == ERROR_HANDLE_EOF)
            {
                return -1;
            }
            else
            {
                throw IOException(WinUtil::getErrorMessage(lastError));
            }
        }
    }

    // Since we are using asynchronous IO we need to wait for the operation to complete.
    DWORD bytesWritten = 0;

    writeSuccess = ::GetOverlappedResult(m_handle, // Handle to wait on
                                         &m_overlapped, // Reference to OVERLAPPED struct
                                         &bytesWritten, // Pointer to int put bytes written into
                                         TRUE); // Block until done writing

    if (!writeSuccess)
    {
        uint32 lastError = ::GetLastError();

        // ERROR_HANDLE_EOF just indicates we are at end of file.
        if (lastError == ERROR_HANDLE_EOF)
        {
            return -1;
        }
        else
        {
            throw IOException(WinUtil::getErrorMessage(lastError));
        }
    }

    return bytesWritten;
}
