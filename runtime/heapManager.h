#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "runtimeConfig.h"
#include "tagManager.h"

void uart_debug_print(const char *str);
void HAL_MPU_Enable(uint32_t MPU_Control);
void HAL_MPU_EnableRegion(uint32_t RegionNumber);
void HAL_MPU_Disable();
void MPU_ConfigureRegion(uint32_t region_num, uint32_t enable, uint32_t base_address, uint32_t size, uint32_t access_permission);

#define  MPU_REGION_NUMBER2              2U
#define  MPU_REGION_NUMBER3              3U
#define  MPU_REGION_NUMBER6              6U

#define  MPU_REGION_DISABLE              0U
#define  MPU_PRIVILEGED_DEFAULT          4U
#define  MPU_REGION_PRIV_RO              2U
