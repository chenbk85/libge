// RAFile.cpp

#include <ge/io/RAFile.h>

#include <ge/data/ShortList.h>
#include <ge/io/IOException.h>
#include <ge/text/UnicodeUtil.h>

#include <gepriv/WinUtil.h>

/*
 * The implementation of the read/write of fundamental types in big or
 * little endian are written in such a way that the local endian-ness never
 * needs to be known. Optimization removes the unneeded memory moves.
 *
 * It's possible to open a file in Windows such that other programs cannot
 * open it. This is currently not done and we always allow shared access.
 */

RAFile::RAFile() :
    m_handle(INVALID_HANDLE_VALUE)
{
}

RAFile::~RAFile()
{

}

void RAFile::open(StringRef fileName)
{
    open(fileName, OPEN_MODE_CREATE_OR_OPEN,
         IO_READ_ACCESS |
         IO_WRITE_ACCESS);
}

void RAFile::open(StringRef fileName, OpenMode_Enum mode, int permissions)
{
    ShortList<wchar_t, 256> uniName;
    DWORD access = 0;
    DWORD creationDisposition = 0;

    UnicodeUtil::utf8ToUtf16(fileName.data(), fileName.length(), &uniName);

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

    m_handle = CreateFileW(uniName.data(),
                           access,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           creationDisposition,
                           FILE_FLAG_RANDOM_ACCESS,
                           NULL);

    DWORD err = ::GetLastError();

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException(WinUtil::getErrorMessage(err));
    }

    // The OPEN_MODE_CREATE_OR_TRUNCATE requires manual truncation if we
    // open an existing file. Windows doesn't have a create or truncate
    // flag combination.
    if (mode == OPEN_MODE_CREATE_OR_TRUNCATE &&
        err == ERROR_ALREADY_EXISTS)
    {
        BOOL res = ::SetEndOfFile(m_handle);

        if (!res)
        {
            throw IOException(WinUtil::getLastErrorMessage());
        }
    }
}

void RAFile::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_handle);
    }

    m_handle = INVALID_HANDLE_VALUE;
}

bool RAFile::isOpen()
{
    return m_handle != INVALID_HANDLE_VALUE;
}

bool RAFile::eof()
{
    return getFileOffset() == getFileLength();
}

int8 RAFile::readInt8()
{
    char buffer[1];
    readFully(buffer, 1);
    return (int8)buffer[0];
}

uint8 RAFile::readUInt8()
{
    char buffer[1];
    readFully(buffer, 1);
    return (uint8)buffer[0];
}

int16 RAFile::readBEInt16()
{
    unsigned char buffer[2];
    readFully((char*)buffer, 2);

    int16 ret = (buffer[0] << 8) |
                buffer[1];
    return ret;
}

int32 RAFile::readBEInt32()
{
    unsigned char buffer[4];
    readFully((char*)buffer, 4);

    int32 ret = 0;
    ret |= (buffer[0] << 24);
    ret |= (buffer[1] << 16);
    ret |= (buffer[2] << 8);
    ret |= buffer[3];
    return ret;
}

int64 RAFile::readBEInt64()
{
    unsigned char buffer[8];
    readFully((char*)buffer, 8);

    int64 ret = ((int64)buffer[0] << 56) | 
                ((int64)buffer[1] << 48) |
                ((int64)buffer[2] << 40) |
                ((int64)buffer[3] << 32) | 
                ((int64)buffer[4] << 24) |
                ((int64)buffer[5] << 16) | 
                ((int64)buffer[6] << 8) |
                (int64)buffer[7];
    return ret;
}

uint16 RAFile::readBEUInt16()
{
    unsigned char buffer[2];
    readFully((char*)buffer, 2);

    uint16 ret = (buffer[0] << 8) |
                 buffer[1];
    return ret;
}

uint32 RAFile::readBEUInt32()
{
    unsigned char buffer[4];
    readFully((char*)buffer, 4);

    uint32 ret = (buffer[0] << 24) | 
                 (buffer[1] << 16) |
                 (buffer[2] << 8) |
                 buffer[3];
    return ret;
}

uint64 RAFile::readBEUInt64()
{
    unsigned char buffer[8];
    readFully((char*)buffer, 8);

    uint64 ret = ((uint64)buffer[0] << 56) | 
                 ((uint64)buffer[1] << 48) |
                 ((uint64)buffer[2] << 40) |
                 ((uint64)buffer[3] << 32) | 
                 ((uint64)buffer[4] << 24) |
                 ((uint64)buffer[5] << 16) | 
                 ((uint64)buffer[6] << 8) |
                 (uint64)buffer[7];
    return ret;
}

