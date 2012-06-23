// RAFile.cpp

// Indicate to Linux headers that we can support 64 bit file offsets
#define _FILE_OFFSET_BITS 64

#include "ge/io/RAFile.h"

#include "ge/data/ShortList.h"
#include "ge/io/IOException.h"
#include "gepriv/UnixUtil.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * The implementation of the read/write of fundamental types in big or
 * little endian are written in such a way that the local endian-ness never
 * needs to be known. Optimization largely removes the unneeded memory moves.
 */

RAFile::RAFile() :
    m_handle(-1)
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

void RAFile::open(StringRef fileName, OpenMode_Enum mode, int32 permissions)
{
    // TODO: Properly convert filename
    size_t fileNameLen = fileName.length();

    ShortList<char, 256> charBuf(fileNameLen+1);

    charBuf.addBlockBack(fileName.data(), fileNameLen);
    charBuf.addBack('\0');

    // Don't inherit any fd
    int flags = O_CLOEXEC;

    // Build permission flags
    // It's legal, if normally pointless, to have no read or write access.
    // You can still access the file length.
    if ((permissions & IO_READ_ACCESS) &&
        (permissions & IO_WRITE_ACCESS))
    {
        flags |= O_RDWR;
    }
    else if (permissions & IO_READ_ACCESS)
    {
        flags |= O_WRONLY;
    }
    else if (permissions & IO_WRITE_ACCESS)
    {
        flags |= O_RDONLY;
    }

    // Build file creation flags
    switch (mode)
    {
    case OPEN_MODE_CREATE_ONLY:
        flags |= O_CREAT;
        flags |= O_EXCL;
        break;
    case OPEN_MODE_CREATE_OR_OPEN:
        flags |= O_CREAT;
        break;
    case OPEN_MODE_CREATE_OR_TRUNCATE: 
        flags |= O_CREAT;
        flags |= O_TRUNC;
        break;
    case OPEN_MODE_OPEN_ONLY:
        // No extra flags
        break;
    case OPEN_MODE_TRUNCATE_ONLY:
        flags |= O_TRUNC;
        break;
    }

    m_handle = UnixUtil::sys_open(charBuf.data(), flags, 0666);

    if (m_handle == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "open",
                                         "RAFile::open");
        throw IOException(error);
    }
}

void RAFile::close()
{
    if (m_handle != -1)
    {
        ::close(m_handle);
    }

    m_handle = -1;
}

bool RAFile::isOpen()
{
    return m_handle != -1;
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

    // TODO

    return ret;
}

uint32 RAFile::read(char* buffer, uint32 bufferLen)
{
    ssize_t res = UnixUtil::sys_read(m_handle, buffer, (size_t)bufferLen);

    if (res == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "read",
                                         "RAFile::read");
        throw IOException(error);
    }

    return (uint32)res;
}

void RAFile::readFully(char* buffer, uint32 bufferLen)
{
    size_t totalRead = 0;

    while (totalRead < (size_t)bufferLen)
    {
        ssize_t res = UnixUtil::sys_read(m_handle, buffer, (size_t)bufferLen);

        if (res == -1)
        {
            Error error = UnixUtil::getError(errno,
                                             "read",
                                             "RAFile::readFully");
            throw IOException(error);
        }

        totalRead += (size_t)res;
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

    writeBEUInt32(asUInt64);
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

    writeLEUInt32(asUInt64);
}

void RAFile::writeString(String value)
{
    // TODO
}

void RAFile::write(const char* buffer, uint32 bufferLen)
{
    int32 res = UnixUtil::sys_write(m_handle,
                                    buffer,
                                    bufferLen);

    if (res == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "write",
                                         "RAFile::write");
        throw IOException(error);
    }
}

void RAFile::seek(uint64 pos)
{
    off_t res = ::lseek(m_handle, (off_t)pos, SEEK_SET);

    if (res == (off_t)-1)
    {
        Error error = UnixUtil::getError(errno,
                                         "lseek",
                                         "RAFile::seek");
        throw IOException(error);
    }
}

uint64 RAFile::getFileOffset()
{
    off_t res = ::lseek(m_handle, 0, SEEK_CUR);

    if (res == (off_t)-1)
    {
        Error error = UnixUtil::getError(errno,
                                         "lseek",
                                         "RAFile::getFileOffset");
        throw IOException(error);
    }

    return res;
}

uint64 RAFile::fileLength()
{
    off_t oldPos = ::lseek(m_handle, 0, SEEK_CUR);

    if (oldPos == (off_t)-1)
    {
        Error error = UnixUtil::getError(errno,
                                         "lseek",
                                         "RAFile::fileLength");
        throw IOException(error);
    }

    off_t seekRes = ::lseek(m_handle, 0, SEEK_END);

    if (seekRes == (off_t)-1)
    {
        Error error = UnixUtil::getError(errno,
                                         "lseek",
                                         "RAFile::fileLength");
        throw IOException(error);
    }

    off_t resetPos = ::lseek(m_handle, oldPos, SEEK_SET);

    if (resetPos == (off_t)-1)
    {
        Error error = UnixUtil::getError(errno,
                                         "lseek",
                                         "RAFile::fileLength");
        throw IOException(error);
    }

    return seekRes;
}

void RAFile::setFileLength(uint64 length)
{
    int32 res = UnixUtil::sys_ftruncate(m_handle, length);

    if (res == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "ftruncate",
                                         "RAFile::setFileLength");
        throw IOException(error);
    }
}

void RAFile::sync()
{
    int res = ::fdatasync(m_handle);

    if (res == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "fdatasync",
                                         "RAFile::sync");
        throw IOException(error);
    }
}
