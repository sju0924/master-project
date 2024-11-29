#include "main.h"
#include "application.h"
#include "testcase_without_pass.h"
#include <string.h>
#include <stdio.h>

struct test_struct{
  int a;
  char b[6];
  long long int c;
};

struct spare_struct{
  int a;
  char b[10];
  long long int c;
};


int num1 = 10;
char log_buffer[512];
int buffer[10];

void application(){
  uint32_t start_tick, end_tick, elapsed_ticks, timer_clock;
  float resolution, elapsed_time_us;

  //StartTimer(); 
  //resolution = CalculateTimerResolution();

  // Pass 적용 시간 측정
  start_tick = HAL_GetTick();

  for(int i = 0 ; i<100 ; i++){
    //CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good();
    CWE476_NULL_Pointer_Dereference__char_01_good();
  }
  
  end_tick = HAL_GetTick();


  elapsed_ticks = end_tick - start_tick;

  // Pass 적용 시간 출력
  sprintf(log_buffer, "Execution Time Without Pass: %u ms, \r\n", elapsed_ticks);
  uart_send_string(log_buffer);
    sprintf(log_buffer, "start tick: %u, end tick: %u\r\n",  start_tick, end_tick);
  uart_send_string(log_buffer);


  // 보호x인 테스트케이스 위해 MPU 해제
  HAL_MPU_Disable();
  // Pass 적용 전 시간 측정
  start_tick = HAL_GetTick();

  for(int i = 0 ; i<100 ; i++){
    //CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_good_without_pass();
    CWE476_NULL_Pointer_Dereference__char_01_good_without_pass();
  }
  
  end_tick = HAL_GetTick();


  elapsed_ticks = end_tick - start_tick;

  // Pass 적용 전 출력
  sprintf(log_buffer, "Execution Time With Pass: %u ms, \r\n", elapsed_ticks);
  uart_send_string(log_buffer);
    sprintf(log_buffer, "start tick: %u, end tick: %u\r\n",  start_tick, end_tick);
  uart_send_string(log_buffer);

  const char* msg1 = "Good passed\r\n";
  uart_send_string(msg1);

  // bad testcase를 위한 보호 설정
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
  //CWE121_Stack_Based_Buffer_Overflow__src_char_declare_cpy_01_bad();
  CWE476_NULL_Pointer_Dereference__char_01_bad();
}
void test_struct_tagging(){
  char not_struct[11]={'N','O','T',' ','S','T','R','U','C','T','\n'};
 
  struct test_struct ts={
    1
    ,{'A','B','C','D','E','F'}
    , 2
  };
  struct spare_struct ss={
    1
    ,{'S','P','R',' ','S','T','R','U','C','\n'}
    , 2
  };

  ss.b[13] = 'F';


  ts.b[3] = 'Z';
  
 uart_send_string_char(ts.b, 6);
 uart_send_string_char(ss.b, 10);
 uart_send_string_char(not_struct, 11);
}

void test_uart_print(){
    const char* msg1 = "Going on...\n";
    const char* msg2 = "Going off...\n";
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
  allocation_test1[40]='z';
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
    *(ptr++) = 'A';
    // 기본 오류 정보 작성
    snprintf(log_buffer, sizeof(log_buffer),
             "Buffer overflow test: Current pc:%p\r\n",
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
             "Global variable underflow test: Current pc:0x%p\r\n",
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
             "Global variable overflow test: Current pc:0x%p\r\n",
             buffer + index);
    uart_send_string(log_buffer);
    index++;
  }
}

void test_nullptr_derefence(){
    char * data;
    data = NULL; /* Initialize data */
    /* POTENTIAL FLAW: Allocate memory without checking if the memory allocation function failed */
    strcpy(data, "initialize\r\n\0");
    data[0] = 'I';
    uart_send_string_char(data,20);
    
    /* FLAW: Initialize memory buffer without checking to see if the memory allocation function failed */

    free(data);
  
}

void test_UAF(){
    char log_buffer[100];
    int *buffer = (int*)malloc(sizeof(int));  // 동적 메모리 할당
    *buffer = 42;  // 초기화

    snprintf(log_buffer, sizeof(log_buffer),
             "Allocated memory value: %d at address: 0x%p\r\n", *buffer, (void*)buffer);
    uart_send_string(log_buffer);

    free(buffer);  // 메모리 해제
    *buffer = 42;  // 초기화
    // 해제된 메모리에 접근 (UAF 오류 발생)
    snprintf(log_buffer, sizeof(log_buffer),
             "After free: Attempting to access memory at address: 0x%p, value: %d\r\n",
             (void*)buffer, *buffer);  // *buffer는 UAF 오류를 유발
    uart_send_string(log_buffer);

}