#define malloc(size) my_malloc(size)
#define free(ptr) my_free(ptr)

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "runtimeConfig.h"
#include "tagManager.h"



// 지연 해제 대기열을 위한 전역 deque
static HeapMetadata* poison_queue[3] = {NULL, NULL, NULL};

