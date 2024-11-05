#include "runtimeConfig.h"

// UART 및 SD 카드 인터페이스 함수 선언

void sd_card_write(const char *message);
void MemManage_Handler(void);

// 오류 원인 타입 정의
typedef enum {
    ERROR_NONE,
    ERROR_TAG_MISMATCH,
    ERROR_MPU_VIOLATION
} ErrorType;

// 오류 정보 구조체
typedef struct {
    ErrorType type;
    uint32_t pc;
    uint32_t lr;
    uint32_t fault_address;
    uint32_t cfsr;
    uint32_t xpsr;
    uint8_t mpu_region;
    void* tag_mismatch_addr;
} ErrorInfo;

// 오류 로그 작성 및 출력 함수
void log_error(ErrorInfo* info) {
    char log_buffer[512];

    // 오류 원인에 따른 메시지 설정
    const char* error_cause;
    if (info->type == ERROR_TAG_MISMATCH) {
        error_cause = "Tag Mismatch Detected";
    } else if (info->type == ERROR_MPU_VIOLATION) {
        error_cause = "MPU Violation Detected";
    } else {
        error_cause = "Unknown Error";
    }

    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Error Cause: %s\r\n"
             "PC: 0x%08X, LR: 0x%08X, xPSR:0x%08X\r\n",
             error_cause, info->pc, info->lr,info->xpsr);

    // 추가 정보 작성
    if (info->type == ERROR_TAG_MISMATCH) {
        snprintf(log_buffer + strlen(log_buffer), sizeof(log_buffer) - strlen(log_buffer),
                 "Tag mismatch address: %p\r\n", info->tag_mismatch_addr);
    } else if (info->type == ERROR_MPU_VIOLATION) {
        snprintf(log_buffer + strlen(log_buffer), sizeof(log_buffer) - strlen(log_buffer),
                 "Fault Address (MMFAR): 0x%08X\r\n"
                 "CFSR: 0x%08X\r\n"
                 "MPU Region: %u\r\n", 
                 info->fault_address, info->cfsr, info->mpu_region);
    }

    // UART와 SD 카드에 오류 로그 전송
    uart_debug_print(log_buffer);
    sd_card_write(log_buffer);
}


// MPU 접근 위반 예외 처리기 (Memory Management Fault Handler)
void MemManage_Handler(void) {
    ErrorInfo info = {0};
    uint32_t *stack_ptr;
    info.type = ERROR_MPU_VIOLATION;

    // PC와 LR 레지스터 값 읽기
    asm volatile ("mov %0, sp" : "=r" (stack_ptr));  // SP 값 얻기

    info.lr = stack_ptr[5];  // Link Register
    info.pc = stack_ptr[6];  // Program Counter
    info.xpsr = stack_ptr[7]; // Program Status Register
    // CFSR 및 MMFAR 레지스터 값 읽기
    info.cfsr = *((volatile uint32_t*)0xE000ED28);   // Configurable Fault Status Register
    info.fault_address = *((volatile uint32_t*)0xE000ED34);  // Memory Management Fault Address Register

    // MPU Region Number Register에서 영역 번호 확인
    info.mpu_region = (*((volatile uint32_t*)0xE000ED98) & 0xFF);  // 하위 8비트만 사용하여 영역 번호 획득

    // 로그 작성 및 출력
    log_error(&info);

    // 시스템 정지 (디버깅 목적)
    while (1);
}