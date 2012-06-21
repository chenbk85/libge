// RAFile.h

#ifndef RA_FILE_H
#define RA_FILE_H

#include <ge/common.h>
#include <ge/io/IO.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

#include <Windows.h>

/*
 * Class representing a random access file.
 *
 * TODO: Implement InputSteam and OutputStream?
 */
class RAFile
{
public:
    RAFile();
    ~RAFile();

    void open(StringRef fileName);
    void open(StringRef fileName, OpenMode_Enum mode, int permissions);
    void close();

    bool isOpen();
    bool eof();

    int8 readInt8();
    uint8 readUInt8();

    int16 readBEInt16();
    int32 readBEInt32();
    int64 readBEInt64();

    uint16 readBEUInt16();
    uint32 readBEUInt32();
    uint64 readBEUInt64();

    float readBEFloat();
    double readBEDouble();

    int16 readLEInt16();
    int32 readLEInt32();
    int64 readLEInt64();

    uint16 readLEUInt16();
    uint32 readLEUInt32();
    uint64 readLEUInt64();

    float readLEFloat();
    double readLEDouble();

    String readLine();

    uint32 read(char* buffer, uint32 bufferLen);
    void readFully(char* buffer, uint32 bufferLen);

    void writeInt8(int8 value);
    void writeUInt8(uint8 value);

    void writeBEInt16(int16 value);
    void writeBEInt32(int32 value);
    void writeBEInt64(int64 value);

    void writeBEUInt16(uint16 value);
    void writeBEUInt32(uint32 value);
    void writeBEUInt64(uint64 value);

    void writeBEFloat(float value);
    void writeBEDouble(double value);

    void writeLEInt16(int16 value);
    void writeLEInt32(int32 value);
    void writeLEInt64(int64 value);

    void writeLEUInt16(uint16 value);
    void writeLEUInt32(uint32 value);
    void writeLEUInt64(uint64 value);

    void writeLEFloat(float value);
    void writeLEDouble(double value);

    void writeString(StringRef value);

    void write(const char* buffer, uint32 bufferLen);

    void seek(uint64 pos);
    uint64 getFileOffset();

    uint64 getFileLength();
    void setFileLength(uint64 length);

    void sync();

private:
    RAFile(const RAFile& other) DELETED;
    RAFile& operator=(const RAFile& other) DELETED;

    HANDLE m_handle;
};

#endif // RA_FILE_H
