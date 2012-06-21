// Process.cpp

#include "ge/io/Process.h"

#include "ge/SystemException.h"
#include "ge/io/IOException.h"
#include "gepriv/UnixUtil.h"

#include <errno.h> // For errno
#include <fcntl.h> // For fcntl()
#include <string.h> // For strcpy()
#include <unistd.h> // For fork(), _exit()
#include <sys/types.h> // For wait_pid()
#include <sys/wait.h> // For wait_pid()

// Defines the size of buffer used when reading an entire stream
#define READ_BUFFER_SIZE 1024

Process::Process()
{
    m_hasStarted = false;
    m_hasStopped = false;
    m_return = 0;
    m_pid = -1;
    m_stdin = NULL;
    m_stdout = NULL;
    m_stderr = NULL;
}

Process::~Process()
{
    /*
     * If the process is a zombie, it will have to be cleaned up by a
     * signal handler.
     */
    delete m_stdin;
    delete m_stdout;
    delete m_stderr;
}

void Process::execProgram(const String programName,
                          const List<String>& args)
{
    List<String> env(0);
    internalExec(programName, args, env, false);
}

void Process::execProgram(const String programName,
                          const List<String>& args,
                          bool mergeOutput)
{
    List<String> env(0);
    internalExec(programName, args, env, mergeOutput);
}

void Process::execProgram(const String programName,
                          const List<String>& args,
                          const List<String>& env)
{
    internalExec(programName, args, env, false);
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
    // Create parameter array for sh
    List<String> paramArray(3);
    paramArray.addBack("sh");
    paramArray.addBack("-c");
    paramArray.addBack(command);

    List<String> envArray(0);

    internalExec("/bin/sh", paramArray, envArray, mergeOutput);
}

bool Process::isRunning()
{
    if (!m_hasStarted ||
        m_hasStopped)
    {
        return false;
    }

    // Call waitpid with WNOHANG to see if it has stopped
    int32 status;

    pid_t pid = ::waitpid(m_pid, &status, WNOHANG);

    // If the pid returned was -1 it failed for some reason
    if (pid == -1)
    {
        throw SystemException(String("waitpid() call failed for Process: ") +
            UnixUtil::getLastErrorMessage());
        return true;
    }
    else if (pid != 0)
    {
        // The process has stopped
        m_hasStopped = true;
        return false;
    }

    // If the pid returned was 0, the process is still going
    return true;
}

