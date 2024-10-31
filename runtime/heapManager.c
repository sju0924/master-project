#define malloc(size) my_malloc(size)
#define free(ptr) my_free(ptr)

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "runtimeConfig.h"



void* my_malloc(size_t size) {
    // 필요한 메모리 크기 계산 (요청 크기 + 메타데이터 크기 + 레드존 * 2 + 정렬 여유 공간)
    size_t total_size = size + sizeof(HeapMetadata) + REDZONE_SIZE + (ALIGNMENT - 1);

    // 메모리 할당
    uintptr_t raw_ptr = (uintptr_t)malloc(total_size);
    if (!raw_ptr) {
        return NULL;  // 할당 실패 시 NULL 반환
    }

    // 32바이트 정렬된 시작 주소 계산
    uintptr_t aligned_ptr = (raw_ptr + sizeof(HeapMetadata) + (REDZONE_SIZE / 2) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    // 만약 정렬로 인해 레드존이 REDZONE_SIZE / 2가 아니게 된 경우 조정
    size_t actual_front_redzone_size = aligned_ptr - (raw_ptr + sizeof(HeapMetadata));
    if (actual_front_redzone_size != REDZONE_SIZE / 2) {
        aligned_ptr += REDZONE_SIZE / 2 - actual_front_redzone_size;
    }

    // 메타데이터 설정
    HeapMetadata* metadata = (HeapMetadata*)(aligned_ptr - sizeof(HeapMetadata) - (REDZONE_SIZE / 2));
    metadata->size = size;
    metadata->raw_ptr = (void*)raw_ptr;  // 메모리 해제 시 사용할 실제 시작 주소 저장

    // 앞뒤 레드존 설정
    for (size_t i = 0; i < REDZONE_SIZE / 2; i++) {
        ((uint8_t*)raw_ptr)[i] = 0xAA;  // 앞쪽 레드존 패턴 설정
        ((uint8_t*)(aligned_ptr + size))[i] = 0xAA;  // 뒤쪽 레드존 패턴 설정
    }

    // 32바이트 정렬된 메모리 블록의 시작 위치 반환
    return (void*)aligned_ptr;
}
void my_free(void* ptr) {
    if (!ptr) return;  // NULL 포인터에 대한 보호

    // 메타데이터 위치 계산
    HeapMetadata* metadata = (HeapMetadata*)((uintptr_t)ptr - (REDZONE_SIZE / 2) - sizeof(HeapMetadata));

    // 힙 객체의 시작 및 끝 주소 계산
    uintptr_t start_address = (uintptr_t)ptr;
    uintptr_t end_address = start_address + metadata->size;

    // 전체 메모리 블록 해제
    free(metadata->raw_ptr);  // 메타데이터에 저장된 실제 시작 주소로 전체 블록 해제
}