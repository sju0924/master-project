# toolchain.cmake

# 크로스 컴파일러 설정
set(CMAKE_C_COMPILER /opt/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /opt/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER /opt/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc)

# MCU에 대한 설정 (STM32L5 예시)


set(CMAKE_C_FLAGS "-mcpu=cortex-m33 -mthumb")
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/stm32/STM32L562xE_FLASH.ld)
set(STARTUP_FILE ${CMAKE_SOURCE_DIR}/stm32/startup_stm32l562xx.s)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")

