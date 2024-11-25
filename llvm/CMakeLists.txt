cmake_minimum_required(VERSION 3.13)
project(TestPass)

# 표준 라이브러리 경로 설정
include_directories(${CMAKE_SYSROOT}/include)
include_directories(${CMAKE_SYSROOT}/include/c++/13.3.1)
include_directories(${CMAKE_SYSROOT}/include/c++/13.3.1/arm-none-eabi/thumb/v8-m.main+fp/hard)
link_directories(${CMAKE_SYSROOT}/lib)


add_library(TestPass MODULE TestPass.cpp)
add_library(TagPass MODULE TagPass.cpp)
add_library(MPUPass MODULE mpupass.cpp)

# 필요한 라이브러리 연결
target_link_libraries(TestPass PRIVATE c++ m LLVM)
target_link_libraries(TagPass PRIVATE c++ m LLVM)
target_link_libraries(MPUPass PRIVATE c++ m LLVM)

# mpupass.h 헤더 파일 디렉터리 설정
target_include_directories(TagPass PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(MPUPass PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


