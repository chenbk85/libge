// Console.cpp

#include "ge/io/Console.h"

#include "ge/data/ShortList.h"
#include "ge/text/UnicodeUtil.h"

#include <Windows.h>

static bool g_consoleInit = false;
static HANDLE g_stdIn = INVALID_HANDLE_VALUE;
static HANDLE g_stdOut = INVALID_HANDLE_VALUE;
static HANDLE g_stdErr = INVALID_HANDLE_VALUE;
static bool g_stdInIsConsole = false;
static bool g_stdOutIsConsole = false;
static bool g_stdErrIsConsole = false;

static void initConsole()
{
    DWORD mode;

    g_stdIn = ::GetStdHandle(STD_INPUT_HANDLE);
    g_stdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    g_stdErr = ::GetStdHandle(STD_ERROR_HANDLE);

    // If GetConsoleMode succeeds, the handles are to a real console
    g_stdInIsConsole = (bool)::GetConsoleMode(g_stdIn, &mode);
    g_stdOutIsConsole = (bool)::GetConsoleMode(g_stdOut, &mode);
    g_stdErrIsConsole = (bool)::GetConsoleMode(g_stdErr, &mode);

    g_consoleInit = true;
}

static void writeHandle(HANDLE handle,
                        const char* data,
                        size_t dataLen,
                        bool isConsole)
{
    DWORD written;

    if (g_stdOutIsConsole)
    {
        // ANSI version is fine as code page is UTF-8
        ::WriteConsoleA(g_stdOut,
                        data,
                        dataLen,
                        &written,
                        NULL);
    }
    else
    {
        ::WriteFile(g_stdOut,
                    data,
                    dataLen,
                    &written,
                    NULL);
    }
}


void Console::out(const StringRef& str)
{
    if (!g_consoleInit)
        initConsole();

    writeHandle(g_stdOut, str.data(), str.length(), g_stdOutIsConsole);
}

void Console::outln(const StringRef& str)
{
    static const char newline[1] = {'\n'};

    if (!g_consoleInit)
        initConsole();

    writeHandle(g_stdOut, str.data(), str.length(), g_stdOutIsConsole);
    writeHandle(g_stdOut, newline, 1, g_stdOutIsConsole);
}

void Console::err(const StringRef& str)
{
    if (!g_consoleInit)
        initConsole();

    writeHandle(g_stdErr, str.data(), str.length(), g_stdErrIsConsole);
}

void Console::errln(const StringRef& str)
{
    static const char newline[1] = {'\n'};

    if (!g_consoleInit)
        initConsole();

    writeHandle(g_stdErr, str.data(), str.length(), g_stdErrIsConsole);
    writeHandle(g_stdErr, newline, 1, g_stdErrIsConsole);
}

size_t Console::read(char* buffer, size_t bufLen)
{
    if (!g_consoleInit)
        initConsole();

    DWORD read;
    BOOL res;

    if (g_stdInIsConsole)
    {
        res = ::ReadConsoleA(g_stdIn,
                             (void*)buffer,
                             bufLen,
                             &read,
                             NULL);
    }
    else
    {
        res = ::ReadFile(g_stdIn,
                         (void*)buffer,
                         bufLen,
                         &read,
                         NULL);
    }

    if (res)
        return read;
    return 0;
}

void Console::setLineBuffering(bool lineBuffering)
{
    if (!g_consoleInit)
        initConsole();

    DWORD outMode;
    DWORD errMode;
    BOOL bRet;

    bRet = ::GetConsoleMode(g_stdOut, &outMode);

    if (!bRet)
    {
        // TODO: Handle
    }

    bRet = ::GetConsoleMode(g_stdErr, &errMode);

    if (!bRet)
    {
        // TODO: Handle
    }

    if (lineBuffering)
    {
        outMode |= ENABLE_LINE_INPUT;
        errMode |= ENABLE_LINE_INPUT;
    }
    else
    {
        outMode &= ~ENABLE_LINE_INPUT;
        errMode &= ~ENABLE_LINE_INPUT;
    }

    bRet = ::SetConsoleMode(g_stdOut, outMode);

    if (!bRet)
    {
        // TODO: Handle
    }

    bRet = ::SetConsoleMode(g_stdErr, errMode);

    if (!bRet)
    {
        // TODO: Handle
    }
}
