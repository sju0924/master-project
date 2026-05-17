#ifndef TEST_CASES_H
#define TEST_CASES_H

/* ── CWE121: Stack-Based Buffer Overflow ───────────────────────────────── */
void CWE121_Stack_Based_Buffer_Overflow__CWE193_char_alloca_loop_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE193_char_alloca_loop_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__CWE193_wchar_t_declare_loop_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE193_wchar_t_declare_loop_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__CWE805_char_declare_snprintf_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE805_char_declare_snprintf_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__CWE805_struct_alloca_memmove_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE805_struct_alloca_memmove_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__CWE805_wchar_t_declare_snprintf_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE805_wchar_t_declare_snprintf_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__CWE806_char_declare_snprintf_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE806_char_declare_snprintf_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__CWE806_wchar_t_declare_snprintf_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__CWE806_wchar_t_declare_snprintf_01_good(void);

void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good(void);

/* ── CWE122: Heap-Based Buffer Overflow ────────────────────────────────── */
void CWE122_Heap_Based_Buffer_Overflow__char_type_overrun_memcpy_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__char_type_overrun_memcpy_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__CWE131_loop_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__CWE131_loop_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__CWE131_memmove_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__CWE131_memmove_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_memmove_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__c_CWE193_char_memmove_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_snprintf_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_char_snprintf_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__c_CWE806_wchar_t_memmove_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__c_CWE806_wchar_t_memmove_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__sizeof_double_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__sizeof_double_01_good(void);

/* ── CWE124: Buffer Underwrite ─────────────────────────────────────────── */
void CWE124_Buffer_Underwrite__CWE839_negative_01_bad(void);
void CWE124_Buffer_Underwrite__CWE839_negative_01_good(void);

void CWE124_Buffer_Underwrite__new_char_cpy_01_bad(void);
void CWE124_Buffer_Underwrite__new_char_cpy_01_good(void);

void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_bad(void);
void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_good(void);

/* ── CWE126: Buffer Overread ───────────────────────────────────────────── */
void CWE126_Buffer_Overread__malloc_char_loop_01_bad(void);
void CWE126_Buffer_Overread__malloc_char_loop_01_good(void);

/* ── CWE415: Double Free ───────────────────────────────────────────────── */
void CWE415_Double_Free__malloc_free_char_01_bad(void);
void CWE415_Double_Free__malloc_free_char_01_good(void);

/* ── CWE416: Use After Free ────────────────────────────────────────────── */
void CWE416_Use_After_Free__malloc_free_char_01_bad(void);
void CWE416_Use_After_Free__malloc_free_char_01_good(void);

/* ── CWE476: NULL Pointer Dereference ──────────────────────────────────── */
void CWE476_NULL_Pointer_Dereference__char_01_bad(void);
void CWE476_NULL_Pointer_Dereference__char_01_good(void);

void CWE476_NULL_Pointer_Dereference__binary_if_01_bad(void);
void CWE476_NULL_Pointer_Dereference__binary_if_01_good(void);

#endif
