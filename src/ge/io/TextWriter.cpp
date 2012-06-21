// TextWriter.cpp

#include <ge/io/TextWriter.h>

TextWriter::TextWriter(OutputStream* outputStream)
{
    m_outputStream = outputStream;
}

TextWriter::~TextWriter()
{

}

void TextWriter::write(String str)
{
    m_outputStream->write(str.data(), str.length());
}