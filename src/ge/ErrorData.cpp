// ErrorData.cpp

// Must match Error.h

const char* commonErrorStrings[] =
{
    "err_success",

    // Generic errors
    "err_invalid_argument",
    "err_invalid_handle",
    "err_too_many_handles",
    "err_argument_out_of_domain",
    "err_bad_address",
    "err_system_call_not_supported",
    "err_identifier_removed",
    "err_not_enough_memory",
    "err_not_supported",
    "err_access_denied",
    "err_timed_out",

    // Process errors
    "err_no_child_process",
    "err_invalid_executable",

    // Generic IO errors
    "err_broken_pipe",
    "err_io_error",
    "err_no_lock_available",
    "err_io_canceled",

    // File IO errors
    "err_file_not_found",
    "err_file_exists",
    "err_file_too_large",
    "err_file_in_use",
    "err_filename_too_long",
    "err_is_a_directory",
    "err_not_a_directory",
    "err_directory_not_empty",
    "err_invalid_seek",
    "err_no_space_on_device",
    "err_no_such_device",
    "err_no_such_file_or_directory",

    // File link errors
    "err_cross_device_link",
    "err_too_many_links",
    "err_too_many_synbolic_link_levels",

    // Inet errors
    "err_no_buffer_space",
    "err_address_not_supported",
    "err_address_in_use",
    "err_address_not_available",
    "err_already_connected",
    "err_argument_list_too_long",
    "err_connection_aborted",
    "err_connection_already_in_progress",
    "err_connection_refused",
    "err_connection_reset",
    "err_connection_shutdown",
    "err_not_connected",
    "err_host_unreachable",
    "err_network_down",
    "err_network_reset",
    "err_network_unreachable",
    "err_destination_address_required",
    "err_message_too_long",
    "err_protocol_error",

    // Text errors
    "err_illegal_byte_sequence",

    // Generic error for unmapped error code
    "err_unknown",
};
