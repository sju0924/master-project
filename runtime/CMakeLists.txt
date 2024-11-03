cmake_minimum_required(VERSION 3.13)
project(Runtime)

# 툴체인 파일 설정
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 크로스 컴파일러 설정
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# ARM 타겟 설정
set(CMAKE_SYSROOT /opt/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/arm-none-eabi)

# 컴파일 및 링크 옵션
set(CMAKE_C_FLAGS "-mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m33 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs")

add_definitions(-DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32L562xx)

# Include 디렉토리 설정
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

# 소스 파일 설정
set(RUNTIME_SRCS
    testPrint.c
    tagManager.c
    heapManager.c
    debugger.c
    intrinsicFunction.c
)

# OBJECT 라이브러리 생성
add_library(runtime OBJECT ${RUNTIME_SRCS})

# runtime.o 파일을 생성하는 명령어
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/runtime/runtime.o
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/runtime
    COMMAND_EXPAND_LISTS
    COMMAND arm-none-eabi-ld -r $<TARGET_OBJECTS:runtime> -o ${CMAKE_BINARY_DIR}/runtime/runtime.o
    DEPENDS runtime
    COMMENT "Linking all object files into runtime.o"
    VERBATIM
)

# runtime_merged 타겟을 생성하여 runtime.o 빌드를 보장
add_custom_target(runtime_merged ALL DEPENDS ${CMAKE_BINARY_DIR}/runtime/runtime.o)
