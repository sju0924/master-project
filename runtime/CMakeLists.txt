cmake_minimum_required(VERSION 3.13)
project(Runtime)

# 툴체인 파일
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 크로스 컴파일러 설정
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc -x assembler-with-cpp)

# ARM 타겟 설정
set(CMAKE_SYSROOT /opt/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/arm-none-eabi)

# 링크 옵션 및 기타 설정
set(CMAKE_C_FLAGS "-mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m33 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs")


add_definitions(-DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32L562xx)

# 표준 라이브러리 경로 설정
include_directories(${CMAKE_SOURCE_DIR}/stm32/Core/Inc)
include_directories(${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Inc)
include_directories(${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Inc/Legacy)
include_directories(${CMAKE_SOURCE_DIR}/stm32/Drivers/CMSIS/Device/ST/STM32L5xx/Include)
include_directories(${CMAKE_SOURCE_DIR}/stm32/Drivers/CMSIS/Include)

# 표준 라이브러리 경로 설정
include_directories(${CMAKE_SYSROOT}/include)
include_directories(${CMAKE_SYSROOT}/include/c++/13.3.1)
include_directories(${CMAKE_SYSROOT}/include/c++/13.3.1/arm-none-eabi/thumb/v8-m.main+fp/hard)
link_directories(${CMAKE_SYSROOT}/lib)


# set(HAL_DRIVER_SRCS
#     ${CMAKE_SOURCE_DIR}/stm32/Core/Src/main.c
#     ${CMAKE_SOURCE_DIR}/stm32/Core/Src/stm32l5xx_it.c
#     ${CMAKE_SOURCE_DIR}/stm32/Core/Src/stm32l5xx_hal_msp.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_utils.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_exti.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_adc.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_adc_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_i2c.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_i2c_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_rcc.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_rcc_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_flash.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_flash_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_flash_ramfunc.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_gpio.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_dma.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_dma_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_exti.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_pwr.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_pwr_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_cortex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_icache.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_dfsdm.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_dfsdm_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_fmc.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_sram.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_uart.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_uart_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_ospi.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_sai.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_sai_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_sdmmc.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_sd.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_sd_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_spi.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_spi_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_tim.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_tim_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_ucpd.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_gpio.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_dma.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_pcd.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_hal_pcd_ex.c
#     ${CMAKE_SOURCE_DIR}/stm32/Drivers/STM32L5xx_HAL_Driver/Src/stm32l5xx_ll_usb.c
#     ${CMAKE_SOURCE_DIR}/stm32/Core/Src/system_stm32l5xx.c
# )

set(RUNTIME_SRCS
    testPrint.c
)
add_library(runtime OBJECT ${RUNTIME_SRCS})

# 개별 .o 파일들을 runtime.o로 묶기
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/runtime/runtime.o
    COMMAND arm-none-eabi-ld -r $<TARGET_OBJECTS:runtime> -o ${CMAKE_BINARY_DIR}/runtime/runtime.o
    DEPENDS runtime
)

add_custom_target(runtime_merged ALL DEPENDS ${CMAKE_BINARY_DIR}/runtime/runtime.o)
