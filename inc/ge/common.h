// common.h

#ifndef COMMON_H
#define COMMON_H

// GCC version check macro
#if defined(__GNUC__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) \
    ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= \
    (major * 10000 + minor * 100 + patch))
#else
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif


// This block of macros and typedefs defines the common types and defines for
// different compilers. Currently Visual Studio or something GCC like are
// supported.
#if defined(_MSC_VER) // If Visual Studio

#pragma warning(push)
#if _MSC_VER >= 1310
// Disable warning that the compiler is working like it's supposed to with
// initializer lists
// "warning C4351: new behavior: elements of array will be default initialized"
#pragma warning(disable: 4351)
// Disable annoying "forcing to bool" optimization warning
// warning C4800: 'char' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)
#endif

// Ignore warnings about using "non-secure" C functions
#define _CRT_SECURE_NO_WARNINGS

// Ignore warnings about using "non-secure" C++ functions
#define _SCL_SECURE_NO_WARNINGS

// Prevent Windows.h from including winsock.h
#define _WINSOCKAPI_

// Note C++11 support
#if (_MSC_VER >= 1600)
#define HAVE_NULLPTR
#define HAVE_OVERRIDE
#endif

// Define primitive types
typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

#if defined(_M_X64)
typedef __int64 ssize_t;
#else
typedef __int32 ssize_t;
#endif

// Define unicode types
typedef wchar_t utf16;
typedef unsigned __int32 utf32; // wchar32_t just an alias for now

// Define alignment macros
#define ALIGN(X, Y) __declspec(align(Y)) X

#define ALIGN_OF(X) __alignof(X)

// Alignments documented here (http://msdn.microsoft.com/en-us/library/ycsb6wwf.aspx)
#if defined(_M_X64)
#define MALLOC_ALIGN 16
#else
#define MALLOC_ALIGN 8
#endif

// Chipset define
#if defined(_M_X86) || defined(_M_X64)
#define CHIPSET_X86 1
#endif

#else // Not Visual Studio

// Clang compatibility
#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef __has_extension
#define __has_extension(x) 0
#endif

// Note C++11 support
#if (GCC_VERSION_AT_LEAST(4, 6, 0) && defined(_GXX_EXPERIMENTAL_CXX0X__)) || \
    __has_feature(cxx_nullptr) || \
    __has_extension(cxx_nullptr)
#define HAVE_NULL_PTR
#endif

#if (GCC_VERSION_AT_LEAST(4, 7, 0) && defined(_GXX_EXPERIMENTAL_CXX0X__)) || \
    __has_feature(cxx_override_control) || \
    __has_extension(cxx_override_control)
#define HAVE_OVERRIDE
#endif

#if (GCC_VERSION_AT_LEAST(4, 4, 0) && defined(_GXX_EXPERIMENTAL_CXX0X__)) || \
    __has_feature(cxx_deleted_functions) || \
    __has_extension(cxx_deleted_functions)
#define HAVE_DELETED_FUNCS
#endif

// Define primitive types
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// Define unicode types
#if defined(HAVE_UNICODE_LIT)
#if (__SIZEOF_WCHAR_T__ == 2)
typedef wchar_t utf16;
typedef char32_t utf32;
#else
typedef char16_t utf16;
typedef wchar_t utf32;
#endif
#else
#if (__SIZEOF_WCHAR_T__ == 2)
typedef wchar_t utf16;
typedef uint32_t utf32;
#else
typedef uint16_t utf16;
typedef wchar_t utf32;
#endif
#endif

// Define NULL
#if !defined(NULL)
#if defined(HAVE_NULLPTR)
#define NULL nullptr
#elif defined(__GNUC__)
#define NULL __null
#else
#define NULL 0L
#endif
#endif

// Define alignment macros
#define ALIGN(X, Y) X __attribute__((aligned(Y)))

#define ALIGN_OF(X) __alignof__(x)

#define MALLOC_ALIGN __alignof__(double)

// Chipset define
#if defined(__i386__)  || defined(__x86_64__)
#define CHIPSET_X86
#endif

#endif // Non-GCC compiler

// Macro for using the override feature of C++ if available
#ifdef HAVE_OVERRIDE
#define OVERRIDE override
#else
#define OVERRIDE
#endif

#ifdef HAVE_DELETED_FUNCS
#define DELETED =delete
#else
#define DELETED
#endif

#endif // COMMON_H
