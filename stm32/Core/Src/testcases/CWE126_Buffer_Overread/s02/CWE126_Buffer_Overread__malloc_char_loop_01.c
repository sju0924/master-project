/* TEMPLATE GENERATED TESTCASE FILE
Filename: CWE126_Buffer_Overread__malloc_char_loop_01.c
Label Definition File: CWE126_Buffer_Overread__malloc.label.xml
Template File: sources-sink-01.tmpl.c
*/
/*
 * @description
 * CWE: 126 Buffer Over-read ((Editted to global variable))
 * BadSource:  Use a small buffer in global variable
 * GoodSource: Use a large buffer in global variable
 * Sink: loop
 *    BadSink : Copy data to string using a loop
 * Flow Variant: 01 Baseline
 *
 * */

#include "std_testcase.h"

#include <wchar.h>

#ifndef OMITBAD

char data_bad[50];
char data_good[100];

void CWE126_Buffer_Overread__malloc_char_loop_01_bad()
{
    if (data_bad == NULL) {exit(-1);}
    memset(data_bad, 'A', 50-1); /* fill with 'A's */
    data_bad[50-1] = '\0'; /* null terminate */
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
            dest[i] = data_bad[i];
        }
        dest[100-1] = '\0';
        printLine(dest);
    }
}

#endif /* OMITBAD */

#ifndef OMITGOOD

/* goodG2B uses the GoodSource with the BadSink */
static void goodG2B()
{
    if (data_good == NULL) {exit(-1);}
    memset(data, 'A', 100-1); /* fill with 'A's */
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
            dest[i] = data_good[i];
        }
        dest[100-1] = '\0';
        printLine(dest);
    }
}

void CWE126_Buffer_Overread__malloc_char_loop_01_good()
{
    goodG2B();
}

#endif /* OMITGOOD */

/* Below is the main(). It is only used when building this testcase on
 * its own for testing or for building a binary to use in testing binary
 * analysis tools. It is not used when compiling all the testcases as one
 * application, which is how source code analysis tools are tested.
 */

#ifdef INCLUDEMAIN

int main(int argc, char * argv[])
{
    /* seed randomness */
    srand( (unsigned)time(NULL) );
#ifndef OMITGOOD
    printLine("Calling good()...");
    CWE126_Buffer_Overread__malloc_char_loop_01_good();
    printLine("Finished good()");
#endif /* OMITGOOD */
#ifndef OMITBAD
    printLine("Calling bad()...");
    CWE126_Buffer_Overread__malloc_char_loop_01_bad();
    printLine("Finished bad()");
#endif /* OMITBAD */
    return 0;
}

#endif