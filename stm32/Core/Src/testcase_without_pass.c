#include <wchar.h>
#include "std_testcase.h"
#include "testcase_without_pass.h"


void printLine(const char * line);

void CWE121_goodG2B_without_pass() 
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
    CWE121_goodG2B_without_pass();
}

static void CWE122_goodG2B_without_pass()
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
     CWE122_goodG2B_without_pass();
}