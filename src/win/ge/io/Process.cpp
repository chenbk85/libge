// Process.cpp

#include <ge/io/Process.h>

#include <ge/SystemException.h>
#include <ge/data/ShortList.h>
#include <ge/io/IOException.h>
#include <ge/text/UnicodeUtil.h>

#include <gepriv/WinUtil.h>

Process::Process()
{
    m_hasStarted = false;
    m_hasStopped = false;
    m_return = 0;
    m_processHandle = INVALID_HANDLE_VALUE;
    m_stdin = NULL;
    m_stdout = NULL;
    m_stderr = NULL;
}

Process::~Process()
{
    if (m_processHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_processHandle);
    }

    delete m_stdin;
    delete m_stdout;
    delete m_stderr;
}

void Process::execProgram(const String programName,
                          const List<String>& args)
{
    List<String> env(0);
    execProgram(programName, args, env, false);
}

void Process::execProgram(const String programName,
                          const List<String>& args,
                          bool mergeOutput)
{
    List<String> env(0);
    execProgram(programName, args, env, mergeOutput);
}

void Process::execProgram(const String programName,
                          const List<String>& args,
                          const List<String>& env)
{
    execProgram(programName, args, env, false);
}

void Process::execProgram(const String programName,
                          const List<String>& args,
                          const List<String>& env,
                          bool mergeOutput)
{
    internalExec(programName, args, env, mergeOutput);
}

void Process::execCommand(const String command)
{
    execCommand(command, false);
}

void Process::execCommand(const String command, bool mergeOutput)
{
    // Make path to cmd.exe
    String shellPath = makeShellPath();

    // Create parameter array for cmd.exe
    String quotedCommand = String("\"") + command + '\"';

    // TODO: Should escape
    List<String> argsArray(3);
    argsArray.addBack("cmd.exe");
    argsArray.addBack("/c");
    argsArray.addBack(quotedCommand);

    List<String> envArray(0);

    internalExec(shellPath, argsArray, envArray, mergeOutput);
}

int32 Process::waitFor()
{
    if (!m_hasStarted)
    {
        throw SystemException("Error waiting for process completion: Process "
            "never started");
    }

    if (m_hasStopped)
    {
        return m_return;
    }

    // Wait for the process to complete
    if (::WaitForSingleObject(m_processHandle, INFINITE))
    {
        throw SystemException(String("Error waiting for process completion: ") +
            WinUtil::getLastErrorMessage());
    }

    // Get the process's return code
    DWORD returnCode;
    if (!::GetExitCodeProcess(m_processHandle, &returnCode))
    {
        throw SystemException(String("Error waiting for process completion, failed "
            "to retrieve process return code: ") + WinUtil::getLastErrorMessage());
    }

    m_return = returnCode;
    m_hasStopped = true;
    return *(int*)(&returnCode);
}

OutputStream* Process::getStdIn() const
{
    return m_stdin;
}

InputStream* Process::getStdOut() const
{
    return m_stdout;
}

InputStream* Process::getStdErr() const
{
    return m_stderr;
}

