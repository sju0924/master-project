#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <setjmp.h>
#include <stdint.h>

typedef uint32_t DetectionSource;

enum {
    DETECTION_NONE = 0,
    DETECTION_SOFTWARE_TAG,
    DETECTION_NULL,
    DETECTION_INTEGER,
    DETECTION_MPU,
    DETECTION_CPU_FAULT
};

/* Shared state between test runner, MemManage_Handler, and handle_tag_mismatch */
extern jmp_buf      g_test_recovery;
extern volatile int g_test_running;
extern volatile int g_error_detected;
extern volatile int g_test_phase;
extern volatile DetectionSource g_detection_source;
extern volatile uint32_t g_detection_cycles;
extern volatile uint32_t g_detection_start_cycle;
extern volatile uint32_t g_last_fault_phase;
extern volatile uint32_t g_last_fault_pc;
extern volatile uint32_t g_last_fault_lr;
extern volatile uint32_t g_last_fault_cfsr;
extern volatile uint32_t g_last_fault_hfsr;
extern volatile uint32_t g_last_fault_mmfar;
extern volatile uint32_t g_last_fault_bfar;

void run_all_tests(void);

#endif /* TEST_RUNNER_H */
