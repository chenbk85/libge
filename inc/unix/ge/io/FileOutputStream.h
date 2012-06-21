// FileOutputStream.h

#ifndef FILE_OUTPUT_STREAM_H
#define FILE_OUTPUT_STREAM_H

#include <ge/common.h>
#include "ge/io/OutputStream.h"
#include <ge/text/String.h>
#include <ge/thread/Mutex.h>

/*
 * Class that wraps a file handle for writing binary data. Output is
 * unbuffered except for OS level buffering. 
 *
 * Optionally allows a file to be opened in append mode. The appends
 * will be atomic for multiple processes unless the file is across a
 * network file system (NFS) which does not support atomic appending.
 *
 * While the name matches a Java class the functionality is not quite the
 * same. The Java class is text oriented. This supports only raw binary.
 */
class FileOutputStream : public OutputStream
{
friend class Process;

public:
	FileOutputStream();
	~FileOutputStream();

	void open(const String fileName);
	void open(const String fileName, bool append);

	void close() OVERRIDE;
	int32 write(int32 character) OVERRIDE;
	int64 write(const void* buffer, uint32 maxlen) OVERRIDE;

private:
	explicit FileOutputStream(int32 fileDescriptor);
	void init(const String fileName, bool append);
	int64 internalWrite(const void* buffer, uint32 maxlen);

	FileOutputStream(const FileOutputStream& other) DELETED;
	FileOutputStream& operator=(const FileOutputStream& other) DELETED;

private:
	int32 m_fileDescriptor;
};

#endif // FILE_OUTPUT_STREAM_H
