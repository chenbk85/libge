// System.cpp

#include "ge/System.h"

#include "ge/data/ShortList.h"

#include <langinfo.h>
#include <stdlib.h>
#include <sys/utsname.h>

bool System::initLibrary()
{

}

void System::cleanupLibrary()
{

}

bool System::isBigEndian()
{
    union
    {
        int32 intVal;
        char buf[4];
    } testUnion;

    testUnion.intVal = 1;

    return testUnion.buf[3];
}

String System::getEnv(const StringRef envName)
{
    ShortList<char, 256> nameBuffer(envName.length()+1);

    nameBuffer.addBlockBack(envName.data(), envName.length());
    nameBuffer.addBack('\0');

    const char* env = ::getenv(nameBuffer.data());

    if (env == NULL)
        return String();

    return String(env);
}

void System::setEnv(const StringRef envName,
                    const StringRef value)
{
    // TODO: Check if envName is empty or has an =

    ShortList<char, 256> nameBuffer(envName.length()+1);
    ShortList<char, 256> valueBuffer(value.length()+1);

    nameBuffer.addBlockBack(envName.data(), envName.length());
    nameBuffer.addBack('\0');

    valueBuffer.addBlockBack(value.data(), value.length());
    valueBuffer.addBack('\0');

    ::setenv(nameBuffer.data(), valueBuffer.data(), 1);
}

String System::getNativeEncoding()
{
    const char* cp = ::nl_langinfo(CODESET);

    return String(cp);
}

String System::getOSName()
{
    utsname utsvalue;

    ::uname(&utsvalue);

    return String(utsvalue.sysname);
}
