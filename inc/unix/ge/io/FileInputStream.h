// FileInputStream.h

#ifndef FILE_INPUT_STREAM_H
#define FILE_INPUT_STREAM_H

#include <ge/common.h>
#include <ge/io/InputStream.h>
#include <ge/thread/Mutex.h>
#include <ge/text/String.h>

/*
 * Class that wraps a file handle for reading binary data. The input is
 * unbuffered except for OS level buffering. While the name matches a Java
 * class the functionality is not quite the same. The Java class is text
 * oriented, this reads raw bytes.
 */
class FileInputStream : public InputStream
{
friend class Process;

public:
	FileInputStream();
	~FileInputStream();

	void open(const String fileName); // TODO: Stringref

	uint32 available() OVERRIDE;
	void close() OVERRIDE;
	int32 read() OVERRIDE;
	int64 read(void* buffer, uint32 len) OVERRIDE;

private:
	explicit FileInputStream(int32 fileDescriptor);
	int64 internalRead(void* buffer, uint32 len);

	FileInputStream(const FileInputStream& other) DELETED;
	FileInputStream& operator=(const FileInputStream &other)  DELETED;

	int32 m_fileDescriptor;
};

#endif // FILE_INPUT_STREAM_H
