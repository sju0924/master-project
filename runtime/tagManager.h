#define UNPOISON_TAG 0xFD
#define POISON_TAG 0xFF
#define RAM_START 0x20000000
#define RAM_END 0x2002A000
#define MAX_STRUCT 10

#include "runtimeConfig.h"

// LLVM 패스가 생성한 구조체 멤버 메타데이터.
// offsets/sizes는 모든 구조체 멤버를 순서대로 이어 붙인 flat 배열이고,
// counts는 각 구조체별 멤버 개수다.
extern uint32_t struct_member_offsets[];
extern uint32_t struct_member_sizes[];
extern uint32_t struct_member_counts[];

uint8_t tag_generator();
uint8_t* get_tag_address(void *address);
void set_tag(void *address, size_t size);
void set_tag_padding(void *address, size_t size);
void set_struct_tags(void *struct_address, uint32_t item_index);
void remove_tag(void *address, size_t size);
uint8_t get_tag(void *address);
void check_live_tag(void *address);
void check_null_ptr(void *address);
void tags_reset(void);
