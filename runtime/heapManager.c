#include "heapManager.h"


void* my_malloc(size_t size) {
    
    char buffer[100];
    uint8_t tag ;
    
    HAL_MPU_Disable();
    
    // 필요한 메모리 크기 계산 (요청 크기 + 메타데이터 크기 + 레드존 * 2 + 정렬 여유 공간)
    size_t total_size = size + sizeof(HeapMetadata) + REDZONE_SIZE + (ALIGNMENT - 1);

    // 메모리 할당
    uintptr_t raw_ptr = (uintptr_t)malloc(total_size);
    if (!raw_ptr) {
        return NULL;  // 할당 실패 시 NULL 반환
    }

    // 32바이트 정렬된 시작 주소 계산
    uintptr_t aligned_ptr = (raw_ptr + sizeof(HeapMetadata) + (REDZONE_SIZE / 2) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

     // free 하려는 힙 객체에 mpu 설정되어 있는지 확인
    // MPU_RNR 레지스터에 리전 번호 설정
    *((volatile uint32_t*)0xE000ED98) = 6;

    // MPU_RBAR 레지스터에서 리전의 Base Address 읽기
    uint32_t base_address = *((volatile uint32_t*)0xE000ED9C) & 0xFFFFFFE0;  // 하위 5비트 제외

    // fault_address가 해당 리전의 주소 범위에 있는지 확인
    if (raw_ptr <  base_address + REDZONE_SIZE/2  && raw_ptr+total_size >= base_address) {

        #ifdef DEBUG
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "MyAlloc: current poison MPU start: %p , current heap start: %p \r\n", base_address, aligned_ptr);
        uart_debug_print(buffer);
        #endif

        
        
        // Red Zone 앞부분 설정 (MPU 영역 2)
        MPU_ConfigureRegion(MPU_REGION_NUMBER6, MPU_REGION_DISABLE, 0x20000000 + (6* 0x100), REDZONE_SIZE / 2, MPU_REGION_PRIV_RO);   


        HAL_MPU_EnableRegion(MPU_REGION_NUMBER2);
        HAL_MPU_EnableRegion(MPU_REGION_NUMBER3);
    }

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

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

    uintptr_t mpu_end_addr = (end_address + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    // free 하려는 힙 객체에 mpu 설정되어 있는지 확인
    // MPU_RNR 레지스터에 리전 번호 설정
    *((volatile uint32_t*)0xE000ED98) = 2;

    // MPU_RBAR 레지스터에서 리전의 Base Address 읽기
    uint32_t base_address = *((volatile uint32_t*)0xE000ED9C) & 0xFFFFFFE0;  // 하위 5비트 제외

    // MPU_RLAR 레지스터에서 리전의 Limit address 읽기
    uint32_t limit_address = *((volatile uint32_t*)0xE000EDA0) & 0xFFFFFFE0;

    #ifdef DEBUG
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Myfree: current heap MPU start: %p , current heap start: %p \r\n", base_address, start_address);
        uart_debug_print(buffer);
        #endif

    // fault_address가 해당 리전의 주소 범위에 있는지 확인
    if ((start_address-REDZONE_SIZE/2) == base_address) {

        #ifdef DEBUG
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Disable MPU when protected heap: %p ~ %p detected\r\n", start_address, end_address);
        uart_debug_print(buffer);
        #endif

        HAL_MPU_Disable();
        
        // Red Zone 앞부분 설정 (MPU 영역 2)
        MPU_ConfigureRegion(MPU_REGION_NUMBER2, MPU_REGION_DISABLE, 0x20000000 + (2* 0x100), REDZONE_SIZE / 2, MPU_REGION_PRIV_RO);  // Red Zone 앞쪽 설정  

        // Red Zone 뒷부분 설정 (MPU 영역 3)
        MPU_ConfigureRegion(MPU_REGION_NUMBER3, MPU_REGION_DISABLE,0x20000000 + (3* 0x100), (REDZONE_SIZE / 2), MPU_REGION_PRIV_RO); // Red Zone 뒤쪽 설정

        HAL_MPU_EnableRegion(MPU_REGION_NUMBER2);
        HAL_MPU_EnableRegion(MPU_REGION_NUMBER3);
    
        HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
    }
    

    // 태그 삭제
    
    remove_tag((void*)start_address,end_address-start_address);
    

    // Free된 영역에 대한 MPU 보호 설정
    configure_mpu_for_poison((void *)(start_address), metadata->size);


    // 전체 메모리 블록 해제
    free(metadata->raw_ptr);  // 메타데이터에 저장된 실제 시작 주소로 전체 블록 해제
}