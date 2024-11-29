#ifndef TEST_CASES_H
#define TEST_CASES_H


void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_bad();
void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good();

void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_bad();
void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_good();

void CWE124_Buffer_Underwrite__CWE839_negative_01_bad();
void CWE124_Buffer_Underwrite__CWE839_negative_01_good();

void CWE124_Buffer_Underwrite__new_char_cpy_01_bad();
void CWE124_Buffer_Underwrite__new_char_cpy_01_good();

void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_bad();
void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_good();

void CWE126_Buffer_Overread__malloc_char_loop_01_bad();
void CWE126_Buffer_Overread__malloc_char_loop_01_good();

void CWE415_Double_Free__malloc_free_char_01_bad();
void CWE415_Double_Free__malloc_free_char_01_good();

void CWE416_Use_After_Free__malloc_free_char_01_bad();
void CWE416_Use_After_Free__malloc_free_char_01_good();

void CWE476_NULL_Pointer_Dereference__char_01_bad();
void CWE476_NULL_Pointer_Dereference__char_01_good();

#endif
