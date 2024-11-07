#include "runtimeConfig.h"

// UART 및 SD 카드 인터페이스 함수 선언

void sd_card_write(const char *message);

// 외부에 정의된 compare_tag 함수 선언
uint8_t compare_tag(void* addr1, void* addr2);

// 오류 원인 타입 정의
typedef enum {   
    ERROR_BUFFER_UNDERFLOW,
    ERROR_BUFFER_OVERFLOW,
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
    } else if (info->type == ERROR_BUFFER_OVERFLOW) {
        error_cause = "Stack Overflow Detected";
    } else if (info->type == ERROR_BUFFER_UNDERFLOW) {
        error_cause = "Stack Underflow Detected";
    }else if (info->type == ERROR_HEAP_OVERFLOW) {
        error_cause = "Heap Overflow Detected";
    } else if (info->type == ERROR_HEAP_UNDERFLOW) {
        error_cause = "Heap Underflow Detected";
    } else if (info->type == ERROR_GLOBAL_VARIABLE_OVERFLOW) {
        error_cause = "Global Variable Overflow Detected";
    } else if (info->type == ERROR_GLOBAL_VARIABLE_UNDERFLOW) {
        error_cause = "Global Variable Overflow Detected";
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
    ErrorInfo info = {0};
    info.type = ERROR_TAG_MISMATCH;

    info.pc = (uintptr_t)start;
    info.lr = 0x00000000;


    // 태그 불일치 발생 위치 탐색
    uint8_t* current = (uint8_t*)start;
    uint8_t* last = (uint8_t*)end;
    while (current <= last) {
        if (compare_tag(start, current) != 0) {
            info.tag_mismatch_addr = current;
            break;
        }
        current++;
    }

    log_error(&info);
    // 시스템 정지 (디버깅 목적)
    while (1);
}

// MPU 접근 위반 예외 처리기 (Memory Management Fault Handler)
void MemManage_Handler(void) {
    ErrorInfo info = {0};
    ExceptionStackFrame *stack_frame;
    uint32_t *stack_ptr;

    asm volatile ("mov %0, sp" : "=r" (stack_ptr));  // SP 값 얻기

    info.pc = stack_frame->pc;
    info.lr = stack_frame->lr;

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