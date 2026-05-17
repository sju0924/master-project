#include <wchar.h>
#include "std_testcase.h"
#include "testcase_without_pass.h"


void printLine(const char * line);
char dataBuffer[100];
char data_good[100];

void CWE121_goodG2B_s09_01_without_pass() 
{
    char data[48]__attribute__((aligned(8)));
    char dest[48] __attribute__((aligned(8)));
    /* FIX: Initialize data as a small buffer that as small or smaller than the small buffer used in the sink */
    memset(data, 'A', 48); /* fill with 'A's */
    data[48-1] = '\0'; /* null terminate */
    
        
    /* POTENTIAL FLAW: Possible buffer overflow if data is larger than dest */
    strcpy(dest, data);
    printLine(dest);
}
void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good_without_pass()
{
    CWE121_goodG2B_s09_01_without_pass();
}

static void CWE122_goodG2B_s09_01_without_pass()
{
    wchar_t * data;
    data = NULL;
    /* FIX: Allocate and point data to a large buffer that is at least as large as the large buffer used in the sink */
    data = (wchar_t *)malloc(100*sizeof(wchar_t));
    if (data == NULL) {exit(-1);}
    data[0] = L'\0'; /* null terminate */
    {
        wchar_t source[100];
        wmemset(source, L'C', 100-1); /* fill with L'C's */
        source[100-1] = L'\0'; /* null terminate */
        /* POTENTIAL FLAW: Possible buffer overflow if source is larger than data */
        memmove(data, source, 100*sizeof(wchar_t));
        data[100-1] = L'\0'; /* Ensure the destination buffer is null terminated */
        //printWLine(data);
        free(data);
    }
}

void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_good_without_pass()
{
     CWE122_goodG2B_s09_01_without_pass();
}

/* goodG2B uses the GoodSource with the BadSink */
static void CWE416_goodG2B_01()
{
    char * data;
    /* Initialize data */
    data = NULL;
    data = (char *)malloc(100*sizeof(char));
    if (data == NULL) {exit(-1);}
    memset(data, 'A', 100-1);
    data[100-1] = '\0';
    /* FIX: Do not free data in the source */
    /* POTENTIAL FLAW: Use of data that may have been freed */
    printLine(data);
    /* POTENTIAL INCIDENTAL - Possible memory leak here if data was not freed */
}

/* goodB2G uses the BadSource with the GoodSink */
static void CWE416_goodB2G_01()
{
    char * data;
    /* Initialize data */
    data = NULL;
    data = (char *)malloc(100*sizeof(char));
    if (data == NULL) {exit(-1);}
    memset(data, 'A', 100-1);
    data[100-1] = '\0';
    /* POTENTIAL FLAW: Free data in the source - the bad sink attempts to use data */
    free(data);
    /* FIX: Don't use data that may have been freed already */
    /* POTENTIAL INCIDENTAL - Possible memory leak here if data was not freed */
    /* do nothing */
    ; /* empty statement needed for some flow variants */
}

void CWE416_Use_After_Free__malloc_free_char_01_good_without_pass()
{
    CWE416_goodG2B_01();
    CWE416_goodB2G_01();
}


/* goodG2B uses the GoodSource with the BadSink */
static void CWE415_s01_goodG2B()
{
    char * data;
    /* Initialize data */
    data = NULL;
    data = (char *)malloc(100*sizeof(char));
    if (data == NULL) {exit(-1);}
    /* FIX: Do NOT free data in the source - the bad sink frees data */
    /* POTENTIAL FLAW: Possibly freeing memory twice */
    free(data);
}

/* goodB2G uses the BadSource with the GoodSink */
static void CWE415_s01_goodB2G()
{
    char * data;
    /* Initialize data */
    data = NULL;
    data = (char *)malloc(100*sizeof(char));
    if (data == NULL) {exit(-1);}
    /* POTENTIAL FLAW: Free data in the source - the bad sink frees data as well */
    free(data);
    /* do nothing */
    /* FIX: Don't attempt to free the memory */
    ;/* empty statement needed for some flow variants */
}

