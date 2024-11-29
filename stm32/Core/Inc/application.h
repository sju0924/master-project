// #include "test_cases_CWE121_Stack_Based_Buffer_Overflow.h"
#include "testcase.h"
#include "main.h"

void test_uart_print();
void test_heap_allocation();
void test_buffer_overflow();
void test_buffer_underflow();
void test_heap_overflow();
void test_heap_underflow();
void test_gv_overflow();
void test_gv_underflow();
void test_nullptr_derefence();
void test_UAF();
void test_struct_tagging();
uint32_t    HAL_GetTick();
uint32_t          HAL_RCC_GetPCLK1Freq(void);
void application();