int32 Process::waitFor()
{
    if (!m_hasStarted)
    {
        throw SystemException("Can't wait for a process that never started");
    }

    if (m_hasStopped)
    {
        return m_return;
    }

    // Wait for the process to exit. Passing zero as the last parameter means
    // we do not want waitpid to return for stopped or paused processes.
    int32 status;

    pid_t pid = UnixUtil::sys_waitpid(m_pid, &status, 0);

    // If the pid returned was -1 it failed for some reason
    if (pid == -1)
    {
        throw SystemException(String("waitpid() call failed with: ") +
            UnixUtil::getLastErrorMessage());
        return 1;
    }

    m_hasStopped = true;

    // If the process did not exit normally we can't get the return
    // value. For now, just returning 1 for failure.
    if (!WIFEXITED(status))
    {
        m_return = 1;
    }
    else
    {
        m_return = WEXITSTATUS(status);
    }

    return m_return;
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

void Process::internalExec(const String& programName,
                           const List<String>& args,
                           const List<String>& env,
                           bool mergeOutput)
{
    int32 error;

    // Unix pipes (index 0 is read, index 1 is write)
    int32 stdInPipe[2] = {-1, -1};
    int32 stdOutPipe[2] = {-1, -1};
    int32 stdErrPipe[2] = {-1, -1};
    int32 childErrorPipe[2] = {-1, -1};

    // Create pipes for stdin, stdout, stderr
    if (::pipe(stdInPipe) == -1)
    {
        throw SystemException(String("pipe() call failed with: ") +
            UnixUtil::getLastErrorMessage());
        return;
    }

    if (::pipe(stdOutPipe) == -1)
    {
        error = errno;
        closePipe(stdInPipe);
        throw SystemException(String("pipe() call failed with: ") +
            UnixUtil::getErrorMessage(error));
        return;
    }

    // Only create a pipe for stderr if we need it
    if (!mergeOutput)
    {
        if (::pipe(stdErrPipe) == -1)
        {
            error = errno;
            closePipe(stdInPipe);
            closePipe(stdOutPipe);
            throw SystemException(String("pipe() call failed with: ") +
                UnixUtil::getErrorMessage(error));
            return;
        }
    }

    // And create one more pipe to receive errors past fork() call
    if (::pipe(childErrorPipe) == -1)
    {
        error = errno;
        closePipe(stdInPipe);
        closePipe(stdOutPipe);
        closePipe(stdErrPipe);
        throw SystemException(String("pipe() call failed with: ") +
            UnixUtil::getErrorMessage(error));
        return;
    }

    // Fork a new process
    pid_t pid = ::fork();

    if (pid == -1)
    {
        // We failed to fork for some reason
        error = errno;
        closePipe(stdInPipe);
        closePipe(stdOutPipe);
        closePipe(stdErrPipe);
        closePipe(childErrorPipe);
        throw SystemException(String("fork() call failed with: ") +
            UnixUtil::getErrorMessage(error));
    }
    else if (pid == 0)
    {
        // If the pid equals zero then we are the child process

        // To inform the parent process of errors we write the error number
        // to childErrorPipe. Our end will get closed by calling exec.

        // Duplicate file desciptors onto the standard io descriptors
        // and then close the unused handles

        UnixUtil::sys_dup2(stdInPipe[0], STDIN_FILENO);
        closePipe(stdInPipe);

        UnixUtil::sys_dup2(stdOutPipe[1], STDOUT_FILENO);
        closePipe(stdOutPipe);

        // Overwrite stderr with stdout if required
        if (mergeOutput)
        {
            UnixUtil::sys_dup2(STDOUT_FILENO, STDERR_FILENO);
        }
        else
        {
            UnixUtil::sys_dup2(stdErrPipe[1], STDERR_FILENO);
        }
        closePipe(stdErrPipe);

        // Make the child error pipe close on successful exec call
        if (::fcntl(childErrorPipe[1], F_SETFD, FD_CLOEXEC) == -1)
        {
            error = errno;
            UnixUtil::sys_write(childErrorPipe[1], &error, sizeof(int32));
            ::close(childErrorPipe[1]);
            ::_exit(error);
        }

        // TODO: Should automatically add in program name if it's missing

        // Make a char** array of arguments for exec. Freed by calling exec
        // or _exit
        char** argv = allocExecArray(args);

        // If there is no enironment passed in, call execvp(), otherwise
        // create an envp array and call execve()
        if (env.size() == 0)
        {
            ::execvp(programName.c_str(), argv);
        }
        else
        {
            // Make a char** array of env for execve. Freed by calling exec
            // or _exit
            char** envp = allocExecArray(env);

            ::execve(programName.c_str(), argv, envp);
        }

        // An error occured if execvp returned.
        error = errno;
        UnixUtil::sys_write_fully(childErrorPipe[1], &error, sizeof(int32));
        ::close(childErrorPipe[1]);
        ::_exit(error);
    }
    else
    {
        // The returned pid was not zero, so we are not the child process
        m_pid = pid;

        // Close unused end of childErrorPipe
        ::close(childErrorPipe[1]);

        // Try to read error from child
        ssize_t bytesRead = UnixUtil::sys_read_fully(childErrorPipe[0], &error, sizeof(int32));

        // Close the read end of the pipe now that we don't need it either
        ::close(childErrorPipe[0]);

        // If the child process sent us an error number, throw an exception
        if (bytesRead == sizeof(int32))
        {
            closePipe(stdInPipe);
            closePipe(stdOutPipe);
            closePipe(stdErrPipe);
            throw SystemException(String("exec() call failed with: ") +
                UnixUtil::getErrorMessage(error));
            return;
        }
        else
        {
            // TODO: Log read failure
        }

        ::close(stdInPipe[0]);
        ::close(stdOutPipe[1]);

        // Make stream objects
        m_stdin = new FileOutputStream(stdInPipe[1]);
        m_stdout = new FileInputStream(stdOutPipe[0]);

        if (!mergeOutput)
        {
            ::close(stdErrPipe[1]);
            m_stderr = new FileInputStream(stdErrPipe[0]);
        }

        m_hasStarted = true;
    }
}

void Process::closePipe(int32* aPipe)
{
    if (aPipe[0] != -1)
    {
        ::close(aPipe[0]);
        aPipe[0] = -1;
    }
    if (aPipe[1] != -1)
    {
        ::close(aPipe[1]);
        aPipe[1] = -1;
    }
}

char** Process::allocExecArray(const List<String>& args)
{
    // Instead of allocating a multidimensional array of null terminated
    // strings, we allocate a single array and shove both the array of char*
    // and the strings into the same buffer.
    uint32 size = sizeof(void*);

    for (uint32 i = 0; i < args.size(); i++)
    {
        size += args.get(i).length() + 1 + sizeof(void*);
    }

    char* ret = new char[size];

    // Iterate through the list of arguments, putting the address of the
    // argument at a space in the beginning of the array, and the string data
    // after the pointers at the end of the array
    char** ptrPos = (char**)ret;
    char* stringPos = ret + ((args.size()+1) * sizeof(void*));

    for (uint32 i = 0; i < args.size(); i++)
    {
        (*ptrPos) = stringPos;
        ptrPos++;

        String arg = args.get(i);
        ::strcpy(stringPos, arg.c_str());
        stringPos += arg.length() + 1;
    }

    (*ptrPos) = NULL;
    return (char**)ret;
}
