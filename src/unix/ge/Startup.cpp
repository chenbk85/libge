// Startup.cpp

#include "ge/Startup.h"

#include <ge/text/StringRef.h>
#include "ge/util/WinUtil.h"

#include <cstdio>


// UTF-8 parameter data
static char* utf8_paramData;

/*
 * Windows version completely ignores argc and argv.
 */
Array<StringRef> Startup::initGoodEnough(int argc, char** argv)
{
    int utf8_argvLen = 0;
    Array<StringRef> ret(argc);

    // Sum how much space the arguments will take up in UTF-8
    for (int i = 0; i < argc; i++)
    {
        int convRet = ::WideCharToMultiByte(CP_UTF8, // Code page
            0, // Flags
            argv_wide[i], // String to size
            -1, // Detect length
            NULL, // Dest string
            0, // Dest string len
            NULL, // Default char
            NULL); // BOOL set if default char used

        if (convRet == 0)
        {
            // TODO: Better error
            ::fprintf(stderr, "Failed to convert arguments to UTF-8\n");
            return ret;
        }

        utf8_argvLen += convRet;
    }

    // Convert the arguments to UTF-8. We store all the converted data in a
    // single array, with StringRefs pointing to it.
    utf8_paramData = new char[utf8_argvLen];
    int paramDataIndex = 0;

    for (int i = 0; i < argc; i++)
    {
        int convRet = ::WideCharToMultiByte(CP_UTF8, // Code page
            0, // Flags
            argv_wide[i], // String to size
            -1, // Detect length
            utf8_paramData + paramDataIndex, // Dest string
            utf8_argvLen - paramDataIndex, // Dest string len
            NULL, // Default char
            NULL); // BOOL set if default char used

        if (convRet == 0)
        {
            // TODO: Better error
            ::fprintf(stderr, "Failed to convert arguments to UTF-8\n");
            return ret;
        }

        ret.set(i, StringRef(utf8_paramData + paramDataIndex, convRet));
        paramDataIndex += convRet;
    }

    return ret;
}

void Startup::cleanupGoodEnough()
{
    ::LocalFree(argv_wide);
    delete[] utf8_paramData;
}
