set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(ZLIB_LIBRARY /usr/lib/x86_64-linux-gnu/libz.so)
set(ZLIB_INCLUDE_DIR /usr/include)

set(zstd_LIBRARY /usr/lib/x86_64-linux-gnu/libzstd.so)
set(zstd_INCLUDE_DIR /usr/include)

set(Terminfo_LIBRARIES /usr/lib/x86_64-linux-gnu/libtinfo.so)

set(FFI_LIBRARIES /usr/lib/x86_64-linux-gnu/libffi.so)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_LINKER /usr/bin/clang++)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_NAME Linux)

cmake_minimum_required(VERSION 3.13)
project(MasterProject C CXX ASM)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})


message(STATUS "System Name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "System Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "C FLAGS: ${CMAKE_C_FLAGS}")



# === 사용할 MCU 모델 정의 ===
# MCU 모델 정의. STM32L562xx를 사용.
add_definitions(-DSTM32L562xx)
add_subdirectory(${CMAKE_SOURCE_DIR}/llvm)
add_subdirectory(${CMAKE_SOURCE_DIR}/runtime)

# === C 프로젝트 빌드 (Makefile 사용) ===
# C 프로젝트가 Makefile로 관리되므로, CMake에서 이를 호출
# /stm32 폴더에서 firmware라는 이름으로 빌드
add_custom_target(firmware ALL
    COMMAND make -C ${CMAKE_SOURCE_DIR}/stm32
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/stm32
)


# LLVM Pass 적용 명령 추가
add_custom_target(
    apply_pass
    DEPENDS firmware 

    # Stack, Heap MPU 보호 설정
    COMMAND opt -load-pass-plugin=${CMAKE_BINARY_DIR}/llvm/libMPUPass.so -passes="my-test-pass,stack-mpu-pass,heap-mpu-pass" -S -o ${CMAKE_BINARY_DIR}/application_mpu_heap_stack_analysed.ll  ${CMAKE_BINARY_DIR}/../stm32/build/application.ll
    
    # Global Variable MPU 설정
    COMMAND opt -load-pass-plugin=${CMAKE_BINARY_DIR}/llvm/libMPUPass.so -passes="global-variable-mpu-pass" -S -o ${CMAKE_BINARY_DIR}/application_mpu_gv_analysed.ll  ${CMAKE_BINARY_DIR}/application_mpu_heap_stack_analysed.ll
    
    # Stack tag 설정 및 태그 연산 분석
    COMMAND opt  -load-pass-plugin=${CMAKE_BINARY_DIR}/llvm/libTagPass.so -passes="stack-tag-pass,arithmetic-pointer-tag-pass" -S -o ${CMAKE_BINARY_DIR}/application_stack_tag_output.ll  ${CMAKE_BINARY_DIR}/application_tagged.ll

    # Struct 메타데이터 수집 및 전역 변수 tag 설정
    COMMAND opt -load-pass-plugin=${CMAKE_BINARY_DIR}/llvm/libTagPass.so -passes="global-variable-tag-pass,struct-metadata-pass" -S -o ${CMAKE_BINARY_DIR}/application_output.ll ${CMAKE_BINARY_DIR}/application_stack_tag_output.ll
    
    # Null pointer mpu 보호 설정
    COMMAND opt -load-pass-plugin=${CMAKE_BINARY_DIR}/llvm/libMPUPass.so -passes="null-ptr-mpu-pass" -S -o  ${CMAKE_BINARY_DIR}/drivers_analysed.ll  ${CMAKE_BINARY_DIR}/../stm32/drivers.ll

    # Application 및 Link
    COMMAND llvm-link -o output.ll ${CMAKE_BINARY_DIR}/drivers_analysed.ll  ${CMAKE_BINARY_DIR}/application_output.ll 

)
add_custom_target(
    build
    DEPENDS apply_pass
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND llc -filetype=obj -march=arm -mcpu=cortex-m33 --float-abi=hard output.ll -o output.o 
    COMMAND arm-none-eabi-gcc -c ../stm32/startup_stm32l562xx.s -o startup.o
    COMMAND arm-none-eabi-gcc output.o startup.o runtime/runtime.o -DSTM32_ENV -o firmware.elf -T ../stm32/STM32L562xE_SHADOW.ld -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-short-enums --specs=nosys.specs  
    COMMAND arm-none-eabi-objcopy -O binary firmware.elf firmware.bin

)