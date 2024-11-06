#include "main.h"
#include "application.h"
#include <string.h>
#include <stdio.h>

int num1 = 10;
char log_buffer[512];
int buffer[10];

void application(){
  test_gv_underflow();
  test_gv_overflow();
}

void test_uart_print(){
    const char* msg1 = "Going on...";
    const char* msg2 = "Going off...";
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);        
    uart_send_string(msg1);
    HAL_Delay(500);  // 500ms 대기


    // LED Off
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    uart_send_string(msg2);
    HAL_Delay(500);  // 500ms 대기
}

void test_heap_allocation(){
num1 = 100;
  char *allocation_test1 = (char *)malloc(sizeof(char)*10);
  char *allocation_test2 = (char *)malloc(sizeof(char)*10);
  char j = 'A';
  for (int i = 0 ; i<9; i++){
    allocation_test1[i] = j;
    j++;
  }
  allocation_test1[9]='\0';

  for (int i = 0 ; i<9; i++){
    allocation_test2[i] = j;
    j++;
  }
  allocation_test2[9]='\0';


  allocation_test1[3]='Z';
  allocation_test2[4]='Z';
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);        
  uart_send_string_char(allocation_test1, 10);
  uart_send_string_char(allocation_test2, 10);

  free(allocation_test1);
  free(allocation_test2);

}


void test_buffer_underflow(){
  char arr[10];
  int index = 0;

  arr[0] = 1;
  while(1){    
    arr[index--] = 1;
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Buffer underflow test: Current pc:0x%p\r\n",
             arr + index);
    uart_send_string(log_buffer);
  }
}

void test_buffer_overflow(){
  char arr[10];
  char *ptr = arr;
  

  while(1){    
    *ptr++ = 'A';
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Buffer overflow test: Current pc:0x%p\r\n",
             ptr);
    uart_send_string(log_buffer);
  }
}


void test_heap_underflow(){
  int index = 0;
  char *arr = (char *)malloc(10*sizeof(char));
  arr[0] = 1;
  while(1){    
    arr[index] = 1;
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Heap underflow test: Current pc:0x%p\r\n",
             arr + index);
    uart_send_string(log_buffer);
    index--;
  }
}

void test_heap_overflow(){
  int index = 0;
  char *arr = (char *)malloc(10*sizeof(char))  ;
  arr[0] = 1;
  while(1){    
    arr[index] = 1;
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Heap overflow test: Current pc:0x%p\r\n",
             arr + index);
    uart_send_string(log_buffer);
    index++;
  }
}


void test_gv_underflow(){
  int index = 0;
  
  while(1){    
    buffer[index] = 1;
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Global variable test: Current pc:0x%p\r\n",
             buffer + index);
    uart_send_string(log_buffer);
    index--;
  }
}

void test_gv_overflow(){
  int index = 0;
  
  while(1){    
    buffer[index] = 1;
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Global variable underflow test: Current pc:0x%p\r\n",
             buffer + index);
    uart_send_string(log_buffer);
    index++;
  }
}