// TextWriter.h

#ifndef TEXT_WRITER_H
#define TEXT_WRITER_H

#include <ge/io/OutputStream.h>
#include <ge/text/String.h>

/*
 * For now this class is a simplistic wrapper around an OutputStream. It's
 * basically a stub to add conversion from some other encoding to unicode,
 * if unicode support is added.
 */
class TextWriter
{
public:
	TextWriter(OutputStream* outputStream);
	~TextWriter();

	/*
	 * Writes a String to the stream.
	 */
	void write(String str);

private:
	OutputStream* m_outputStream;
};

#endif // TEXT_WRITER_H
