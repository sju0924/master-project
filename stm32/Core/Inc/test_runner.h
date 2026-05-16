#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <setjmp.h>

/* Shared state between test runner, MemManage_Handler, and handle_tag_mismatch */
extern jmp_buf      g_test_recovery;
extern volatile int g_test_running;
extern volatile int g_error_detected;

void run_all_tests(void);

#endif /* TEST_RUNNER_H */
