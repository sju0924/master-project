#include "main.h"
#include "application.h"
#include <string.h>

int num1 = 10;
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

void application(){
  test_struct_tagging();
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
  allocation_test1[40]='z';
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);        
  uart_send_string_char(allocation_test1, 10);
  uart_send_string_char(allocation_test2, 10);

  free(allocation_test1);
  free(allocation_test2);

}