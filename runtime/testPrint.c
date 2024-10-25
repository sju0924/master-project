void uart_send_string(const char *str);

void test_print(const char *func_name) {
    uart_send_string("Function called: ");
    uart_send_string(func_name);
    uart_send_string("\r\n");
}
