#define malloc(size) my_malloc(size)
#define free(ptr) my_free(ptr)

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "runtimeConfig.h"
#include "tagManager.h"

// poison_queue 최대 크기 설정
const size_t POISON_QUEUE_MAX_SIZE = 3;

// 지연 해제 대기열을 위한 전역 deque
static HeapMetadata* poison_queue[3] = {NULL, NULL, NULL};
static uint8_t poison_queue_index = 0;
