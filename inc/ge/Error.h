// Error.h

#ifndef ERROR_H
#define ERROR_H

#include <ge/common.h>

class String;

enum CommonError
{
    err_success = 0,

    // Generic errors
    err_invalid_argument, // EINVAL, ENOPROTOOPT, WSAEINVAL,
                          // ERROR_INVALID_PARAMETER, ERROR_BAD_ARGUMENTS
    err_invalid_handle, // ENOTSOCK, EBADF, WSAENOTSOCK
    err_too_many_handles, // ENFILE, EMFILE
    err_argument_out_of_domain, // EDOM
    err_bad_address, // EFAULT, WSAEFAULT
    err_system_call_not_supported, // ENOSYS
    err_identifier_removed, // EIDRM
    err_not_enough_memory, // ENOMEM
    err_not_supported, // ENOTSUP, EOPNOTSUPP
    err_access_denied, // EACCES, EPERM, EROFS, ERROR_ACCESS_DENIED,
                       // ERROR_CURRENT_DIRECTORY, ERROR_WRITE_PROTECT,
                       // ERROR_SHARING_VIOLATION
    err_timed_out, // ETIMEDOUT, ETIME

    // Process errors
    err_no_child_process, // ECHILD
    err_invalid_executable, // ENOEXEC, ERROR_BAD_EXE_FORMAT

    // Generic IO errors
    err_broken_pipe, // EPIPE
    err_io_error, // EIO
    err_no_lock_available, // ENOLCK
    err_io_canceled, // ECANCELED, WSA_OPERATION_ABORTED

    // File IO errors
    err_file_not_found, // ENOENT, ERROR_FILE_NOT_FOUND, ERROR_INVALID_DRIVE
    err_file_exists, // EEXIST, ERROR_FILE_EXISTS, ERROR_ALREADY_EXISTS
    err_file_too_large, // EFBIG
    err_file_in_use, // EBUSY, ETXTBSY
    err_filename_too_long, // ENAMETOOLONG, ERROR_BUFFER_OVERFLOW
    err_is_a_directory, // EISDIR
    err_not_a_directory, // ENOTDIR
    err_directory_not_empty, // ENOTEMPTY, ERROR_DIR_NOT_EMPTY
    err_invalid_seek, // ESPIPE
    err_no_space_on_device, // ENOSPC, ERROR_HANDLE_DISK_FULL, ERROR_DISK_FULL
    err_no_such_device, // ENXIO, ENODEV

    // File link errors
    err_cross_device_link, // EXDEV
    err_too_many_links, // EMLINK
    err_too_many_synbolic_link_levels, // ELOOP

    // Inet errors
    err_no_buffer_space, // ENOBUFS, WSAENOBUFS
    err_address_not_supported, // EAFNOSUPPORT
    err_address_in_use, // EADDRINUSE
    err_address_not_available, // EADDRNOTAVAIL
    err_already_connected, // EISCONN
    err_argument_list_too_long, // E2BIG
    err_connection_aborted, // ECONNABORTED, WSAECONNABORTED
    err_connection_already_in_progress, // EALREADY
    err_connection_refused, // ECONNREFUSED
    err_connection_reset, // ECONNRESET, WSAECONNRESET
    err_connection_shutdown, // ESHUTDOWN, WSAESHUTDOWN
    err_not_connected, // ENOTCONN, WSAENOTCONN
    err_host_unreachable, // EHOSTUNREACH
    err_network_down, // ENETDOWN, WSAENETDOWN
    err_network_reset, // ENETRESET, WSAENETRESET
    err_network_unreachable, // ENETUNREACH
    err_destination_address_required, // EDESTADDRREQ
    err_message_too_long, // EMSGSIZE
    err_protocol_error, // EPROTO, EPROTONOSUPPORT

    // Text errors
    err_illegal_byte_sequence, // EILSEQ

    // Generic error for unmapped error code
    err_unknown,
};

enum ErrorType
{
    err_type_library,
    err_type_system
};

/*
 * Represents an error code, but contains extra information about the cause.
 */
class Error
{
public:
    Error();
    Error(CommonError commonValue,
          const char* failurePoint);
    Error(CommonError commonValue,
          uint32      systemError,
          const char* failurePoint,
          const char* systemCall);
    Error(CommonError commonValue,
          ErrorType   errorType,
          uint32      systemError,
          const char* failurePoint,
          const char* systemCall);

    Error(const Error& other);
    Error operator=(const Error& other);

    bool isSet() const;

    CommonError getCommonValue() const;

    int32 getSystemValue() const;

    const char* getFailurePoint() const;

    const char* getSystemCall() const;

    String toString() const;

private:
    CommonError _commonError;
    ErrorType _errorType;
    int32 _systemValue;
    const char* _failurePoint;
    const char* _systemCall;
};

#endif // ERROR_H