void CWE415_Double_Free__malloc_free_char_01_good_without_pass()
{
    CWE415_s01_goodG2B();
    CWE415_s01_goodB2G();
}

/* goodG2B uses the GoodSource with the BadSink */
static void CWE476_01_goodG2B()
{
    char * data;
    /* FIX: Initialize data */
    data = "Good";
    /* POTENTIAL FLAW: Attempt to use data, which may be NULL */
    /* printLine() checks for NULL, so we cannot use it here */
    printHexCharLine(data[0]);
}

/* goodB2G uses the BadSource with the GoodSink */
static void CWE476_01_goodB2G()
{
    char * data;
    /* POTENTIAL FLAW: Set data to NULL */
    data = NULL;
    /* FIX: Check for NULL before attempting to print data */
    if (data != NULL)
    {
        /* printLine() checks for NULL, so we cannot use it here */
        printHexCharLine(data[0]);
    }
    else
    {
        printLine("data is NULL");
    }
}

void CWE476_NULL_Pointer_Dereference__char_01_good_without_pass()
{
    CWE476_01_goodG2B();
    CWE476_01_goodB2G();
}

/* goodG2B uses the GoodSource with the BadSink */
static void CWE124_s02_01_goodG2B()
{
    int data;
    /* Initialize data */
    data = -1;
    /* FIX: Use a value greater than 0, but less than 10 to avoid attempting to
    * access an index of the array in the sink that is out-of-bounds */
    data = 7;
    {
        int i;
        int buffer[10] = { 0 };
        /* POTENTIAL FLAW: Attempt to access a negative index of the array
        * This code does not check to see if the array index is negative */
        if (data < 10)
        {
            buffer[data] = 1;
            /* Print the array values */
            for(i = 0; i < 10; i++)
            {
                printIntLine(buffer[i]);
            }
        }
        else
        {
            printLine("ERROR: Array index is negative.");
        }
    }
}

/* goodB2G uses the BadSource with the GoodSink */
static void CWE124_s02_01_goodB2G()
{
    int data;
    /* Initialize data */
    data = -1;
    /* POTENTIAL FLAW: Use an invalid index */
    data = -5;
    {
        int i;
        int buffer[10] = { 0 };
        /* FIX: Properly validate the array index and prevent a buffer underwrite */
        if (data >= 0 && data < (10))
        {
            buffer[data] = 1;
            /* Print the array values */
            for(i = 0; i < 10; i++)
            {
                printIntLine(buffer[i]);
            }
        }
        else
        {
            printLine("ERROR: Array index is out-of-bounds");
        }
    }
}

void CWE124_Buffer_Underwrite__CWE839_negative_01_good_without_pass()
{
    CWE124_s02_01_goodG2B();
    CWE124_s02_01_goodB2G();
}

static void CWE124_s03_01_goodG2B()
{
    char * data;
    data = NULL;
    {
        char * dataBuffer = malloc(100*sizeof(char));
        memset(dataBuffer, 'A', 100-1);
        dataBuffer[100-1] = '\0';
        /* FIX: Set data pointer to the allocated memory buffer */
        data = dataBuffer;
    }
    {
        char source[100];
        memset(source, 'C', 100-1); /* fill with 'C's */
        source[100-1] = '\0'; /* null terminate */
        /* POTENTIAL FLAW: Possibly copying data to memory before the destination buffer */
        strcpy(data, source);
        printLine(data);
        /* INCIDENTAL CWE-401: Memory Leak - data may not point to location
         * returned by new [] so can't safely call delete [] on it */
    }
}

void CWE124_Buffer_Underwrite__new_char_cpy_01_good_without_pass()
{
    CWE124_s03_01_goodG2B();
}

/* goodG2B uses the GoodSource with the BadSink */
static void CWE124_s04_01_goodG2B()
{
    char * data;
    dataBuffer[100-1] = L'\0';
    /* FIX: Set data pointer to the allocated memory buffer */
    data = dataBuffer;
    {
        char source[100];
        memset(source, 'C', 100-1); /* fill with 'C's */
        source[100-1] = '\0'; /* null terminate */
        /* POTENTIAL FLAW: Possibly copying data to memory before the destination buffer */
        memcpy(data, source, 100*sizeof(char));
        /* Ensure the destination buffer is null terminated */
        data[100-1] = '\0';
        printLine(data);
    }
}

