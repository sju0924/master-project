// #include "stm32l5xx.h"
// #include "stm32l5xx_hal_conf.h"
// #include "stm32l562xx.h"  // IRQn_Type 및 Cortex-M33 장치 관련 정의 포함
// #include "core_cm33.h"
#include <stdint.h>
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

#if (__MPU_PRESENT == 1)
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


const unsigned int REDZONE_SIZE = ARM_MPU_REGION_SIZE_32B;

void HAL_MPU_Enable(uint32_t MPU_Control);
void HAL_MPU_EnableRegion(uint32_t RegionNumber);
void HAL_MPU_ConfigRegion(MPU_Region_InitTypeDef *MPU_RegionInit);
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
void MPU_ConfigureRegion(uint32_t region, uint32_t base_address, uint32_t size, uint32_t access_permission) ;

void MPU_Enable(void) {
    HAL_MPU_Enable(MPU_HFNMI_PRIVDEF_NONE);
     for (int i = 0; i < 6; i++) {
        HAL_MPU_EnableRegion(i);
     }
}

void configure_mpu_redzone_for_call(uint32_t stack_frame_size) {
    uint32_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));  // 현재 SP 가져오기

    // Red Zone의 앞뒤 주소 계산
    uint32_t front_addr = sp - stack_frame_size - REDZONE_SIZE;
    uint32_t back_addr = sp + REDZONE_SIZE;

    HAL_MPU_Disable();

    // Red Zone 앞부분 설정 (MPU 영역 0)
    MPU_ConfigureRegion(MPU_REGION_NUMBER0, front_addr, REDZONE_SIZE, MPU_PRIVILEGED_DEFAULT);  // Red Zone 앞쪽 설정  

    // Red Zone 뒷부분 설정 (MPU 영역 1)
    MPU_ConfigureRegion(MPU_REGION_NUMBER1, back_addr - REDZONE_SIZE, REDZONE_SIZE, MPU_PRIVILEGED_DEFAULT); // Red Zone 뒤쪽 설정
  
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}


void MPU_ConfigureRegion(uint32_t region_num, uint32_t base_address, uint32_t size, uint32_t access_permission) {
    
    MPU_Region_InitTypeDef* region;

    region->Enable = MPU_REGION_ENABLE;
    region->Number = region_num;
    region->BaseAddress = base_address;
    region->LimitAddress = base_address + size;
    region->AttributesIndex = 0;
    MPU_InitStruct.AccessPermission = access_permission;          // 전체 접근 권한 부여
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;        // 실행 가능
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 


    HAL_MPU_ConfigRegion(region);

}
