#include <wchar.h>
#include "std_testcase.h"
#include "testcase_without_pass.h"


void printLine(const char * line);

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