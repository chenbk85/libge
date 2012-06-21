// UtilData.h

#ifndef UTIL_DATA_H
#define UTIL_DATA_H

#include <ge/common.h>

/*
 * Common data used by more than one util class/namespace. See UtilData.cpp
 * for the definitions.
 */
namespace UtilData
{
    // The ten's digit of numbers from 0-99
    extern char g_digitTens[100];

    // The one's digit of numbers from 0-99
    extern char g_digitOnes[100];

    // The numeric value of the given byte, or -1 if not a numeric type.
    extern int8 g_charValue[256];

    // The number of ones in a byte of the given index
    extern uint8 g_onesCountsTable[256];
}

#endif // UTIL_DATA_H
