#include "heapManager.h"


void* my_malloc(size_t size) {
    
    char buffer[100];
    uint8_t tag ;
 
    // 필요한 메모리 크기 계산 (요청 크기 + 메타데이터 크기 + 레드존 * 2 + 정렬 여유 공간)
    size_t total_size = size + sizeof(HeapMetadata) + REDZONE_SIZE + (ALIGNMENT - 1);

    // 메모리 할당
    uintptr_t raw_ptr = (uintptr_t)malloc(total_size);
    if (!raw_ptr) {
        return NULL;  // 할당 실패 시 NULL 반환
    }

    // 32바이트 정렬된 시작 주소 계산
    uintptr_t aligned_ptr = (raw_ptr + sizeof(HeapMetadata) + (REDZONE_SIZE / 2) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    // 메타데이터 설정
    HeapMetadata* metadata = (HeapMetadata*)((aligned_ptr - REDZONE_SIZE/2 - sizeof(HeapMetadata)) & ~(4 - 1));
    metadata->size = size;
    metadata->raw_ptr = (void*)raw_ptr;  // 메모리 해제 시 사용할 실제 시작 주소 저장

    // 태그 설정
    // 우선 전체 할당 범위에만 태그 부여
    set_tag((void*)aligned_ptr, size);

    // 32바이트 정렬된 메모리 블록의 시작 위치 반환
    return (void*)aligned_ptr;
}


void my_free(void* ptr) {
    if (!ptr) return;  // NULL 포인터에 대한 보호

    // 메타데이터 위치 계산
    HeapMetadata* metadata = (HeapMetadata*)(((uintptr_t)ptr - REDZONE_SIZE/2 - sizeof(HeapMetadata)) & ~(4 - 1));

    // 힙 객체의 시작 및 끝 주소 계산
    uintptr_t start_address = (uintptr_t)ptr;
    uintptr_t end_address = start_address + metadata->size;

    // 태그 삭제
    for (uintptr_t i = start_address; i < end_address; i += 8) {
        remove_tag((void*)i, metadata->size);
    }

    // Free된 영역에 대한 MPU 보호 설정
    configure_mpu_for_poison((void *)(start_address), metadata->size);


    // 전체 메모리 블록 해제
    free(metadata->raw_ptr);  // 메타데이터에 저장된 실제 시작 주소로 전체 블록 해제
}