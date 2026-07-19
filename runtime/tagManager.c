#include "tagManager.h"
#include <setjmp.h>
#include <string.h>

/* Shared with test_runner.c – resolved at link time */
extern jmp_buf      g_test_recovery;
extern volatile int g_test_running;
extern volatile int g_error_detected;

uint8_t prev_tag = 0;


void uart_debug_print(const char *str);

uint8_t tag_generator(){
    uint8_t tag = 0x00;
    while(tag == prev_tag || tag == 0x00 || tag == UNPOISON_TAG || tag == POISON_TAG){
        tag = rand() % 0xFF;
    }

    prev_tag = tag;

    return tag;
}
// 주어진 RAM 주소에 해당하는 태그 테이블 주소를 반환
uint8_t* get_tag_address(void* address) {
    if ((uintptr_t)address < RAM_START || (uintptr_t)address >= RAM_END){
        return 0;
    }
    uint32_t offset = ((uintptr_t)address - RAM_START) / 8;
    return (uint8_t *)(RAM_END + offset);
}

void set_tag(void *address, size_t size) {
    uint8_t *tag_address;
    char buffer[100];
    if(size == 0){
        return;
    }

    if(tag_address = get_tag_address(address)){
        uint8_t tag = tag_generator();
        uintptr_t end_address = (uintptr_t)address + size - 1;
        uint8_t *tag_end = get_tag_address((void*)end_address);

        while (tag_end && tag_address <= tag_end) {
            *tag_address++ = tag;
        }

        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Tag assigned at: %p, size: %d tag: %u", address, size, tag);
        uart_debug_print(buffer);
        #endif
    }


}

void remove_tag(void *address, size_t size) {
    uint8_t *tag_address;
    char buffer[100];
    if(size == 0){
        return;
    }

    if(tag_address = get_tag_address(address)){
        uint8_t tag = UNPOISON_TAG;
        uintptr_t end_address = (uintptr_t)address + size - 1;
        uint8_t *tag_end = get_tag_address((void*)end_address);

        while (tag_end && tag_address <= tag_end) {
            *tag_address++ = tag;
        }


    }
    #ifdef DEBUG
    snprintf(buffer, sizeof(buffer), "Tag removed at: %p, size: %d tag: %u", address, size, UNPOISON_TAG);
    uart_debug_print(buffer);
    #endif


}

void set_tag_padding(void *address, size_t size) {
    uint8_t *tag_address;
    if(size == 0){
        return;
    }

    if(tag_address = get_tag_address(address)){
        uint8_t padding_tag = 0x00;
        uintptr_t end_address = (uintptr_t)address + size - 1;
        uint8_t *tag_end = get_tag_address((void*)end_address);

        while (tag_end && tag_address <= tag_end) {
            *tag_address++ = padding_tag;
        }
    }
}

// 구조체 필드마다 태그를 설정하는 함수
void set_struct_tags(void *struct_address, uint32_t item_index) {
    char buffer[100];
    uint32_t index = 0;

    // flat metadata 배열에서 item_index 구조체의 첫 멤버 위치 계산
    for(uint32_t i = 0 ; i < item_index ; i++){
        index += struct_member_counts[i];
    }

    // 구조체 메타데이터 불러오기
    uint32_t* member_offsets = struct_member_offsets + index;
    uint32_t* member_sizes = struct_member_sizes + index;
    uint32_t num_members = struct_member_counts[item_index];

    uintptr_t base_address = (uintptr_t)struct_address;

    #ifdef DEBUG
    if(num_members){//debug
        snprintf(buffer, sizeof(buffer), "Tag metadata size: %zu, base address: %p\n", num_members, base_address + member_offsets[0]);
        uart_debug_print(buffer);
        snprintf(buffer, sizeof(buffer), "Address of tables: offset: %p, size: %p\n", member_offsets, member_sizes);
        uart_debug_print(buffer);
        snprintf(buffer, sizeof(buffer), "Second element of member_offsets: %zu, size: %zu\n", member_offsets[1], member_sizes[1]);
        uart_debug_print(buffer);
    }
    #endif

    for (uint32_t i = 0; i < num_members; i++) {
        uintptr_t member_address = base_address + member_offsets[i];
        uint32_t member_size = (uint32_t)member_sizes[i];
        if (member_size == 0) {
            continue;
        }

        uintptr_t member_end = member_address + member_size - 1;

        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Member address: %d, member size: %zu\n", member_address, member_size);
        uart_debug_print(buffer);
        #endif

        if (member_address < RAM_START || member_end >= RAM_END) {
            continue; // 유효하지 않은 태그 주소는 무시
        }

        uint8_t current_tag = tag_generator();
        uint8_t *tag_address = get_tag_address((void*)member_address);
        uint8_t *tag_end = get_tag_address((void*)member_end);

        while (tag_address && tag_end && tag_address <= tag_end) {
            *tag_address++ = current_tag;
        }

        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Tag assigned: %u\n",  current_tag);
        uart_debug_print(buffer);
        #endif

    }
}

uint8_t get_tag(void *address) {
    uint8_t *tag_address;
    uint8_t tag = 0;
    if(tag_address = get_tag_address(address)){
        tag = *tag_address;
    }
    return tag;
}

void check_live_tag(void *address) {
    uint8_t *tag_address = get_tag_address(address);

    if (tag_address && *tag_address == UNPOISON_TAG) {
        handle_tag_mismatch(address, address);
    }
}

// 두 주소의 태그를 비교하는 함수
uint8_t compare_tag(void* addr1, void* addr2) {

    char buffer[100];

    uint8_t* tag1 = get_tag_address(addr1);
    uint8_t* tag2 = get_tag_address(addr2);

    /* addr1/addr2가 tracked RAM 범위 밖이면 get_tag_address가 0을 반환한다.
     * 역참조 전에 반드시 확인해야 한다. */
    if (!tag1 || !tag2) {
        return TRUE;
    }

    /* 해제된 메모리 접근 탐지 (UAF).
     * remove_tag()가 freed 영역을 UNPOISON_TAG(0xFD)로 마킹하므로
     * 0xFD가 보이면 해제 후 접근이다. */
    if (*tag1 == UNPOISON_TAG || *tag2 == UNPOISON_TAG) {
        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "UAF detected: from: %p(%u), to: %p(%u)", addr1, *tag1, addr2, *tag2);
        uart_debug_print(buffer);
        #endif
        handle_tag_mismatch(addr1, addr2);
        return FALSE;
    }

    /* 버퍼 경계 초과 탐지 — 두 주소의 태그가 다르면 객체 경계를 넘었다. */
    if (*tag1 != *tag2) {
        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Tags mismatch for addresses:  from: %p(%u), to: %p(%u)", addr1, *tag1, addr2, *tag2);
        uart_debug_print(buffer);
        #endif
        handle_tag_mismatch(addr1, addr2);
        return FALSE;
    }

    #ifdef DEBUG
    snprintf(buffer, sizeof(buffer), "Tags match for addresses:  from: %p, to: %p", addr1, addr2);
    uart_debug_print(buffer);
    #endif
    return TRUE;
}
/* Clear entire tag memory region – called by test runner between test cases */
void tags_reset(void) {
    memset((void *)RAM_END, 0x00, 0x20030000U - RAM_END);
}
