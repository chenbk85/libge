// HttpUtil.h

#ifndef HTTP_UTIL_H
#define HTTP_UTIL_H

#include <ge/text/String.h>
#include <ge/text/StringRef.h>
#include <ge/util/Date.h>

/*
 * HTTP utility methods
 *
 * HTTP URL encoding is not the same as URI encoding. It came from an early
 * standard that uses '+' instead of a '%20' for spaces, although %20 does
 * function. Do not use the functions here for URI strings.
 */
namespace HttpUtil
{
    /*! \brief Escapes a URL string.
     *
     * \param  str    String to escape
     * \return Escaped URL string
     */
    String escapeUrl(const StringRef& str);

    /*! \brief Unescapes a URL string. Appends result to dest. Returns true on
     * success.
     *
     * \param str    String to be unescaped
     * \param dest   Where allocated result will go
     * \return Success of unescape operation
     */
    bool unescapeUrl(const StringRef& str, String& dest);

    /*! \brief Unescapes a URL string in place. A string can only get
     * shorter. Returns true on success. Returns false on failure, but
     * guarentees nothing about the state of the passed string.
     *
     * \param str    String to unescape
     * \param size   Input and output of string before and after unescaping.
     * \return Success of unescape operation
     */
    bool unescapeUrlInPlace(char* str, size_t* size);

    /*! \brief Converts the passed unix timestamp to the most common format
     *         used by HTTP headers.
     *
     * HTTP allows these three formats:
     *
     * Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
     * Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
     * Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
     *
     * This function will always output the first, as it has a four digit
     * year.
     *
     * \param time The time to convert to text
     * \return Timestamp String
     */
    String formatTimestamp(Date date);

    /*! \brief Converts the passed unix timestamp to the most common format
     *         used by HTTP headers.
     *
     * HTTP allows these three formats:
     *
     * Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
     * Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
     * Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
     *
     * This function will always output the first, as it has a four digit
     * year. This means your buffer should be at least 30 bytes long.
     *
     * \param time    The time to convert to text
     * \param dest    Buffer to receive the timestamp
     */
    void formatTimestamp(Date date,
                         char* dest);
};

#endif // HTTP_UTIL_H