void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_good_without_pass()
{
    CWE124_s04_01_goodG2B();
}

/* goodG2B uses the GoodSource with the BadSink */
static void CWE126_s02_01_goodG2B()
{
    if (data_good == NULL) {exit(-1);}
    memset(data_good, 'A', 100-1); /* fill with 'A's */
    data_good[100-1] = '\0'; /* null terminate */
    {
        size_t i, destLen;
        char dest[100];
        memset(dest, 'C', 100-1);
        dest[100-1] = '\0'; /* null terminate */
        destLen = strlen(dest);
        /* POTENTIAL FLAW: using length of the dest where data
         * could be smaller than dest causing buffer overread */
        for (i = 0; i < destLen; i++)
        {
            data_good[i] = dest[i];
        }
        dest[100-1] = '\0';
        printLine(dest);
    }
}

void CWE126_Buffer_Overread__malloc_char_loop_01_good_without_pass()
{
     CWE126_s02_01_goodG2B();
}

/* -*- mode: C++; c-file-style: "gnu-mode" -*- */
/* BEEBS aha-compress benchmark

   Copyright (C) 2013 Embecosm Limited and University of Bristol

   Contributor James Pallister <james.pallister@bristol.ac.uk>

   This file is part of the Bristol/Embecosm Embedded Benchmark Suite.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>. */
#include <stdio.h>
#include <stdlib.h>


/* This scale factor will be changed to equalise the runtime of the
   benchmarks. */
#define SCALE_FACTOR    (REPEAT_FACTOR >> 0)

/* Code from GLS.  Nine insns in the loop, giving 9*32 + 3 = 291 insns
worst case (mask = all 1's, not counting subroutine linkage). */

unsigned compress1_without_pass (unsigned x, unsigned mask) {
  unsigned result = 0, bit = 1;
  while (mask != 0) {
    if ((mask & 1) != 0) {
      if (x & 1) result |= bit;
      bit <<= 1;
    }
    mask >>= 1;
    x >>= 1;
  }
  return result;
}

/* A version with no branches in the loop.  Eight insns in the loop,
giving 8*32 + 2 = 258 insns worst case (however, the code above might
be faster if the mask is sparse). */

// ------------------------------ cut ----------------------------------
unsigned compress2_without_pass (unsigned x, unsigned m) {
   unsigned r, s, b;    // Result, shift, mask bit.

   r = 0;
   s = 0;
   do {
      b = m & 1;
      r = r | ((x & b) << s);
      s = s + b;
      x = x >> 1;
      m = m >> 1;
   } while (m != 0);
   return r;
}
// ---------------------------- end cut --------------------------------

/* Code from GLS.  Runs on a basic RISC in 159 ops total, incl.
subroutine overhead (just 3 ops).  Makes it clear that the five
masks can be precomputed if the mask is known (the five masks are
independent of x).  But this costs 5 stores and 5 loads because
"masks" is an array.  */

unsigned compress3_without_pass (unsigned x, unsigned mask) {
  unsigned masks[5];
  unsigned long q, m, zm;
  int i;
  m = ~mask;
  zm = mask;
  for (i = 0; i < 5; i++) {
      q = m;
      m ^= m << 1;
      m ^= m << 2;
      m ^= m << 4;
      m ^= m << 8;
      m ^= m << 16;
      masks[i] = (m << 1) & zm;
      m = q & ~m;
      q = zm & masks[i]; zm = zm ^ q ^ (q >> (1 << i));
  }
  x = x & mask;
  q = x & masks[0];  x = x ^ q ^ (q >> 1);
  q = x & masks[1];  x = x ^ q ^ (q >> 2);
  q = x & masks[2];  x = x ^ q ^ (q >> 4);
  q = x & masks[3];  x = x ^ q ^ (q >> 8);
  q = x & masks[4];  x = x ^ q ^ (q >> 16);
  return x;
}

