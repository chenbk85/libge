// WinUtil.h

#ifndef WIN_UTIL_H
#define WIN_UTIL_H

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/text/String.h>

/*
 * Collection of Windows utility functions and data.
 */
namespace WinUtil
{
    /*
     * Creates an Error object from a normal system call error.
     */
    Error getError(int errorNumber,
                   const char* systemCall,
                   const char* context);

	/*
	 * Returns a string that contains the system error message for the
     * current last error code.
	 */
	String getLastErrorMessage();

    /*
	 * Returns a string that contains the system error message for the
     * current last WSA error code.
	 */
    String getWSALastErrorMessage();

	/*
	 * This will return an error message string for the passed error number.
	 */
	String getErrorMessage(int errorNumber);
};

#endif // WIN_UTIL_H