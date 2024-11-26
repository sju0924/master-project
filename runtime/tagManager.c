#include "tagManager.h"

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
    if(tag_address = get_tag_address(address)){
        uint8_t tag = tag_generator();
        
        // 8바이트 단위로 태그 설정
        for (size_t i = 0; i < size / 8; i++) {
            *tag_address++ = tag;
        }

        // 남은 바이트가 있을 경우 마지막 태그 설정
        if (size % 8 != 0) {
            *tag_address = tag;
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
    if(tag_address = get_tag_address(address)){
        uint8_t tag = UNPOISON_TAG;
        
        // 8바이트 단위로 태그 설정
        for (size_t i = 0; i < size / 8; i++) {
            *tag_address++ = tag;
        }

        // 남은 바이트가 있을 경우 마지막 태그 설정
        if (size % 8 != 0) {
            *tag_address = tag;
        }

        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Tag assigned at: %p, size: %d tag: %u", address, size, tag);
        uart_debug_print(buffer);
        #endif
    }   

    
}

void set_tag_padding(void *address, size_t size) {
    uint8_t *tag_address;
    if(tag_address = get_tag_address(address)){
        uint8_t padding_tag = 0x00;
        
        // 8바이트 단위로 태그 설정
        for (size_t i = 0; i < size / 8; i++) {
            *tag_address++ = padding_tag;
        }

        // 남은 바이트가 있을 경우 마지막 태그 설정
        if (size % 8 != 0) {
            *tag_address = padding_tag;
        }
    }
}

// 8바이트 경계마다 태그를 다르게 설정하는 함수
void set_struct_tags(void *struct_address, uint32_t item_index) {
    char buffer[100];
    uint32_t index = 0;
    // 인덱스 불러오기
    for(int i = 0 ; i<item_index ; i++){
        index += struct_member_counts[i];
    }

    // 구조체 메타데이터 불러오기
    uint32_t* member_offsets = (uint32_t*)(struct_member_offsets + index);
    uint32_t* member_sizes = (uint32_t*)(struct_member_sizes + index);
    uint32_t num_members = struct_member_counts[index];

    // 분석에 필요한 멤버 변수 설정
    uintptr_t base_address = (uintptr_t)struct_address;
    uintptr_t last_tagged_address = base_address; 

    uint32_t current_address  = 0;
    uint8_t current_tag = 0;
    uint8_t *tag_address;


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

        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Member address: %d, member size: %zu\n", member_address, member_size);
        uart_debug_print(buffer);
        #endif

        if (member_address < RAM_START || member_address>= RAM_END) {
            continue; // 유효하지 않은 태그 주소는 무시
        }

        // 만약 객체의 끝 부분이 8바이트로 나누어 떨어질 시 태그 부여
        if(member_offsets[i] % 8 == 0 ){
            current_address = last_tagged_address;
            current_tag = tag_generator();

            // 이전 객체에 대한 태그 설정
            while(current_address <= member_address){
                tag_address = get_tag_address((void*)current_address);
                *tag_address = current_tag;
                current_address += 8;
            }

            last_tagged_address = current_address;

            // 디버그 출력 (각 멤버별 태그 정보)
            #ifdef DEBUG
            snprintf(buffer, sizeof(buffer), "Tag assigned: %u\n",  current_tag);
            uart_debug_print(buffer);
            #endif
        }

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

// 두 주소의 태그를 비교하는 함수
uint8_t compare_tag(void* addr1, void* addr2) {
    
    char buffer[100];

    // 각 주소의 태그 가져오기
    uint8_t* tag1 = get_tag_address(addr1);
    uint8_t* tag2 = get_tag_address(addr2);

    // 태그 값 비교
    if (*tag1 == *tag2) {
        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Tags match for addresses:  from: %p, to: %p", addr1, addr2);
        uart_debug_print(buffer);
        #endif
        return TRUE;
    } else {
        #ifdef DEBUG
        snprintf(buffer, sizeof(buffer), "Tags mismatch for addresses:  from: %p(%u), to: %p(%u)", addr1, *tag1, addr2, *tag2);
        uart_debug_print(buffer);
        #endif
        handle_tag_mismatch(addr1, addr2);
        return FALSE; // Todo: mismatch 시 오류 처리할 핸들러 생성
    }
}