// Console.h

#ifndef CONSOLE_H
#define CONSOLE_H

#include <ge/text/String.h>
#include <ge/text/StringRef.h>

/*
 * Collection of functions for manipulating the console/terminal of the
 * program.
 */
namespace Console
{
    void out(const StringRef& str);

    void outln(const StringRef& str);

    void err(const StringRef& str);

    void errln(const StringRef& str);

    size_t read(char* buffer, size_t bufLen);

    void setLineBuffering(bool lineBuffering);
};

#endif // CONSOLE_H
