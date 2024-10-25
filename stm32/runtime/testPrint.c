#include <stdint.h>
#include "cmsis_gcc.h"
#include "stm32l5xx_hal.h"
#include "system_stm32l5xx.h"
#include "stm32l5xx_hal_dma.h"
#include "stm32l5xx_hal_uart.h"  // UART 모듈 헤더 파일
#include "main.h"

void runtime_func(const char *func_name) {
    uart_send_string("Function called: ");
    uart_send_string(func_name);
    uart_send_string("\n");
}
