
#include <ge/System.h>
#include <ge/io/Console.h>

int main()
{
    System::initLibrary();

    Console::outln("Hello World");

    System::cleanupLibrary();

    return 0;
}

