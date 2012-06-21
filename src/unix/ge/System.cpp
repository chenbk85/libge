// System.cpp

#include "ge/System.h"

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

String System::getEnv(String envName)
{
    return "";
}

void System::setEnv(String envName, String value)
{

}

String System::getNativeEncoding()
{
    return "";
}

String System::getOSName()
{
    return "";
}
