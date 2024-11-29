#ifndef TEST_CASES_H
#define TEST_CASES_H

void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_bad();
void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good();

void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_bad();
void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_good();

void CWE416_Use_After_Free__malloc_free_char_01_bad();
void CWE416_Use_After_Free__malloc_free_char_01_good();

void CWE415_Double_Free__malloc_free_char_01_bad();
void CWE415_Double_Free__malloc_free_char_01_good();


#endif
