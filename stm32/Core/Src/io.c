#include <inttypes.h> // for PRId64
#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>
#include "std_testcase.h"

#ifdef BENCHMARK_MODE
volatile uint32_t g_benchmark_checksum;

static void benchmark_consume_u32(uint32_t value)
{
    g_benchmark_checksum =
        (g_benchmark_checksum << 5) ^ (g_benchmark_checksum >> 2) ^ value;
}
#endif

#ifndef _WIN32
#include <wchar.h>
#endif

void printLine (const char * line)
{
    if (line == NULL) return;
#ifdef BENCHMARK_MODE
    while (*line != '\0')
    {
        benchmark_consume_u32((uint8_t)*line++);
    }
#else
    char *ptr = line;
    size_t len = 0;
    while (*ptr++ != '\0')
    {
        len++;
    }


    // uart_send_string_char(line, len) ;
#endif
}

void printWLine (const wchar_t * line)
{
#ifdef BENCHMARK_MODE
    if (line == NULL) return;
    while (*line != L'\0')
    {
        benchmark_consume_u32((uint32_t)*line++);
    }
#else
    if(line != NULL)
    {
        wprintf(L"%ls\n", line);
    }
#endif
}

void printIntLine (int intNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)intNumber);
#else
    printf("%d\n", intNumber);
#endif
}

void printShortLine (short shortNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)(int32_t)shortNumber);
#else
    printf("%hd\n", shortNumber);
#endif
}

void printFloatLine (float floatNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)floatNumber);
#else
    printf("%f\n", floatNumber);
#endif
}

void printLongLine (long longNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)longNumber);
#else
    printf("%ld\n", longNumber);
#endif
}

void printLongLongLine (int64_t longLongIntNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)longLongIntNumber);
    benchmark_consume_u32((uint32_t)((uint64_t)longLongIntNumber >> 32));
#else
    printf("%lld\n", (long long)longLongIntNumber);
#endif
}

void printSizeTLine (size_t sizeTNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)sizeTNumber);
#else
    printf("%zu\n", sizeTNumber);
#endif
}

void printHexCharLine (char charHex)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint8_t)charHex);
#else
    printf("%02x\n", charHex);
#endif
}

void printWcharLine(wchar_t wideChar)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)wideChar);
#else
    /* ISO standard dictates wchar_t can be ref'd only with %ls, so we must make a
     * string to print a wchar */
    wchar_t s[2];
        s[0] = wideChar;
        s[1] = L'\0';
    printf("%ls\n", s);
#endif
}

void printUnsignedLine(unsigned unsignedNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)unsignedNumber);
#else
    printf("%u\n", unsignedNumber);
#endif
}

void printHexUnsignedCharLine(unsigned char unsignedCharacter)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)unsignedCharacter);
#else
    printf("%02x\n", unsignedCharacter);
#endif
}

void printDoubleLine(double doubleNumber)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)doubleNumber);
#else
    printf("%g\n", doubleNumber);
#endif
}

void printStructLine (const twoIntsStruct * structTwoIntsStruct)
{
#ifdef BENCHMARK_MODE
    benchmark_consume_u32((uint32_t)structTwoIntsStruct->intOne);
    benchmark_consume_u32((uint32_t)structTwoIntsStruct->intTwo);
#else
    printf("%d -- %d\n", structTwoIntsStruct->intOne, structTwoIntsStruct->intTwo);
#endif
}

void printBytesLine(const unsigned char * bytes, size_t numBytes)
{
#ifdef BENCHMARK_MODE
    for (size_t i = 0; i < numBytes; ++i)
    {
        benchmark_consume_u32(bytes[i]);
    }
#else
    size_t i;
    for (i = 0; i < numBytes; ++i)
    {
        printf("%02x", bytes[i]);
    }
    puts("");	/* output newline */
#endif
}

/* Decode a string of hex characters into the bytes they represent.  The second
 * parameter specifies the length of the output buffer.  The number of bytes
 * actually written to the output buffer is returned. */
size_t decodeHexChars(unsigned char * bytes, size_t numBytes, const char * hex)
{
    size_t numWritten = 0;

    /* We can't sscanf directly into the byte array since %02x expects a pointer to int,
     * not a pointer to unsigned char.  Also, since we expect an unbroken string of hex
     * characters, we check for that before calling sscanf; otherwise we would get a
     * framing error if there's whitespace in the input string. */
    while (numWritten < numBytes && isxdigit(hex[2 * numWritten]) && isxdigit(hex[2 * numWritten + 1]))
    {
        int byte;
        sscanf(&hex[2 * numWritten], "%02x", &byte);
        bytes[numWritten] = (unsigned char) byte;
        ++numWritten;
    }

    return numWritten;
}

/* Decode a string of hex characters into the bytes they represent.  The second
 * parameter specifies the length of the output buffer.  The number of bytes
 * actually written to the output buffer is returned. */
 size_t decodeHexWChars(unsigned char * bytes, size_t numBytes, const wchar_t * hex)
 {
    size_t numWritten = 0;

    /* We can't swscanf directly into the byte array since %02x expects a pointer to int,
     * not a pointer to unsigned char.  Also, since we expect an unbroken string of hex
     * characters, we check for that before calling swscanf; otherwise we would get a
     * framing error if there's whitespace in the input string. */
    while (numWritten < numBytes && iswxdigit(hex[2 * numWritten]) && iswxdigit(hex[2 * numWritten + 1]))
    {
        int byte;
        swscanf(&hex[2 * numWritten], L"%02x", &byte);
        bytes[numWritten] = (unsigned char) byte;
        ++numWritten;
    }

    return numWritten;
}

/* The two functions always return 1 or 0, so a tool should be able to
   identify that uses of these functions will always return these values */
int globalReturnsTrue()
{
    return 1;
}

int globalReturnsFalse()
{
    return 0;
}

int globalReturnsTrueOrFalse()
{
    return (rand() % 2);
}

/* The variables below are declared "const", so a tool should
   be able to identify that reads of these will always return their
   initialized values. */
const int GLOBAL_CONST_TRUE = 1; /* true */
const int GLOBAL_CONST_FALSE = 0; /* false */
const int GLOBAL_CONST_FIVE = 5;

/* The variables below are not defined as "const", but are never
   assigned any other value, so a tool should be able to identify that
   reads of these will always return their initialized values. */
int globalTrue = 1; /* true */
int globalFalse = 0; /* false */
int globalFive = 5;

/* define a bunch of these as empty functions so that if a test case forgets
   to make their's statically scoped, we'll get a linker error */
void good1() { }
void good2() { }
void good3() { }
void good4() { }
void good5() { }
void good6() { }
void good7() { }
void good8() { }
void good9() { }

/* shouldn't be used, but just in case */
void bad1() { }
void bad2() { }
void bad3() { }
void bad4() { }
void bad5() { }
void bad6() { }
void bad7() { }
void bad8() { }
void bad9() { }

/* define global argc and argv */

#ifdef __cplusplus
extern "C" {
#endif

int globalArgc = 0;
char** globalArgv = NULL;

#ifdef __cplusplus
}
#endif