void Process::internalExec(const String programName,
                           const List<String>& args,
                           const List<String>& env,
                           bool mergeOutput)
{
    HANDLE hInputRead = INVALID_HANDLE_VALUE;
    HANDLE hInputWrite = INVALID_HANDLE_VALUE;
    HANDLE hOutputRead = INVALID_HANDLE_VALUE;
    HANDLE hOutputWrite = INVALID_HANDLE_VALUE;
    HANDLE hErrorRead = INVALID_HANDLE_VALUE;
    HANDLE hErrorWrite = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa;
    DWORD errorNumber;

    if (m_hasStarted)
    {
        throw SystemException("Failed to create process: Cannot start two "
            "processes with one Process object");
    }

    // Set up the security attributes struct.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // Create the child input pipe.
    if (!::CreatePipe(&hInputRead, &hInputWrite, &sa, 0))
    {
        throw SystemException(String("Failed to create process. Couldn't create "
            "pipe: ") + WinUtil::getLastErrorMessage());
    }

    // Prevent the stdin write handle from being inherited
    if (!::SetHandleInformation(hInputWrite, HANDLE_FLAG_INHERIT, 0))
    {
        errorNumber = ::GetLastError();
        closePipe(hInputRead, hInputWrite);
        throw SystemException(String("Failed to create process. Couldn't set "
            "handle inheritance: ") + WinUtil::getErrorMessage(errorNumber));
    }

    // Create the child output pipe.
    if (!::CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0))
    {
        errorNumber = GetLastError();
        closePipe(hInputRead, hInputWrite);
        throw SystemException(String("Failed to create process. Couldn't create "
            "pipe: ") + WinUtil::getErrorMessage(errorNumber));
    }

    // Prevent the stdout read handle from being inherited
    if (!::SetHandleInformation(hOutputRead, HANDLE_FLAG_INHERIT, 0))
    {
        errorNumber = ::GetLastError();
        closePipe(hInputRead, hInputWrite);
        closePipe(hOutputRead, hOutputWrite);
        throw SystemException(String("Failed to create process. Couldn't "
            "set handle inheritance: ") + WinUtil::getErrorMessage(errorNumber));
    }

    if (!mergeOutput)
    {
        // Create the child error pipe.
        if (!::CreatePipe(&hErrorRead, &hErrorWrite, &sa, 0))
        {
            errorNumber = ::GetLastError();
            closePipe(hInputRead, hInputWrite);
            closePipe(hOutputRead, hOutputWrite);
            throw SystemException(String("Failed to create process. Couldn't "
                "create pipe: ") + WinUtil::getErrorMessage(errorNumber));
        }

        // Prevent the stderr read handle from being inherited
        if (!::SetHandleInformation(hErrorRead, HANDLE_FLAG_INHERIT, 0))
        {
            errorNumber = ::GetLastError();
            closePipe(hInputRead, hInputWrite);
            closePipe(hOutputRead, hOutputWrite);
            closePipe(hErrorRead, hErrorWrite);
            throw SystemException(String("Failed to create process. Couldn't "
                "set handle inheritance: ") + WinUtil::getErrorMessage(errorNumber));
        }
    }
    else
    {
        // If we are merging the output, just duplicate the stdout write
        // handle and make the child use it as its stderr

        if (!::DuplicateHandle(::GetCurrentProcess(), // Source Process
                               hOutputWrite, // Source HANDLE
                               ::GetCurrentProcess(), // Destination Process
                               &hErrorWrite, // Destination HANDLE address
                               0, // Access rights (ignored due to DUPLICATE_SAME_ACCESS)
                               TRUE, // Make the HANDLE inheritable
                               DUPLICATE_SAME_ACCESS)) // Use same access rights
        {
            closePipe(hInputRead, hInputWrite);
            closePipe(hOutputRead, hOutputWrite);
            throw SystemException(String("Failed to create process. Couldn't "
                "duplicate handle: ") + WinUtil::getLastErrorMessage());
        }
    }

    // Start the process
    m_processHandle = launchProcess(programName, args, env, hInputRead, hOutputWrite, hErrorWrite);

    // Close our copies of the pipe handles that the child process will use.
    ::CloseHandle(hInputRead);
    ::CloseHandle(hOutputWrite);
    ::CloseHandle(hErrorWrite);

    // Make stream objects
    m_stdin = new FileOutputStream(hInputWrite);
    m_stdout = new FileInputStream(hOutputRead);

    if (!mergeOutput)
    {
        m_stderr = new FileInputStream(hErrorRead);
    }

    m_hasStarted = true;
}

