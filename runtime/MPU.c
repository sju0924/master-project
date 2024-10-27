// #include "stm32l5xx.h"
// #include "stm32l5xx_hal_conf.h"
// #include "stm32l562xx.h"  // IRQn_Type 및 Cortex-M33 장치 관련 정의 포함
// #include "core_cm33.h"
#include <stdint.h>
#define  MPU_HFNMI_PRIVDEF_NONE          0U

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


void MPU_ConfigureRegion(uint32_t num, uint32_t base_address, uint32_t size, uint32_t access_permission) {
    
    MPU_Region_InitTypeDef* region;

    region->Enable = 1;
    region->Number = num;
    region->BaseAddress = base_address;
    region->LimitAddress = base_address + size;
    region->AttributesIndex = 0;
    region->AccessPermission = 0;
    region->DisableExec = 0;
    region->IsShareable = 0;

    HAL_MPU_ConfigRegion(region);

}
