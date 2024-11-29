#include <wchar.h>
#include "std_testcase.h"
#include "testcase_without_pass.h"


void printLine(const char * line);
char dataBuffer[100];

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
    wchar_t * data;
    dataBuffer[100-1] = L'\0';
    /* FIX: Set data pointer to the allocated memory buffer */
    data = dataBuffer;
    {
        wchar_t source[100];
        wmemset(source, L'C', 100-1); /* fill with 'C's */
        source[100-1] = L'\0'; /* null terminate */
        /* POTENTIAL FLAW: Possibly copying data to memory before the destination buffer */
        memcpy(data, source, 100*sizeof(wchar_t));
        /* Ensure the destination buffer is null terminated */
        data[100-1] = L'\0';
        printWLine(data);
    }
}

void CWE124_Buffer_Underwrite__wchar_t_alloca_memcpy_01_good_without_pass()
{
    CWE124_s04_01_goodG2B();
}

/* goodG2B uses the GoodSource with the BadSink */
static void CWE126_s02_01_goodG2B()
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

void CWE126_Buffer_Overread__malloc_char_loop_01_good_without_pass()
{
     CWE126_s02_01_goodG2B();
}