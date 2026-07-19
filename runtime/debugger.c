#include "runtimeConfig.h"
#include "test_runner.h"
#include <setjmp.h>

// UART 및 SD 카드 인터페이스 함수 선언
void sd_card_write(const char *message);

// Test-runner shared state (defined in test_runner.c, part of output.o)
extern jmp_buf      g_test_recovery;
extern volatile int g_test_running;
extern volatile int g_error_detected;
extern void HAL_MPU_Disable(void);

#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)

static void record_software_detection(DetectionSource source) {
    if (g_detection_source == DETECTION_NONE) {
        g_detection_source = source;
        g_detection_cycles = DWT_CYCCNT - g_detection_start_cycle;
    }
}

// 외부에 정의된 compare_tag 함수 선언
uint8_t* get_tag_address(void *address);

// 오류 원인 타입 정의
typedef enum {   
    ERROR_STACK_UNDERFLOW,
    ERROR_STACK_OVERFLOW,
    ERROR_HEAP_UNDERFLOW,
    ERROR_HEAP_OVERFLOW,
    ERROR_GLOBAL_VARIABLE_UNDERFLOW,
    ERROR_GLOBAL_VARIABLE_OVERFLOW,
    ERROR_USE_AFTER_FREE,
    ERROR_NULL_PTR,
    ERROR_TAG_MISMATCH,
    ERROR_NONE
} ErrorType;

// 오류 정보 구조체
typedef struct {
    ErrorType type;
    uint32_t pc;
    uint32_t lr;
    uint32_t fault_address;
    uint32_t cfsr;
    uint8_t mpu_region;
    void* tag_mismatch_addr;
} ErrorInfo;

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} ExceptionStackFrame;

// 오류 로그 작성 및 출력 함수
void log_error(ErrorInfo* info) {
    char log_buffer[512];

    // 오류 원인에 따른 메시지 설정
    const char* error_cause;
    if (info->type == ERROR_TAG_MISMATCH) {
        error_cause = "Tag Mismatch Detected";
    } else if (info->type == ERROR_STACK_OVERFLOW) {
        error_cause = "Stack Overflow Detected";
    } else if (info->type == ERROR_STACK_UNDERFLOW) {
        error_cause = "Stack Underflow Detected";
    }else if (info->type == ERROR_HEAP_OVERFLOW) {
        error_cause = "Heap Overflow Detected";
    } else if (info->type == ERROR_HEAP_UNDERFLOW) {
        error_cause = "Heap Underflow Detected";
    } else if (info->type == ERROR_GLOBAL_VARIABLE_OVERFLOW) {
        error_cause = "Global Variable Overflow Detected";
    } else if (info->type == ERROR_GLOBAL_VARIABLE_UNDERFLOW) {
        error_cause = "Global Variable Underflow Detected";
    } else if (info->type == ERROR_USE_AFTER_FREE) {
        error_cause = "Use After Free Detected";
    } else if (info->type == ERROR_NULL_PTR) {
        error_cause = "Null Ptr Use Detected";
    } else {
        error_cause = "Unknown Error";
    }

    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Error Cause: %s\r\n"
             "PC: 0x%08X, LR: 0x%08X\r\n",
             error_cause, info->pc, info->lr);

    // 추가 정보 작성
    if (info->type == ERROR_TAG_MISMATCH) {
        snprintf(log_buffer + strlen(log_buffer), sizeof(log_buffer) - strlen(log_buffer),
                 "Tag mismatch address: %p\r\n", info->tag_mismatch_addr);
    } else{
        snprintf(log_buffer + strlen(log_buffer), sizeof(log_buffer) - strlen(log_buffer),
                 "Fault Address (MMFAR): 0x%08X\r\n"
                 "CFSR: 0x%08X\r\n"
                 "MPU Region: %u\r\n", 
                 info->fault_address, info->cfsr, info->mpu_region);
    }


    // UART와 SD 카드에 오류 로그 전송
    uart_debug_print(log_buffer);
    // sd_card_write(log_buffer);
}

// 태그 불일치 예외 감지 함수 (태그 불일치 발생 시 호출됨)
void handle_tag_mismatch(void* start, void* end) {
    record_software_detection(DETECTION_SOFTWARE_TAG);
    ErrorInfo info = {0};
    info.type = ERROR_TAG_MISMATCH;
    info.pc   = (uintptr_t)start;
    info.lr   = 0x00000000;

    // 태그 불일치 발생 위치 탐색
    uint8_t* current = (uint8_t*)start;
    uint8_t* last    = (uint8_t*)end;
    while (current <= last) {
        if (!(*get_tag_address(start) == *get_tag_address(current))) {
            info.tag_mismatch_addr = current;
            break;
        }
        current++;
    }

    log_error(&info);

    // 테스트 러너 실행 중이면 예외 대신 복구 경로로 점프
    if (g_test_running) {
        HAL_MPU_Disable();
        g_error_detected = 1;
        longjmp(g_test_recovery, 2);
    }

    while (1);
}

void check_null_ptr(void *address) {
    if (address != NULL) {
        return;
    }

    record_software_detection(DETECTION_NULL);
    ErrorInfo info = {0};
    info.type = ERROR_NULL_PTR;
    info.pc = 0x00000000;
    info.lr = 0x00000000;
    info.fault_address = 0x00000000;
    info.cfsr = 0x00000000;
    info.mpu_region = 0U;

    log_error(&info);

    if (g_test_running) {
        HAL_MPU_Disable();
        g_error_detected = 1;
        longjmp(g_test_recovery, 2);
    }

    while (1);
}

void report_integer_underflow(void) {
    record_software_detection(DETECTION_INTEGER);

    if (g_test_running) {
        HAL_MPU_Disable();
        g_error_detected = 1;
        longjmp(g_test_recovery, 2);
    }

    while (1);
}




/* MemManage_Handler is defined in stm32l5xx_it.c (naked wrapper + C body).
   This file previously had a duplicate stub – removed to avoid linker conflict. */
