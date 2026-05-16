#include "test_runner.h"
#include "main.h"
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ── shared recovery state ───────────────────────────────────────────── */
jmp_buf      g_test_recovery;
volatile int g_test_running   = 0;
volatile int g_error_detected = 0;

/* ── DWT cycle counter ───────────────────────────────────────────────── */
#define DEM_CR     (*(volatile uint32_t *)0xE000EDFC)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)

static void dwt_init(void) {
    DEM_CR   |= (1UL << 24);   /* TRCENA  */
    DWT_CTRL |= (1UL << 0);    /* CYCCNTENA */
}

static uint32_t dwt_elapsed_us(uint32_t start) {
    return (DWT_CYCCNT - start) / (SystemCoreClock / 1000000U);
}

/* ── state reset between test cases ─────────────────────────────────── */
extern void HAL_MPU_Disable(void);
extern void HAL_MPU_Enable(uint32_t MPU_Control);
extern void configure_mpu_for_null_ptr(void);
extern void heap_reset(void);
extern void tags_reset(void);

#define MPU_RNR          (*(volatile uint32_t *)0xE000ED98)
#define MPU_RLAR         (*(volatile uint32_t *)0xE000EDA0)
#define MPU_PRIVILEGED_DEFAULT  4U

static void reset_mpu(void) {
    HAL_MPU_Disable();
    for (int i = 0; i < 7; i++) {   /* disable regions 0–6; region 7 = null-ptr */
        MPU_RNR  = (uint32_t)i;
        MPU_RLAR &= ~0x1UL;
    }
    configure_mpu_for_null_ptr();
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void reset_test_state(void) {
    reset_mpu();
    heap_reset();
    tags_reset();
}

/* ── UART output ─────────────────────────────────────────────────────── */
extern void uart_send_string(const char *str);

/* ── test-case table ─────────────────────────────────────────────────── */
typedef struct {
    const char *name;
    void (*bad)(void);
    void (*good)(void);
} TestCase;

/* forward declarations */
void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_bad(void);
void CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good(void);

void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_bad(void);
void CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_good(void);

void CWE124_Buffer_Underwrite__CWE839_negative_01_bad(void);
void CWE124_Buffer_Underwrite__CWE839_negative_01_good(void);
void CWE124_Buffer_Underwrite__new_char_cpy_01_bad(void);
void CWE124_Buffer_Underwrite__new_char_cpy_01_good(void);
void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_bad(void);
void CWE124_Buffer_Underwrite__char_alloca_memcpy_01_good(void);

void CWE126_Buffer_Overread__malloc_char_loop_01_bad(void);
void CWE126_Buffer_Overread__malloc_char_loop_01_good(void);

void CWE415_Double_Free__malloc_free_char_01_bad(void);
void CWE415_Double_Free__malloc_free_char_01_good(void);

void CWE416_Use_After_Free__malloc_free_char_01_bad(void);
void CWE416_Use_After_Free__malloc_free_char_01_good(void);

void CWE476_NULL_Pointer_Dereference__char_01_bad(void);
void CWE476_NULL_Pointer_Dereference__char_01_good(void);

static const TestCase test_table[] = {
    {
        "CWE121 Stack Overflow  (src_char_declare_cpy_01)",
        CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_bad,
        CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good
    },
    {
        "CWE122 Heap Overflow   (CWE805_wchar_t_memmove_01)",
        CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_bad,
        CWE122_Heap_Based_Buffer_Overflow__c_CWE805_wchar_t_memmove_01_good
    },
    {
        "CWE124 Buf Underwrite  (CWE839_negative_01)",
        CWE124_Buffer_Underwrite__CWE839_negative_01_bad,
        CWE124_Buffer_Underwrite__CWE839_negative_01_good
    },
    {
        "CWE124 Buf Underwrite  (new_char_cpy_01)",
        CWE124_Buffer_Underwrite__new_char_cpy_01_bad,
        CWE124_Buffer_Underwrite__new_char_cpy_01_good
    },
    {
        "CWE124 Buf Underwrite  (char_alloca_memcpy_01)",
        CWE124_Buffer_Underwrite__char_alloca_memcpy_01_bad,
        CWE124_Buffer_Underwrite__char_alloca_memcpy_01_good
    },
    {
        "CWE126 Buf Overread    (malloc_char_loop_01)",
        CWE126_Buffer_Overread__malloc_char_loop_01_bad,
        CWE126_Buffer_Overread__malloc_char_loop_01_good
    },
    {
        "CWE415 Double Free     (malloc_free_char_01)",
        CWE415_Double_Free__malloc_free_char_01_bad,
        CWE415_Double_Free__malloc_free_char_01_good
    },
    {
        "CWE416 Use After Free  (malloc_free_char_01)",
        CWE416_Use_After_Free__malloc_free_char_01_bad,
        CWE416_Use_After_Free__malloc_free_char_01_good
    },
    {
        "CWE476 NULL Ptr Deref  (char_01)",
        CWE476_NULL_Pointer_Dereference__char_01_bad,
        CWE476_NULL_Pointer_Dereference__char_01_good
    },
};

#define NUM_TESTS  (sizeof(test_table) / sizeof(test_table[0]))

/* ── runner ──────────────────────────────────────────────────────────── */
void run_all_tests(void) {
    dwt_init();

    char buf[160];
    int  pass = 0, fail = 0;

    uart_send_string("\r\n========================================\r\n");
    uart_send_string("  Memory Error Detection Test Runner\r\n");
    uart_send_string("========================================\r\n");

    for (int i = 0; i < (int)NUM_TESTS; i++) {
        const TestCase *tc = &test_table[i];

        /* ---- bad path: must detect ---- */
        reset_test_state();
        g_error_detected = 0;
        g_test_running   = 1;
        uint32_t t0 = DWT_CYCCNT;
        if (setjmp(g_test_recovery) == 0) {
            tc->bad();
        }
        uint32_t t_bad_us = dwt_elapsed_us(t0);
        int bad_caught = g_error_detected;
        g_test_running = 0;

        /* ---- good path: must NOT detect ---- */
        reset_test_state();
        g_error_detected = 0;
        g_test_running   = 1;
        t0 = DWT_CYCCNT;
        if (setjmp(g_test_recovery) == 0) {
            tc->good();
        }
        uint32_t t_good_us = dwt_elapsed_us(t0);
        int good_fp = g_error_detected;
        g_test_running = 0;

        /*
         * PASS: bad case detected  AND  good case not falsely detected
         * FAIL-MISS: bad case not detected
         * FAIL-FP  : good case falsely detected
         */
        const char *verdict;
        if      ( bad_caught && !good_fp) { verdict = "PASS    "; pass++; }
        else if (!bad_caught && !good_fp) { verdict = "FAIL-MISS"; fail++; }
        else if ( bad_caught &&  good_fp) { verdict = "FAIL-FP  "; fail++; }
        else                              { verdict = "FAIL-BOTH"; fail++; }

        snprintf(buf, sizeof(buf),
                 "[%s] %s\r\n"
                 "         bad: %6lu us  good: %6lu us\r\n",
                 verdict, tc->name,
                 (unsigned long)t_bad_us,
                 (unsigned long)t_good_us);
        uart_send_string(buf);
    }

    uart_send_string("----------------------------------------\r\n");
    snprintf(buf, sizeof(buf),
             "  Result: %d / %d PASS\r\n", pass, (int)NUM_TESTS);
    uart_send_string(buf);
    uart_send_string("========================================\r\n");
}
