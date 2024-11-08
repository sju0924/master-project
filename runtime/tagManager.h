#define UNPOISON_TAG 0xFD
#define POISON_TAG 0xFF
#define RAM_START 0x20000000
#define RAM_END 0x2002A000
#define MAX_STRUCT 10

// LLVM 패스가 생성한 전역 배열 (2차원 배열)
extern "C" size_t struct_member_offsets[][MAX_STRUCT];  // 구조체마다 최대 10개의 멤버
extern "C" size_t struct_member_sizes[][MAX_STRUCT];    // 구조체마다 최대 10개의 멤버
extern "C" size_t struct_member_counts[];       // 각 구조체의 멤버 수

#include "runtimeConfig.h"

uint8_t tag_generator();
uint8_t* get_tag_address(void *address);
void set_tag(void *address, size_t size);
void set_tag_padding(void *address, size_t size) ;
uint8_t get_tag(void *address);