/* Modification of GLS's code in which last 5 lines are
merged into the loop, to avoid the stores and loads of
array "masks."  Num. insns. = 5*24 + 7 = 127 total
(compiled to Cyclops and adding 1 for the "andc" op).
   Michael Dalton has observed that the first shift left
can be omitted. */

// ------------------------------ cut ----------------------------------
unsigned compress4_without_pass (unsigned x, unsigned m) {
   unsigned long mk, mp, mv, t;
   int i;

   x = x & m;           // Clear irrelevant bits.
   mk = ~m << 1;        // We will count 0's to right.

   for (i = 0; i < 5; i++) {
      mp = mk ^ (mk << 1);              // Parallel suffix.
      mp = mp ^ (mp << 2);
      mp = mp ^ (mp << 4);
      mp = mp ^ (mp << 8);
      mp = mp ^ (mp << 16);
      mv = mp & m;                      // Bits to move.
      m = (m ^ mv) | (mv >> (1 << i));    // Compress m.
      t = x & mv;
      x = (x ^ t) | (t >> (1 << i));      // Compress x.
      mk = mk & ~mp;
   }
   return x;
}

const unsigned long test[] = {
//       Data        Mask       Result
    0xFFFFFFFF, 0x80000000, 0x00000001,
    0xFFFFFFFF, 0x0010084A, 0x0000001F,
    0xFFFFFFFF, 0x55555555, 0x0000FFFF,
    0xFFFFFFFF, 0x88E00F55, 0x00001FFF,
    0x01234567, 0x0000FFFF, 0x00004567,
    0x01234567, 0xFFFF0000, 0x00000123,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0,          0,          0,
    0,          0xFFFFFFFF, 0,
    0xFFFFFFFF, 0,          0,
    0x80000000, 0x80000000, 1,
    0x55555555, 0x55555555, 0x0000FFFF,
    0x55555555, 0xAAAAAAAA, 0,
    0x789ABCDE, 0x0F0F0F0F, 0x00008ACE,
    0x789ABCDE, 0xF0F0F0F0, 0x000079BD,
    0x92345678, 0x80000000, 0x00000001,
    0x12345678, 0xF0035555, 0x000004ec,
    0x80000000, 0xF0035555, 0x00002000,
};




int
benchmark_without_pass (void)
{
   int errors = 0,  n, i;
   unsigned int r;

   n = sizeof(test)/sizeof(test[0]);

   for (i = 0; i < n; i += 3) {
      r = compress1_without_pass (test[i], test[i+1]);
      if (r != test[i+2])
         errors = 1;
   }

   for (i = 0; i < n; i += 3) {
      r = compress2_without_pass (test[i], test[i+1]);
      if (r != test[i+2])
         errors = 1;
   }

   for (i = 0; i < n; i += 3) {
      r = compress3_without_pass (test[i], test[i+1]);
      if (r != test[i+2])
         errors = 1;
   }

   for (i = 0; i < n; i += 3) {
      r = compress4_without_pass(test[i], test[i+1]);
      if (r != test[i+2])
         errors = 1;
   }

   return errors;
}

/* This scale factor will be changed to equalise the runtime of the
   benchmarks. */
#define SCALE_FACTOR    (REPEAT_FACTOR >> 0)

   /* assume all data is positive */

int
benchmark_insert_sort_without_pass (void)
{
  unsigned int a[11]; 
  int i,j, temp;
  i = 2;

    a[0] = 0;
    a[1] = 11;
    a[2] = 10;
    a[3] = 9;
    a[4] = 8;
    a[5] = 7;
    a[6] = 6;
    a[7] = 5;
    a[8] = 4;
    a[9] = 3;
    a[10]= 2;

  while(i <= 10){
      j = i;
      while (a[j] < a[j-1])
      {
        temp = a[j];
        a[j] = a[j-1];
        a[j-1] = temp;
        j--;
    }
    i++;
    }
  return 0;
}