float RAFile::readBEFloat()
{
    int32 asInt = readBEInt32();
    float ret;

    char* intPtr = (char*)&asInt;
    char* retPtr = (char*)&ret;

    retPtr[0] = intPtr[0];
    retPtr[1] = intPtr[1];
    retPtr[2] = intPtr[2];
    retPtr[3] = intPtr[3];

    return ret;
}

double RAFile::readBEDouble()
{
    int64 asInt = readBEInt64();
    double ret;

    char* intPtr = (char*)&asInt;
    char* retPtr = (char*)&ret;

    retPtr[0] = intPtr[0];
    retPtr[1] = intPtr[1];
    retPtr[2] = intPtr[2];
    retPtr[3] = intPtr[3];
    retPtr[4] = intPtr[4];
    retPtr[5] = intPtr[5];
    retPtr[6] = intPtr[6];
    retPtr[7] = intPtr[7];

    return ret;
}

int16 RAFile::readLEInt16()
{
    unsigned char buffer[2];
    readFully((char*)buffer, 2);

    int16 ret = (buffer[1] << 8) |
                buffer[0];
    return ret;
}

int32 RAFile::readLEInt32()
{
    unsigned char buffer[4];
    readFully((char*)buffer, 4);

    int32 ret = (buffer[3] << 24) | 
                (buffer[2] << 16) |
                (buffer[1] << 8) |
                buffer[0];
    return ret;
}

int64 RAFile::readLEInt64()
{
    unsigned char buffer[8];
    readFully((char*)buffer, 8);

    int64 ret = ((int64)buffer[7] << 56) | 
                ((int64)buffer[6] << 48) |
                ((int64)buffer[5] << 40) |
                ((int64)buffer[4] << 32) | 
                ((int64)buffer[3] << 24) |
                ((int64)buffer[2] << 16) | 
                ((int64)buffer[1] << 8) |
                (int64)buffer[0];
    return ret;
}

uint16 RAFile::readLEUInt16()
{
    unsigned char buffer[2];
    readFully((char*)buffer, 2);

    uint16 ret = (buffer[1] << 8) |
                 buffer[0];
    return ret;
}

uint32 RAFile::readLEUInt32()
{
    unsigned char buffer[4];
    readFully((char*)buffer, 4);

    uint32 ret = (buffer[3] << 24) | 
                 (buffer[2] << 16) |
                 (buffer[1] << 8) |
                 buffer[0];
    return ret;
}

uint64 RAFile::readLEUInt64()
{
    unsigned char buffer[8];
    readFully((char*)buffer, 8);

    uint64 ret = ((uint64)buffer[7] << 56) | 
                 ((uint64)buffer[6] << 48) |
                 ((uint64)buffer[5] << 40) |
                 ((uint64)buffer[4] << 32) | 
                 ((uint64)buffer[3] << 24) |
                 ((uint64)buffer[2] << 16) | 
                 ((uint64)buffer[1] << 8) |
                 (uint64)buffer[0];
    return ret;
}

float RAFile::readLEFloat()
{
    int32 asInt = readLEInt32();
    float ret;
    
    char* intPtr = (char*)&asInt;
    char* retPtr = (char*)&ret;

    retPtr[0] = intPtr[0];
    retPtr[1] = intPtr[1];
    retPtr[2] = intPtr[2];
    retPtr[3] = intPtr[3];

    return ret;
}

double RAFile::readLEDouble()
{
    int64 asInt = readLEInt64();
    double ret;

    char* intPtr = (char*)&asInt;
    char* retPtr = (char*)&ret;

    retPtr[0] = intPtr[0];
    retPtr[1] = intPtr[1];
    retPtr[2] = intPtr[2];
    retPtr[3] = intPtr[3];
    retPtr[4] = intPtr[4];
    retPtr[5] = intPtr[5];
    retPtr[6] = intPtr[6];
    retPtr[7] = intPtr[7];

    return ret;
}

String RAFile::readLine()
{
    String ret;
    char buffer[1];

    uint32 amountRead = read(buffer, 1);

    while (amountRead == 1 &&
           buffer[0] != '\n')
    {
        if (buffer[0] != '\r')
            ret.appendChar(buffer[0]);

        amountRead = read(buffer, 1);
    }

    return ret;
}

uint32 RAFile::read(char* buffer, uint32 bufferLen)
{
    DWORD amountRead;
    BOOL ret = ::ReadFile(m_handle,
                          buffer,
                          (DWORD)bufferLen,
                          &amountRead,
                          NULL);

    if (!ret)
    {
        DWORD err = ::GetLastError();

        if (err != ERROR_HANDLE_EOF)
        {
            throw IOException(WinUtil::getErrorMessage(err));
        }
    }

    return amountRead;
}

