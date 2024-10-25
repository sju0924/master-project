cmake_minimum_required(VERSION 3.13)
project(TestPass)
# ARM 타겟 설정
# set(CMAKE_SYSROOT /opt/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/arm-none-eabi)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# 표준 라이브러리 경로 설정
include_directories(${CMAKE_SYSROOT}/include)
include_directories(${CMAKE_SYSROOT}/include/c++/13.3.1)
include_directories(${CMAKE_SYSROOT}/include/c++/13.3.1/arm-none-eabi/thumb/v8-m.main+fp/hard)
link_directories(${CMAKE_SYSROOT}/lib)
# include_directories(${CMAKE_SOURCE_DIR}/CMSIS_5/CMSIS/Core/Include)


#add_definitions(-DSTM32_ENV)  # 모든 타겟에 대해 매크로 정의
# add_definitions(--target=arm-none-eabi)
# add_definitions(-mcpu=cortex-m33)
# add_definitions(-mthumb)
# add_definitions(-stdlib=libc++)

# 호스트에서 HAL 드라이버 코드를 제외하여 TestPass.so를 생성
if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    add_library(TestPass MODULE TestPass.cpp)
else()
    add_library(TestPass MODULE TestPass.cpp ${HAL_DRIVER_SRCS})
endif()

target_link_libraries(TestPass PRIVATE c++ m LLVM)
