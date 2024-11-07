#define UNPOISON_TAG 0xFD
#define RAM_START 0x20000000
#define RAM_END 0x2002A000
#include "runtimeConfig.h"

uint8_t tag_generator();
uint8_t* get_tag_address(void *address);
void set_tag(void *address, size_t tag);
uint8_t get_tag(void *address);
