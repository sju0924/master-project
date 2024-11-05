#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#define ARM_MPU_REGION_SIZE_32B      ((uint8_t)0x04U) ///!< MPU Region Size 32 Bytes
#define ARM_MPU_REGION_SIZE_64B      ((uint8_t)0x05U) ///!< MPU Region Size 64 Bytes
#define ARM_MPU_REGION_SIZE_128B     ((uint8_t)0x06U) ///!< MPU Region Size 128 Bytes
#define ARM_MPU_REGION_SIZE_256B     ((uint8_t)0x07U) ///!< MPU Region Size 256 Bytes
#define ARM_MPU_REGION_SIZE_512B     ((uint8_t)0x08U) ///!< MPU Region Size 512 Bytes
#define ARM_MPU_REGION_SIZE_1KB      ((uint8_t)0x09U) ///!< MPU Region Size 1 KByte
#define ARM_MPU_REGION_SIZE_2KB      ((uint8_t)0x0AU) ///!< MPU Region Size 2 KBytes
#define ARM_MPU_REGION_SIZE_4KB      ((uint8_t)0x0BU) ///!< MPU Region Size 4 KBytes
#define ARM_MPU_REGION_SIZE_8KB      ((uint8_t)0x0CU) ///!< MPU Region Size 8 KBytes
#define ARM_MPU_REGION_SIZE_16KB     ((uint8_t)0x0DU) ///!< MPU Region Size 16 KBytes
#define ARM_MPU_REGION_SIZE_32KB     ((uint8_t)0x0EU) ///!< MPU Region Size 32 KBytes
#define ARM_MPU_REGION_SIZE_64KB     ((uint8_t)0x0FU) ///!< MPU Region Size 64 KBytes

#define ALIGNMENT 32
#define REDZONE_SIZE 64

// poison_queue 최대 크기 설정
#define POISON_QUEUE_MAX_SIZE  2
#define TRUE 1
#define FALSE 0

#include <stdlib.h> 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>



typedef struct {
    size_t size;        // 힙 객체의 크기 정보
    void* raw_ptr;      // 할당된 메모리 블록의 실제 시작 주소
} HeapMetadata;

void configure_mpu_for_poison(void *ptr, uint32_t size) ;
void uart_debug_print(const char *str);

#endif  // RUNTIME_CONFIG_H
