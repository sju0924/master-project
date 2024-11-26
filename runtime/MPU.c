// #include "stm32l5xx.h"
// #include "stm32l5xx_hal_conf.h"
// #include "stm32l562xx.h"  // IRQn_Type 및 Cortex-M33 장치 관련 정의 포함
// #include "core_cm33.h"

#define  MPU_HFNMI_PRIVDEF_NONE          0U
/* Exported constants --------------------------------------------------------*/

/** @defgroup CORTEX_Exported_Constants CORTEX Exported Constants
  * @{
  */

/** @defgroup CORTEX_Preemption_Priority_Group CORTEX Preemption Priority Group
  * @{
  */
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 1 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
/**
  * @}
  */

/** @defgroup CORTEX_SysTick_clock_source CORTEX SysTick clock source
  * @{
  */
#define SYSTICK_CLKSOURCE_HCLK_DIV8    ((uint32_t)0x00000000)
#define SYSTICK_CLKSOURCE_HCLK         ((uint32_t)0x00000004)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_HFNMI_PRIVDEF_Control CORTEX MPU HFNMI and PRIVILEGED Access control
  * @{
  */
#define  MPU_HFNMI_PRIVDEF_NONE          0U
#define  MPU_HARDFAULT_NMI               2U
#define  MPU_PRIVILEGED_DEFAULT          4U
#define  MPU_HFNMI_PRIVDEF               6U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Region_Enable CORTEX MPU Region Enable
  * @{
  */
#define  MPU_REGION_ENABLE               1U
#define  MPU_REGION_DISABLE              0U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Instruction_Access CORTEX MPU Instruction Access
  * @{
  */
#define  MPU_INSTRUCTION_ACCESS_ENABLE   0U
#define  MPU_INSTRUCTION_ACCESS_DISABLE  1U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Access_Shareable CORTEX MPU Instruction Access Shareable
  * @{
  */
#define  MPU_ACCESS_NOT_SHAREABLE        0U
#define  MPU_ACCESS_OUTER_SHAREABLE      2U
#define  MPU_ACCESS_INNER_SHAREABLE      3U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Region_Permission_Attributes CORTEX MPU Region Permission Attributes
  * @{
  */
#define  MPU_REGION_PRIV_RW              0U
#define  MPU_REGION_ALL_RW               1U
#define  MPU_REGION_PRIV_RO              2U
#define  MPU_REGION_ALL_RO               3U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Region_Number CORTEX MPU Region Number
  * @{
  */
#define  MPU_REGION_NUMBER0              0U
#define  MPU_REGION_NUMBER1              1U
#define  MPU_REGION_NUMBER2              2U
#define  MPU_REGION_NUMBER3              3U
#define  MPU_REGION_NUMBER4              4U
#define  MPU_REGION_NUMBER5              5U
#define  MPU_REGION_NUMBER6              6U
#define  MPU_REGION_NUMBER7              7U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Attributes_Number CORTEX MPU Memory Attributes Number
  * @{
  */
#define  MPU_ATTRIBUTES_NUMBER0          0U
#define  MPU_ATTRIBUTES_NUMBER1          1U
#define  MPU_ATTRIBUTES_NUMBER2          2U
#define  MPU_ATTRIBUTES_NUMBER3          3U
#define  MPU_ATTRIBUTES_NUMBER4          4U
#define  MPU_ATTRIBUTES_NUMBER5          5U
#define  MPU_ATTRIBUTES_NUMBER6          6U
#define  MPU_ATTRIBUTES_NUMBER7          7U
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Attributes CORTEX MPU Attributes
  * @{
  */
#define  MPU_DEVICE_nGnRnE          0x0U  /* Device, noGather, noReorder, noEarly acknowledge. */
#define  MPU_DEVICE_nGnRE           0x4U  /* Device, noGather, noReorder, Early acknowledge.   */
#define  MPU_DEVICE_nGRE            0x8U  /* Device, noGather, Reorder, Early acknowledge.     */
#define  MPU_DEVICE_GRE             0xCU  /* Device, Gather, Reorder, Early acknowledge.       */

#define  MPU_WRITE_THROUGH          0x0U  /* Normal memory, write-through. */
#define  MPU_NOT_CACHEABLE          0x4U  /* Normal memory, non-cacheable. */
#define  MPU_WRITE_BACK             0x4U  /* Normal memory, write-back.    */

#define  MPU_TRANSIENT              0x0U  /* Normal memory, transient.     */
#define  MPU_NON_TRANSIENT          0x8U  /* Normal memory, non-transient. */

#define  MPU_NO_ALLOCATE            0x0U  /* Normal memory, no allocate.         */
#define  MPU_W_ALLOCATE             0x1U  /* Normal memory, write allocate.      */
#define  MPU_R_ALLOCATE             0x2U  /* Normal memory, read allocate.       */
#define  MPU_RW_ALLOCATE            0x3U  /* Normal memory, read/write allocate. */

#define OUTER(__ATTR__)        ((__ATTR__) << 4U)
#define INNER_OUTER(__ATTR__)  ((__ATTR__) | ((__ATTR__) << 4U))

#include "runtimeConfig.h"


/**
  * @}
  */

typedef struct
{
  uint8_t                Enable;                /*!< Specifies the status of the region. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Enable                 */
  uint8_t                Number;                /*!< Specifies the number of the region to protect. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Number                 */
  uint32_t               BaseAddress;           /*!< Specifies the base address of the region to protect.                           */
  uint32_t               LimitAddress;          /*!< Specifies the limit address of the region to protect.                          */
  uint8_t                AttributesIndex;       /*!< Specifies the memory attributes index.
                                                     This parameter can be a value of @ref CORTEX_MPU_Attributes_Number             */
  uint8_t                AccessPermission;      /*!< Specifies the region access permission type. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Permission_Attributes  */
  uint8_t                DisableExec;           /*!< Specifies the instruction access status. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Instruction_Access            */
  uint8_t                IsShareable;           /*!< Specifies the shareability status of the protected region. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Access_Shareable              */
} MPU_Region_InitTypeDef;


void HAL_MPU_Enable(uint32_t MPU_Control);
void HAL_MPU_EnableRegion(uint32_t RegionNumber);
void HAL_MPU_Disable();
void HAL_MPU_ConfigRegion(MPU_Region_InitTypeDef *MPU_RegionInit);
void uart_debug_print(const char *str);
/*
MPU_Enable: MPU 활성화
*/
void MPU_Enable(void) ;

/*
MPU_ConfigureRegion: MPU 영역 설정
@input
    - uint32_t region: 리전 번호
    - uint32_t base_address: MPU 영역 시작 주소(32의 배수)
    - uint32_t size: MPU 영역 크기(32의 배수)
    - uint32_t access_permission: 권한
*/
void MPU_ConfigureRegion(uint32_t region_num, uint32_t enable, uint32_t base_address, uint32_t size, uint32_t access_permission);

void MPU_Enable(void) {
    HAL_MPU_Enable(MPU_HFNMI_PRIVDEF_NONE);
     for (int i = 0; i < 6; i++) {
        HAL_MPU_EnableRegion(i);
     }
}

void configure_mpu_redzone_for_call(uint32_t sp, uint32_t r7) {

    HAL_MPU_Disable();

    #ifdef DEBUG
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Stack Pointer and R7 values:  SP: %p, R7: %p", sp, r7);
    uart_debug_print(buffer);
    #endif

    // Red Zone의 앞뒤 주소 계산
    uint32_t front_addr = sp & ~(uintptr_t)(ALIGNMENT - 1); // Redzone 0이 시작되는 주소
    uint32_t back_addr = (r7 + ALIGNMENT - 1) & ~(uintptr_t)(ALIGNMENT - 1);; // Redzone 1이 시작하는 주소

    // 디버그
    #ifdef DEBUG
    snprintf(buffer, sizeof(buffer), "Stack MPU started: %p, ended: %p", (void *)front_addr, (void *)back_addr);
    uart_debug_print(buffer);
    #endif

    

    // Red Zone 앞부분 설정 (MPU 영역 0)
    MPU_ConfigureRegion(MPU_REGION_NUMBER0, MPU_REGION_ENABLE, front_addr - REDZONE_SIZE/2, REDZONE_SIZE/2, MPU_REGION_PRIV_RO);  // Red Zone 앞쪽 설정  

    // Red Zone 뒷부분 설정 (MPU 영역 1)
    MPU_ConfigureRegion(MPU_REGION_NUMBER1, MPU_REGION_ENABLE, back_addr, REDZONE_SIZE/2, MPU_REGION_PRIV_RO); // Red Zone 뒤쪽 설정
  
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void configure_mpu_redzone_for_return() {
  
    HAL_MPU_Disable();

    uint32_t sp, r7;
    __asm__ volatile("mov %0, sp" : "=r"(sp));  // 현재 SP 가져오기
    __asm__ volatile("mov %0, r7" : "=r"(r7));  // 현재 SP 가져오기

    sp = sp & ~(uintptr_t)(ALIGNMENT - 1);
    r7 = (r7 + ALIGNMENT - 1) & ~(uintptr_t)(ALIGNMENT - 1);

    // Red Zone의 앞뒤 주소 계산
    uint32_t front_addr = sp; // Redzone 0이 시작되는 주소
    uint32_t back_addr = r7; // Redzone 1이 시작하는 주소

    // 디버그
    #ifdef DEBUG
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Unset Stack Pointer and R7 values:  SP: %x, R7: %x", sp, r7);
    uart_debug_print(buffer);
    #endif


    // Red Zone 앞부분 설정 (MPU 영역 0)
    MPU_ConfigureRegion(MPU_REGION_NUMBER0, MPU_REGION_DISABLE, front_addr - REDZONE_SIZE/2, REDZONE_SIZE/2, MPU_REGION_ALL_RO);  // Red Zone 앞쪽 설정  

    // Red Zone 뒷부분 설정 (MPU 영역 1)
    MPU_ConfigureRegion(MPU_REGION_NUMBER1, MPU_REGION_DISABLE, back_addr, REDZONE_SIZE/2, MPU_REGION_ALL_RO); // Red Zone 뒤쪽 설정

    HAL_MPU_EnableRegion(MPU_REGION_NUMBER0);
    HAL_MPU_EnableRegion(MPU_REGION_NUMBER1);
  
    HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);
}

void configure_mpu_redzone_for_heap_access(void* ptr){

      // MPU 설정
    HAL_MPU_Disable();

        if (!ptr) {
        printf("Invalid pointer\n");
        return;
    }

    // 메타데이터 위치를 계산
    HeapMetadata* metadata = (HeapMetadata*)(((uintptr_t)ptr - (REDZONE_SIZE / 2) - sizeof(HeapMetadata)) & ~(4 - 1));

    // 힙 객체의 시작과 끝 주소 계산
    uintptr_t start_addr = (uintptr_t)ptr;
    uintptr_t end_addr = start_addr + metadata->size;
    end_addr = (end_addr + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    // 디버그
    #ifdef DEBUG
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Set Heap redzone: ptr: %p, size: %d", (void*)ptr, metadata->size);
    uart_debug_print(buffer);
    snprintf(buffer, sizeof(buffer), "Heap MPU started: %p, ended: %p", (void *)start_addr, (void *)end_addr);
    uart_debug_print(buffer);
    #endif
    



    // Red Zone 앞부분 설정 (MPU 영역 2)
    MPU_ConfigureRegion(MPU_REGION_NUMBER2, MPU_REGION_ENABLE, start_addr - (REDZONE_SIZE / 2), REDZONE_SIZE / 2, MPU_REGION_PRIV_RO);  // Red Zone 앞쪽 설정  

    // Red Zone 뒷부분 설정 (MPU 영역 3)
    MPU_ConfigureRegion(MPU_REGION_NUMBER3, MPU_REGION_ENABLE, end_addr, (REDZONE_SIZE / 2), MPU_REGION_PRIV_RO); // Red Zone 뒤쪽 설정

    HAL_MPU_EnableRegion(MPU_REGION_NUMBER2);
    HAL_MPU_EnableRegion(MPU_REGION_NUMBER3);
  
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

void configure_mpu_redzone_for_global(void *ptr, uint64_t size) {
      // MPU 설정
    HAL_MPU_Disable();


    uintptr_t start_addr = (uintptr_t)ptr;
    uintptr_t end_addr = start_addr + size;
    end_addr = (end_addr + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);

    // 디버그
    #ifdef DEBUG
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Set Global redzone:  Start address: %p, end address: %p, global variable start: %p", (void*)start_addr, (void*)end_addr, ptr);
    uart_debug_print(buffer);
    #endif


    // Red Zone 앞부분 설정 (MPU 영역 4)
    MPU_ConfigureRegion(MPU_REGION_NUMBER4, MPU_REGION_ENABLE, start_addr - (REDZONE_SIZE / 2), REDZONE_SIZE / 2, MPU_REGION_PRIV_RO);  // Red Zone 앞쪽 설정  

    // Red Zone 뒷부분 설정 (MPU 영역 5)
    MPU_ConfigureRegion(MPU_REGION_NUMBER5, MPU_REGION_ENABLE, end_addr, (REDZONE_SIZE / 2), MPU_REGION_PRIV_RO); // Red Zone 뒤쪽 설정

    HAL_MPU_EnableRegion(MPU_REGION_NUMBER4);
    HAL_MPU_EnableRegion(MPU_REGION_NUMBER5);
  
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void configure_mpu_for_poison(void *ptr, uint32_t size) {
      // MPU 설정
    HAL_MPU_Disable();

    uintptr_t start_addr = (uintptr_t)ptr;
    uintptr_t end_addr = start_addr + size;
    end_addr = (end_addr+ ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    // 디버그
    #ifdef DEBUG
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Set MPU-Protected Poison:  Start address: %p, end address: %p, size: %zu\r\n", (void*)start_addr, (void*)end_addr, (size_t)(end_addr-start_addr));
    uart_debug_print(buffer);
    #endif



    
    MPU_ConfigureRegion(MPU_REGION_NUMBER6, MPU_REGION_ENABLE, start_addr, (size_t)(end_addr-start_addr) , MPU_REGION_PRIV_RO);
    HAL_MPU_EnableRegion(MPU_REGION_NUMBER6);
      
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void configure_mpu_for_null_ptr(){
  // MPU 설정
  HAL_MPU_Disable();
  MPU_ConfigureRegion(MPU_REGION_NUMBER7, MPU_REGION_ENABLE, 0x0, 0x20 , MPU_REGION_PRIV_RO);
  HAL_MPU_EnableRegion(MPU_REGION_NUMBER7);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

   // 디버그
  #ifdef DEBUG
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "Set MPU-Protected NullPtr\r\n"); 
  uart_debug_print(buffer);
  #endif
}


void MPU_ConfigureRegion(uint32_t region_num, uint32_t enable, uint32_t base_address, uint32_t size, uint32_t access_permission) {
    MPU_Region_InitTypeDef region;  // 지역 변수로 선언

    region.Enable = enable;
    region.Number = region_num;
    region.BaseAddress = base_address;
    region.LimitAddress = base_address + size - 1;  // LimitAddress는 마지막 주소이므로 -1 필요
    region.AttributesIndex = 0;
    region.AccessPermission = access_permission;  // 접근 권한 설정
    region.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;  // 실행 가능
    region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;  // 공유 불가 설정

    // 설정된 지역 변수를 사용하여 MPU 영역 구성
    HAL_MPU_ConfigRegion(&region);
}

