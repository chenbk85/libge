// System.cpp

#include <ge/System.h>

#include <ge/text/UnicodeUtil.h>

#include <cstdio>
#include <winsock2.h>
#include <Windows.h>

typedef void (WINAPI *nativeSystemInfoFunc_t)(LPSYSTEM_INFO);

namespace System
{

bool initLibrary()
{
    WORD wVersionRequested = MAKEWORD(2, 0);
    WSADATA wsaData;

    int err = WSAStartup(wVersionRequested, &wsaData);

    if (err != 0)
    {
        // TODO: Improve error handling
        printf("WSAStartup failed with error: %d\n", err);
        return false;
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 0)
    {
        // TODO: Improve error handling
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return false;
    }
    
    return true;
}

void cleanupLibrary()
{
    WSACleanup();
}

List<String> convertCommandLine(int argc, char** argv)
{
    wchar_t* commandLine = ::GetCommandLineW();
    wchar_t** argv_wide = ::CommandLineToArgvW(commandLine, &argc);

    List<String> ret(argc);

    for (int i = 0; i < argc; i++)
    {
        ret.addBack(UnicodeUtil::utf16ToUtf8(argv_wide[i], wcslen(argv_wide[i])));
    }

    ::LocalFree(argv_wide);

    return ret;
}

bool isBigEndian()
{
    union
    {
        int32 intVal;
        char buf[4];
    } testUnion;

    testUnion.intVal = 1;

    return (bool)testUnion.buf[3];
}

String getEnv(String envName)
{
    wchar_t wideBuffer[128];
    char narrowBuffer[256];
    wchar_t* widePtr = wideBuffer;
    char* narrowPtr = narrowBuffer;

    //::GetEnvironmentVariableW(

    return "";
}

void setEnv(String envName, String value)
{

}

String getNativeEncoding()
{
    char buffer[14];
    ::_snprintf(buffer, sizeof(buffer), "CP%u", ::GetACP());
    return String(buffer);
}

String getOSName()
{
    String ret;
    OSVERSIONINFOEXW osVersionInfoEx;
    SYSTEM_INFO systemInfo;
    HMODULE kernel32Handle;
    nativeSystemInfoFunc_t nativeSystemInfoFunc;

    ::memset(&osVersionInfoEx, 0, sizeof(OSVERSIONINFOEXW));
    osVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    BOOL verSucc = ::GetVersionExW((OSVERSIONINFOW*)&osVersionInfoEx);

    if (!verSucc)
    {
        return "Windows Unknown";
    }

    ::memset(&systemInfo, 0, sizeof(SYSTEM_INFO));

    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise. If
    // we don't do this, we can be fooled by WOW64.
    kernel32Handle = GetModuleHandleW(L"kernel32.dll");

    if (kernel32Handle == NULL)
    {
        return "Windows Unknown";
    }

    nativeSystemInfoFunc = (nativeSystemInfoFunc_t)::GetProcAddress(kernel32Handle, "GetNativeSystemInfo");
    if (nativeSystemInfoFunc != NULL)
    {
        nativeSystemInfoFunc(&systemInfo);
    }
    else
    {
        ::GetSystemInfo(&systemInfo);
    }

    if (osVersionInfoEx.dwPlatformId == VER_PLATFORM_WIN32_NT && 
        osVersionInfoEx.dwMajorVersion > 4)
    {
        if (osVersionInfoEx.dwMajorVersion == 6)
        {
            if (osVersionInfoEx.dwMinorVersion == 0)
            {
                if (osVersionInfoEx.wProductType == VER_NT_WORKSTATION)
                {
                    return "Windows Vista";
                }
                else
                {
                    return "Windows Server 2008";
                }
            }
            else if (osVersionInfoEx.dwMinorVersion == 1)
            {
                if (osVersionInfoEx.wProductType == VER_NT_WORKSTATION)
                {
                    return "Windows 7";
                }
                else
                {
                    return "Windows Server 2008 R2";
                }
            }

            // Drop through
        }

        // Drop through
    }
    else if (osVersionInfoEx.dwMajorVersion == 5 &&
             osVersionInfoEx.dwMinorVersion == 2 )
    {
         if (::GetSystemMetrics(SM_SERVERR2))
         {
             return "Windows Server 2003 R2";
         }
         else if (osVersionInfoEx.wSuiteMask & VER_SUITE_STORAGE_SERVER)
         {
             return "Windows Storage Server 2003";
         }
         else if (osVersionInfoEx.wSuiteMask & VER_SUITE_WH_SERVER)
         {
             return "Windows Home Server";
         }
         else if (osVersionInfoEx.wProductType == VER_NT_WORKSTATION &&
                  systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
         {
             return "Windows XP Professional x64 Edition";
         }
         else
         {
             return "Windows Server 2003";
         }
    }
    else if (osVersionInfoEx.dwMajorVersion == 5 &&
             osVersionInfoEx.dwMinorVersion == 1)
    {
         return "Windows XP";
    }
    else if (osVersionInfoEx.dwMajorVersion == 5 &&
             osVersionInfoEx.dwMinorVersion == 0)
    {
         return "Windows 2000";
    }

    return "Windows Unknown";
}

} // End namespace System
