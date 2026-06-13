#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <setjmp.h>

extern uint8_t compare_tag(void* addr1, void* addr2);

/* test runner state — defined in test_runner.c, linked at final link step */
extern volatile int g_test_running;
extern jmp_buf      g_test_recovery;

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

char* my_strcpy(char* dest, const char* origin){
    size_t len = 0;
    char *ptr = origin;
    while(*ptr++ != '\0'){
        len++;
    }

    compare_tag(dest, dest+len);
    return strcpy(dest, origin);
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

/*
 * Wide-string intrinsics: CWE193/CWE126 계열처럼 wchar_t 버퍼를 대상으로 하는
 * 오버플로우는 wcscpy/wcsncpy/wmemcpy 가 태그 검사 없이 호출되어 탐지되지 않는다.
 * my_strcpy 와 동일한 패턴으로 dest 시작~복사 끝 주소의 태그를 비교한다.
 * compare_tag 는 void* 를 받으므로 wchar_t* 포인터 산술 결과가 바이트 주소로
 * 올바르게 전달된다.
 */
wchar_t* my_wcscpy(wchar_t* dest, const wchar_t* src) {
    size_t len = wcslen(src);           /* null 제외 wchar_t 개수 */
    compare_tag(dest, dest + len);      /* dest[0] vs dest[len] (오버플로우 착지점) */
    return wcscpy(dest, src);
}

wchar_t* my_wcsncpy(wchar_t* dest, const wchar_t* src, size_t n) {
    if (n > 0) {
        compare_tag(dest, dest + n - 1);
    }
    return wcsncpy(dest, src, n);
}

wchar_t* my_wcscat(wchar_t* dest, const wchar_t* src) {
    size_t dest_len = wcslen(dest);
    size_t src_len  = wcslen(src);
    compare_tag(dest, dest + dest_len + src_len);
    return wcscat(dest, src);
}

wchar_t* my_wmemcpy(wchar_t* dest, const wchar_t* src, size_t n) {
    if (n > 0) {
        compare_tag((uint8_t*)src,  (uint8_t*)(src  + n) - 1);
        compare_tag((uint8_t*)dest, (uint8_t*)(dest + n) - 1);
    }
    return wmemcpy(dest, src, n);
}

wchar_t* my_wmemmove(wchar_t* dest, const wchar_t* src, size_t n) {
    if (n > 0) {
        compare_tag((uint8_t*)src,  (uint8_t*)(src  + n) - 1);
        compare_tag((uint8_t*)dest, (uint8_t*)(dest + n) - 1);
    }
    return wmemmove(dest, src, n);
}

/* ── narrow-string intrinsics ────────────────────────────────────────── */

char* my_strncpy(char* dest, const char* src, size_t n) {
    if (n > 0) {
        compare_tag((uint8_t*)dest, (uint8_t*)dest + n - 1);
    }
    return strncpy(dest, src, n);
}

char* my_strcat(char* dest, const char* src) {
    size_t dest_len = strlen(dest);
    size_t src_len  = strlen(src);
    compare_tag((uint8_t*)dest, (uint8_t*)dest + dest_len + src_len);
    return strcat(dest, src);
}

/* strncat writes at most n chars then a null terminator at dest[dest_len+n] */
char* my_strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    compare_tag((uint8_t*)dest, (uint8_t*)dest + dest_len + n);
    return strncat(dest, src, n);
}

/* ── exit interception ───────────────────────────────────────────────── */
/*
 * Replaces exit() via -Dexit=my_exit in APPLICATION_FLAGS.
 * When called from within a test case, redirects to longjmp so the test
 * runner survives malloc-NULL guards (e.g. CWE191, CWE127) without freezing.
 * g_error_detected is NOT set here — exit(-1) from a NULL guard is not the
 * vulnerability itself, so the verdict correctly becomes FAIL-MISS.
 */
__attribute__((noreturn)) void my_exit(int status) {
    (void)status;
    if (g_test_running) {
        longjmp(g_test_recovery, 1);
    }
    while (1);
}