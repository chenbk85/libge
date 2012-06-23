// System.h

#ifndef SYSTEM_H
#define SYSTEM_H

#include <ge/common.h>
#include <ge/data/List.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

namespace System
{
    /*
     * Initializes the library.
     */
    bool initLibrary();

    /*
     * Does cleanup of the library.
     */
    void cleanupLibrary();

    /*
     * Converts command line arguments from native encoding to UTF-8. Ignores
     * arguments on Windows and accesses the unicode arguments directly.
     */
    List<String> convertCommandLine(int argc, char** argv);

    /*
     * Returns true if the system is a big endian environment.
     */
    bool isBigEndian();

    /*
     * Returns the value of the passed environment variable.
     */
    String getEnv(const StringRef envName);

    /*
     * Sets the value of the passed environment variable.
     */
    void setEnv(const StringRef envName,
                const StringRef value);

    /*
     * Returns the current process's underlying 8-bit encoding
     */
    String getNativeEncoding();

    /*
     * Returns a string representing the current OS name.
     */
    String getOSName();
}

#endif // SYSTEM_H
