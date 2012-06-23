// WinUtil.cpp

#include <gepriv/WinUtil.h>

#include <Winsock2.h> // Must be before Windows.h
#include <Windows.h>


Error WinUtil::getError(int errorNumber,
                        const char* context,
                        const char* systemCall)
{
    CommonError commonErr = err_unknown;

    switch (errorNumber)
    {
        // Generic errors
        case ERROR_INVALID_PARAMETER: // WSA_INVALID_PARAMETER
        case ERROR_BAD_ARGUMENTS:
        case WSAEINVAL:
            commonErr = err_invalid_argument; break;
        case ERROR_INVALID_HANDLE: // WSA_INVALID_HANDLE
        case WSAENOTSOCK:
            commonErr = err_invalid_handle; break;
        case ERROR_TOO_MANY_OPEN_FILES:
        case ERROR_NO_MORE_FILES:
            commonErr = err_too_many_handles; break;
        case WSAEFAULT:
            commonErr = err_bad_address; break;
        case ERROR_NOT_ENOUGH_MEMORY: // WSA_NOT_ENOUGH_MEMORY
        case ERROR_OUTOFMEMORY:
            commonErr = err_not_enough_memory; break;
        case ERROR_ACCESS_DENIED:
        case ERROR_CURRENT_DIRECTORY: // The directory cannot be removed
        case ERROR_WRITE_PROTECT:
        case ERROR_SHARING_VIOLATION:
            commonErr = err_access_denied; break;

        // Process errors
        case ERROR_BAD_EXE_FORMAT:
            commonErr = err_invalid_executable; break;

        // Generic IO errors
        case ERROR_OPERATION_ABORTED: // WSA_OPERATION_ABORTED
            commonErr = err_io_canceled; break;

        // File IO errors
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_DRIVE:
            commonErr = err_file_not_found; break;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            commonErr = err_file_exists; break;
        case ERROR_BUFFER_OVERFLOW: // The file name is too long.
            commonErr = err_filename_too_long; break;
        case ERROR_DIR_NOT_EMPTY:
            commonErr = err_directory_not_empty; break;
        case ERROR_HANDLE_DISK_FULL:
        case ERROR_DISK_FULL:
            commonErr = err_no_space_on_device; break;

        // INet errors
        case WSAENOBUFS:
            commonErr = err_no_buffer_space; break;
        case WSAECONNABORTED:
            commonErr = err_connection_aborted; break;
        case WSAECONNRESET:
            commonErr = err_connection_reset; break;
        case WSAESHUTDOWN:
            commonErr = err_connection_shutdown; break;
        case WSAENETDOWN:
            commonErr = err_network_down; break;
        case WSAENETRESET:
            commonErr = err_network_reset; break;
        case WSAENOTCONN:
            commonErr = err_not_connected; break;
    }

    return Error(commonErr,
                 errorNumber,
                 context,
                 systemCall);
}

String WinUtil::getLastErrorMessage()
{
	return getErrorMessage(::GetLastError());
}

String WinUtil::getWSALastErrorMessage()
{
	return getErrorMessage(::WSAGetLastError());
}

String WinUtil::getErrorMessage(int errorNumber)
{
	char* msgBuffer;

    DWORD strLen = ::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |	// Allocate a buffer for the string
		FORMAT_MESSAGE_FROM_SYSTEM |		// This is a system error message
        FORMAT_MESSAGE_IGNORE_INSERTS |		// Don't add insertion strings
        FORMAT_MESSAGE_MAX_WIDTH_MASK,      // Don't add newline breaks
        NULL,								// No source
        errorNumber,						// The message id (error number)
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // The language id (English essentially)
        (LPTSTR)&msgBuffer,					// The location to copy a pointer to the buffer to
        0,									// Buffer size (ignored)
		NULL);								// Argument list (ignored)

    if (strLen == 0)
    {
        // TODO: Handle
    }

    // Strip end white space
    char lastChar = msgBuffer[strLen-1];

    while (lastChar == ' ')
    {
        msgBuffer[strLen-1] = '\0';
        strLen--;
        lastChar = msgBuffer[strLen-1];
    }

	// Make a string object
	String ret = String(msgBuffer, strLen);

	// Free the memory Windows allocated
	// LocalFree is nessesary due to the allocation method
	::LocalFree(msgBuffer);

	return ret;
}
