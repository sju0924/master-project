#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// 외부에 정의된 compare_tag 함수 선언
extern uint8_t compare_tag(void* addr1, void* addr2);

// 커스텀 my_memset 함수
void* my_memset(void* ptr, int value, size_t num) {
    // 시작 포인터와 끝 포인터 계산
    uint8_t* start = (uint8_t*)ptr;
    uint8_t* end = start + num - 1;

    // 시작 포인터와 끝 포인터의 태그 비교
    compare_tag(start, end);

    // 실제 memset 호출
    return memset(ptr, value, num);
}

// 커스텀 my_memcpy 함수
void* my_memcpy(void* dest, const void* src, size_t num) {
    // 시작 포인터와 끝 포인터 계산
    uint8_t* src_start = (uint8_t*)src;
    uint8_t* src_end = src_start + num - 1;
    uint8_t* dest_start = (uint8_t*)dest;
    uint8_t* dest_end = dest_start + num - 1;

    // 원본과 대상 포인터 각각의 시작과 끝 태그 비교
    compare_tag(src_start, src_end);
    compare_tag(dest_start, dest_end);

    // 실제 memcpy 호출
    return memcpy(dest, src, num);
}

// 커스텀 my_memmove 함수
void* my_memmove(void* dest, const void* src, size_t num) {
    // 시작 포인터와 끝 포인터 계산
    uint8_t* src_start = (uint8_t*)src;
    uint8_t* src_end = src_start + num - 1;
    uint8_t* dest_start = (uint8_t*)dest;
    uint8_t* dest_end = dest_start + num - 1;

    // 원본과 대상 포인터 각각의 시작과 끝 태그 비교
    compare_tag(src_start, src_end);
    compare_tag(dest_start, dest_end);

    // 실제 memmove 호출
    return memmove(dest, src, num);
}