void RAFile::readFully(char* buffer, uint32 bufferLen)
{
    uint32 totalRead = 0;

    while (totalRead < bufferLen)
    {
        DWORD amountRead;

        BOOL ret = ::ReadFile(m_handle,
                              buffer + totalRead,
                              bufferLen - totalRead,
                              &amountRead,
                              NULL);

        if (!ret)
        {
            throw IOException(WinUtil::getLastErrorMessage());
        }

        totalRead += amountRead;
    }
}

void RAFile::writeInt8(int8 value)
{
    char buffer[1];
    buffer[0] = value;
    write(buffer, 1);
}

void RAFile::writeUInt8(uint8 value)
{
    char buffer[1];
    buffer[0] = (char)value;
    write(buffer, 1);
}

void RAFile::writeBEInt16(int16 value)
{
    char buffer[2];
    buffer[0] = (char)(value >> 8);
    buffer[1] = (char)value;
    write(buffer, 2);
}

void RAFile::writeBEInt32(int32 value)
{
    char buffer[4];
    buffer[0] = (char)(value >> 24);
    buffer[1] = (char)(value >> 16);
    buffer[2] = (char)(value >> 8);
    buffer[3] = (char)value;
    write(buffer, 4);
}

void RAFile::writeBEInt64(int64 value)
{
    char buffer[8];
    buffer[0] = (char)(value >> 56);
    buffer[1] = (char)(value >> 48);
    buffer[2] = (char)(value >> 40);
    buffer[3] = (char)(value >> 32);
    buffer[4] = (char)(value >> 24);
    buffer[5] = (char)(value >> 16);
    buffer[6] = (char)(value >> 8);
    buffer[7] = (char)value;
    write(buffer, 8);
}

void RAFile::writeBEUInt16(uint16 value)
{
    char buffer[2];
    buffer[0] = (char)(value >> 8);
    buffer[1] = (char)value;
    write(buffer, 2);
}

void RAFile::writeBEUInt32(uint32 value)
{
    char buffer[4];
    buffer[0] = (char)(value >> 24);
    buffer[1] = (char)(value >> 16);
    buffer[2] = (char)(value >> 8);
    buffer[3] = (char)value;
    write(buffer, 4);
}

void RAFile::writeBEUInt64(uint64 value)
{
    char buffer[8];
    buffer[0] = (char)(value >> 56);
    buffer[1] = (char)(value >> 48);
    buffer[2] = (char)(value >> 40);
    buffer[3] = (char)(value >> 32);
    buffer[4] = (char)(value >> 24);
    buffer[5] = (char)(value >> 16);
    buffer[6] = (char)(value >> 8);
    buffer[7] = (char)value;
    write(buffer, 8);
}

void RAFile::writeBEFloat(float value)
{
    uint32 asUInt32;

    char* uint32Ptr = (char*)&asUInt32;
    char* floatPtr = (char*)&value;

    uint32Ptr[0] = floatPtr[0];
    uint32Ptr[1] = floatPtr[1];
    uint32Ptr[2] = floatPtr[2];
    uint32Ptr[3] = floatPtr[3];

    writeBEUInt32(asUInt32);
}

void RAFile::writeBEDouble(double value)
{
    uint64 asUInt64;

    char* uint32Ptr = (char*)&asUInt64;
    char* doublePtr = (char*)&value;

    uint32Ptr[0] = doublePtr[0];
    uint32Ptr[1] = doublePtr[1];
    uint32Ptr[2] = doublePtr[2];
    uint32Ptr[3] = doublePtr[3];
    uint32Ptr[4] = doublePtr[4];
    uint32Ptr[5] = doublePtr[5];
    uint32Ptr[6] = doublePtr[6];
    uint32Ptr[7] = doublePtr[7];

    writeBEUInt64(asUInt64);
}

void RAFile::writeLEInt16(int16 value)
{
    char buffer[2];
    buffer[1] = (char)(value >> 8);
    buffer[0] = (char)value;
    write(buffer, 2);
}

void RAFile::writeLEInt32(int32 value)
{
    char buffer[4];
    buffer[4] = (char)(value >> 24);
    buffer[3] = (char)(value >> 16);
    buffer[2] = (char)(value >> 8);
    buffer[1] = (char)value;
    write(buffer, 4);
}

void RAFile::writeLEInt64(int64 value)
{
    char buffer[8];
    buffer[7] = (char)(value >> 56);
    buffer[6] = (char)(value >> 48);
    buffer[5] = (char)(value >> 40);
    buffer[4] = (char)(value >> 32);
    buffer[3] = (char)(value >> 24);
    buffer[2] = (char)(value >> 16);
    buffer[1] = (char)(value >> 8);
    buffer[0] = (char)value;
    write(buffer, 8);
}

