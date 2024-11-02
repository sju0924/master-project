#define SHADOW_MEM_START  0x2002A000      // 쉐도우 메모리 시작 주소
#define MAIN_RAM_START    0x20000000      // 메인 RAM 시작 주소

#include "tagManager.h"

uint8_t prev_tag = 0;

uint8_t tag_generator(){
    srand(time(NULL));
    uint8_t tag = prev_tag;
    while(tag == prev_tag || tag == 0x00 || tag == POISON_TAG){
        tag = rand() % 0x100;
    }

    return tag;
}
// 주어진 RAM 주소에 해당하는 태그 테이블 주소를 반환
uint8_t* get_tag_address(uint32_t address) {
    uint32_t offset = (address - MAIN_RAM_START) / 8;
    return (uint8_t *)(SHADOW_MEM_START + offset);
}

void set_tag(void *address, size_t size) {
    uint8_t *tag_address = get_tag_address((uintptr_t)address);
    uint8_t tag = tag_generator();

    // 8바이트 단위로 태그 설정
    for (size_t i = 0; i < size / 8; i++) {
        *tag_address++ = tag;
    }

    // 남은 바이트가 있을 경우 마지막 태그 설정
    if (size % 8 != 0) {
        *tag_address = tag;
    }
}

uint8_t get_tag(void *address) {
    uint8_t *tag_address = get_tag_address((uint32_t)address);
    return *tag_address;
}

// 두 주소의 태그를 비교하는 함수
bool compare_tag(void* addr1, void* addr2) {
    // 각 주소의 태그 가져오기
    uint8_t tag1 = get_tag_address(addr1);
    uint8_t tag2 = get_tag_address(addr2);

    // 태그 값 비교
    if (tag1 == tag2) {
        std::cout << "Tags match for addresses: " << addr1 << " and " << addr2 << std::endl;
        return true;
    } else {
        std::cout << "Tag mismatch! Address " << addr1 << " has tag " << (int)tag1
                  << ", Address " << addr2 << " has tag " << (int)tag2 << std::endl;
        return false; // Todo: mismatch 시 오류 처리할 핸들러 생성
    }
}
/*

Todo:
    1. heap / Global variable 할당시 태그 부여(완료)
    2. Stack 할당시 태그부여(완료)
    3. Intra-struct 태그 부여

*/