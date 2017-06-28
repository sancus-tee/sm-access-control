#include "common.h"

#ifndef NSANCUS_COMPILE
int __attribute__((noinline)) putchar(int c)
{
    if (c == '\n')
        putchar('\r');

    uart_write_byte(c);
    return c;
}
#endif

long public_long;

void __attribute__((noinline)) printf_int(const char* fmt, unsigned int i) {
    printf(fmt, i);
}

void __attribute__((noinline)) printf_int_int(const char* fmt, unsigned int i, unsigned int j) {
    printf(fmt, i, j);
}


void __attribute__((noinline)) printf_long(const char* fmt, unsigned long i) {
    printf(fmt, i);
}

void __attribute__((noinline)) printf_str(const char* string) {
    printf("%s", string);
}

void __attribute__((noinline)) printf_char(const char c) {
    printf("%c", c);
}

void __attribute__((noinline)) print_buf(const char* msg, uint8_t* buf, size_t len)
{
    printf("%s: ", msg);

    for (size_t i = 0; i < len; i++)
        printf("%02x ", buf[i]);

    printf("\n");
}