void RAFile::writeLEUInt16(uint16 value)
{
    char buffer[2];
    buffer[1] = (char)(value >> 8);
    buffer[0] = (char)value;
    write(buffer, 2);
}

void RAFile::writeLEUInt32(uint32 value)
{
    char buffer[4];
    buffer[4] = (char)(value >> 24);
    buffer[3] = (char)(value >> 16);
    buffer[2] = (char)(value >> 8);
    buffer[1] = (char)value;
    write(buffer, 4);
}

void RAFile::writeLEUInt64(uint64 value)
{
    char buffer[8];
    buffer[7] = (char)(value >> 56);
    buffer[6] = (char)(value >> 48);
    buffer[5] = (char)(value >> 40);
    buffer[4] = (char)(value >> 32);
    buffer[3] = (char)(value >> 24);
    buffer[2] = (char)(value >> 16);
    buffer[1] = (char)(value >> 8);
    buffer[0] = (char)value;
    write(buffer, 8);
}

void RAFile::writeLEFloat(float value)
{
    uint32 asUInt32;

    char* uint32Ptr = (char*)&asUInt32;
    char* floatPtr = (char*)&value;

    uint32Ptr[0] = floatPtr[0];
    uint32Ptr[1] = floatPtr[1];
    uint32Ptr[2] = floatPtr[2];
    uint32Ptr[3] = floatPtr[3];

    writeLEUInt32(asUInt32);
}

void RAFile::writeLEDouble(double value)
{
    uint64 asUInt64;

    char* uint32Ptr = (char*)&asUInt64;
    char* doublePtr = (char*)&value;

    uint32Ptr[0] = doublePtr[0];
    uint32Ptr[1] = doublePtr[1];
    uint32Ptr[2] = doublePtr[2];
    uint32Ptr[3] = doublePtr[3];
    uint32Ptr[4] = doublePtr[4];
    uint32Ptr[5] = doublePtr[5];
    uint32Ptr[6] = doublePtr[6];
    uint32Ptr[7] = doublePtr[7];

    writeLEUInt64(asUInt64);
}

void RAFile::writeString(StringRef value)
{
    write(value.data(), value.length());
}

void RAFile::write(const char* buffer, uint32 bufferLen)
{
    DWORD written;

    BOOL ret = ::WriteFile(m_handle,
                           buffer,
                           bufferLen,
                           &written,
                           NULL);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }
}

void RAFile::seek(uint64 pos)
{
    LARGE_INTEGER offset;

    offset.QuadPart = pos;

    BOOL ret = ::SetFilePointerEx(m_handle,
                                 offset,
                                 NULL,
                                 FILE_BEGIN);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }
}

uint64 RAFile::getFileOffset()
{
    LARGE_INTEGER zeroOffset;
    LARGE_INTEGER curOffset;

    zeroOffset.QuadPart = 0;

    BOOL ret = ::SetFilePointerEx(m_handle,
                                  zeroOffset,
                                  &curOffset,
                                  FILE_CURRENT);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }

    return curOffset.QuadPart;
}

uint64 RAFile::getFileLength()
{
    LARGE_INTEGER largeSize;

    BOOL ret = ::GetFileSizeEx(m_handle, &largeSize);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }

    return largeSize.QuadPart;
}

void RAFile::setFileLength(uint64 length)
{
    LARGE_INTEGER zeroOffset;
    LARGE_INTEGER oldOffset;
    LARGE_INTEGER newOffset;
    BOOL ret;

    // ftruncate64 on unix

    // To extend the current size we have to seek. Seek once 0 distance to
    // get current position so we can reset to the old position.
    zeroOffset.QuadPart = 0;

    ret = ::SetFilePointerEx(m_handle,
                             zeroOffset,
                             &oldOffset,
                             FILE_CURRENT);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }

    newOffset.QuadPart = length;

    ret = ::SetFilePointerEx(m_handle,
                             newOffset,
                             NULL,
                             FILE_BEGIN);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }

    // Set the end of the file

    // NSetEndOfFile is more costly than SetFileValidData which does not
    // zero the file if extended. We could use that function, but the unix
    // ftruncate functions always zero the file and zeroed data is relatively
    // safer.
    ret = ::SetEndOfFile(m_handle);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }

    // Reset to old position
    ret = ::SetFilePointerEx(m_handle,
                             oldOffset,
                             NULL,
                             FILE_BEGIN);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }
}

void RAFile::sync()
{
    BOOL ret = ::FlushFileBuffers(m_handle);

    if (!ret)
    {
        throw IOException(WinUtil::getLastErrorMessage());
    }
}
