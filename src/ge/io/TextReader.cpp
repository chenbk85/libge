// TextReader.cpp

#include <ge/io/TextReader.h>

#include <ge/util/Bool.h>

TextReader::TextReader(InputStream* inputStream)
{
    m_inputStream = inputStream;
}

TextReader::~TextReader()
{

}

String TextReader::readLine(bool* success)
{
    char buffer[1] = {0};
    int64 bytesRead = 1;
    String ret;

    success = false;

    while (true)
    {
        bytesRead = m_inputStream->read(buffer, 1);

        // Break at end of stream
        if (bytesRead < 0)
            break;

        Bool::setBool(success, true);

        // Eat a linefeed before a \n, but otherwise keep it
        if (buffer[0] == '\r')
        {
            m_inputStream->read(buffer, 1);
            if (buffer[0] == '\n')
                break;

            ret.appendChar('\r');
            ret.appendChar(buffer[0]);
            continue;
        }

        // Break at end of line
        if (buffer[0] == '\n')
            break;

        ret.appendChar(buffer[0]);
    }

    return ret;
}

String TextReader::readAll()
{
    char buffer[1024];
    int64 bytesRead = 1;
    String ret;

    while (true)
    {
        bytesRead = m_inputStream->read(buffer, 1024);

        if (bytesRead < 0)
            break;

        ret.append(buffer, (size_t)bytesRead);
    }

    // TODO: cleaner implementation
    //ret.replaceAll("\r\n", "\n");
    return ret;
}
