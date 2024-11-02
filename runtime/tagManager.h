#define POISON_TAG 0xFF
#include "runtimeConfig.h"

uint8_t tag_generator();
uint8_t* get_tag_address(void *address);
void set_tag(void *address, size_t tag);
uint8_t get_tag(void *address);
