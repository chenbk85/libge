// Console.cpp

#include "ge/io/Console.h"

#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


static
bool g_consoleInit = false;

static
struct termios g_oldTio;

static void initConsole()
{
    /* get the terminal settings for stdin */
    ::tcgetattr(0, &g_oldTio);

    // TODO: Should acquire iconv_t here
    g_consoleInit = true;
}

static void writeFd(int fd,
                    const char* data,
                    size_t dataLen)
{
    // TODO: Should scan for high bit set, convert if needed
    ssize_t res = -1;

    do
    {
        res = ::write(fd, data, dataLen);
    } while (res == -1 && errno == EINTR);
}

void Console::out(const StringRef& str)
{
    if (!g_consoleInit)
        initConsole();

    writeFd(1, str.data(), str.length());
}

void Console::outln(const StringRef& str)
{
    static const char newline[1] = {'\n'};

    if (!g_consoleInit)
        initConsole();

    writeFd(1, str.data(), str.length());
    writeFd(1, newline, 1);
}

void Console::err(const StringRef& str)
{
    if (!g_consoleInit)
        initConsole();

    writeFd(2, str.data(), str.length());
}

void Console::errln(const StringRef& str)
{
    static const char newline[1] = {'\n'};

    if (!g_consoleInit)
        initConsole();

    writeFd(2, str.data(), str.length());
    writeFd(2, newline, 1);
}

size_t Console::read(char* buffer, size_t bufLen)
{
    ssize_t res = -1;

    do
    {
        res = ::read(0, buffer, bufLen);
    } while (res == -1 && errno == EINTR);

    return res;
}

void Console::setLineBuffering(bool lineBuffering)
{
    if (!g_consoleInit)
        initConsole();

    struct termios newTio;

    ::memcpy(&newTio, &g_oldTio, sizeof(struct termios));

    if (lineBuffering)
    {
        newTio.c_lflag |= ICANON;
    }
    else
    {
        newTio.c_lflag &= (~ICANON);
    }

    ::tcsetattr(0, TCSANOW, &newTio);
}