HANDLE Process::launchProcess(const String programName,
                              const List<String>& args,
                              const List<String>& env,
                              HANDLE hChildStdIn,
                              HANDLE hChildStdOut,
                              HANDLE hChildStdErr)
{
    List<wchar_t> wideProgramName;
    List<wchar_t> parameterString;
    List<wchar_t> envString;

    UnicodeUtil::utf8ToUtf16(programName.data(), programName.length(), &wideProgramName);
    wideProgramName.addBack(L'\0');
    
    // Make the parameter string
    makeParameterString(programName, args, &parameterString);

    // Make environment string. Need to pass NULL if the size is zero to get
    // default env.
    makeEnvArray(env, &envString);

    // Create the needed PROCESS_INFORMATION and STARTUPINFO structs
    PROCESS_INFORMATION processInfo;
    STARTUPINFOW startupInfo;

    memset(&startupInfo, 0, sizeof(startupInfo));
    memset(&processInfo, 0, sizeof(processInfo));

    // Set up the start up info struct.
    startupInfo.cb = sizeof(STARTUPINFOW);
    startupInfo.dwX = CW_USEDEFAULT;
    startupInfo.dwY = CW_USEDEFAULT;
    startupInfo.dwXSize = CW_USEDEFAULT;
    startupInfo.dwYSize = CW_USEDEFAULT;
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = hChildStdOut;
    startupInfo.hStdInput  = hChildStdIn;
    startupInfo.hStdError  = hChildStdErr;

    // Launch the process
    if (!CreateProcessW(wideProgramName.data(), // Name of the executable
                        parameterString.data(), // Parameters of the executable
                        NULL, // Default process attributes
                        NULL, // Default thread attributes
                        TRUE, // Use the std handles
                        CREATE_DEFAULT_ERROR_MODE | CREATE_UNICODE_ENVIRONMENT, // Don't inherit error mode, the env is unicode
                        envString.size() ? envString.data() : NULL, // Environment variables
                        NULL, // Run in current directory
                        &startupInfo, // Pass in our startup information
                        &processInfo)) // CreateProcess will fill this PROCESS_INFORMATION struct
    {
        throw SystemException(String("Failed to create process: ") +
            WinUtil::getLastErrorMessage());
    }

    // Close the unneeded handle to the process's primary thread
    ::CloseHandle(processInfo.hThread);

    // Return the process's handle
    return processInfo.hProcess;
}

void Process::closePipe(HANDLE& readHandle, HANDLE& writeHandle)
{
    if (readHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(readHandle);
        readHandle = INVALID_HANDLE_VALUE;
    }
    if (writeHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(writeHandle);
        writeHandle = INVALID_HANDLE_VALUE;
    }
}

/*
 * In a study of innefficiency, this currently converts from UTF-16 to UTF-8
 * to use the existing interface. It has to be converted back later.
 */
String Process::makeShellPath()
{
    // Ask how long the path to the system folder is
    uint32 pathLen = ::GetSystemDirectoryW(NULL, 0);

    if (pathLen == 0)
    {
        throw SystemException(String("Failed to start process. Couldn't "
            "retrieve path to \"cmd.exe\"") + WinUtil::getLastErrorMessage());
    }

    ShortList<wchar_t, 256> utf16Buffer;
    utf16Buffer.resize(pathLen);

    // Fill the buffer with the real path
    pathLen = ::GetSystemDirectoryW(utf16Buffer.data(), pathLen);

    if (pathLen == 0)
    {
        throw SystemException(String("Failed to start process. Couldn't "
            "retrieve path to \"cmd.exe\"") + WinUtil::getLastErrorMessage());
    }

    String result = UnicodeUtil::utf16ToUtf8(utf16Buffer.data(), utf16Buffer.size() - 1);

    // Append the name of the command interpreter
    result.append("\\cmd.exe");

    // Make string object to return
    return result;
}

void Process::makeParameterString(const String command,
                                  const List<String>& args,
                                  List<wchar_t>* result)
{
    // TODO: Check if arguments includes the program name?

    ShortList<wchar_t, 256> convList;

    String ret;

    size_t argsSize = args.size();
    for (size_t i = 0; i < argsSize; i++)
    {
        convList.clear();
        UnicodeUtil::utf8ToUtf16(args.get(i).data(), args.get(i).length(), &convList);

        result->addBlockBack(convList.data(), convList.size());

        if (i != argsSize - 1)
            result->addBack(L' ');
    }

    // Add trailing NUL
    result->addBack(L'\0');
}

void Process::makeEnvArray(const List<String>& env,
                           List<wchar_t>* envDest)
{
    // Empty list is special, using as an indication to pass a NULL
    // to CreateProcessW.
    if (env.size() == 0)
        return;

    // CreateProcess wants an unusual format of an array containing null
    // terminated strings, terminated by two nulls. For example:
    //
    // PATH=C:\(NUL)PIE=APPLE(NUL)PS1='pwd'(NUL)(NUL)

    ShortList<wchar_t, 256> convList;

    uint32 pos = 0;

    size_t envSize = env.size();
    for (size_t i = 0; i < envSize; i++)
    {
        convList.clear();
        UnicodeUtil::utf8ToUtf16(env.get(i).data(), env.get(i).length(), &convList);

        envDest->addBlockBack(convList.data(), convList.size());
        envDest->addBack(L'\0');
    }

    // Appending trailing '\0' character
    envDest->addBack(L'\0');
}
