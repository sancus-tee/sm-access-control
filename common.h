#ifndef COMMON_H
#define COMMON_H

#ifndef NSANCUS_COMPILE
    #include <sancus/sm_support.h>
    #include <sancus_support/uart.h>
    #include <msp430.h>
    #include <sancus_support/tsc.h>
#endif // NSANCUS_COMPILE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define SUCCESS         1       // a positive value, to indicate success
#define FAILURE         EOF     // a negative value, to indicate failure

int __attribute__((noinline)) putchar(int c);

void __attribute__((noinline)) printf_int(const char* fmt, unsigned int i);
void __attribute__((noinline)) printf_int_int(const char* fmt, unsigned int i, unsigned int j);
void __attribute__((noinline)) printf_long(const char* fmt, unsigned long i);
void __attribute__((noinline)) printf_str(const char* string);
void __attribute__((noinline)) printf_char(const char);

void __attribute__((noinline)) print_buf(const char* msg, uint8_t* buf, size_t len);

#ifndef NOCOLOR
#define BLACK       "\033[1;30m"
#define RED         "\033[1;31m"
#define GREEN       "\033[0;32m"
#define YELLOW      "\033[0;33m"
#define BLUE        "\033[0;34m"
#define MAGENTA     "\033[0;35m"
#define CYAN        "\033[0;36m"
#define WHITE       "\033[0;37m"
#define BOLD        "\033[1m"
#define NONE        "\033[0m"       // to flush the previous color property
#else
#define BLACK       ""
#define RED         ""
#define GREEN       ""
#define YELLOW      ""
#define BLUE        ""
#define MAGENTA     ""
#define CYAN        ""
#define WHITE       ""
#define BOLD        ""
#define NONE        ""
#endif

extern long public_long;
#ifndef NODEBUG
#define printdebug_int(string, i) \
    do { \
        printf_int(string, (intptr_t) i); \
    } while(0)

#define printdebug_int_int(string, i, j) \
    do { \
        printf_int_int(string, (intptr_t) i, (intptr_t) j); \
    } while(0)


#define printdebug_long(string, long) \
    do { \
        public_long = long; \
        printf_long(string, public_long); \
    } while(0)

#define printdebug(string) \
    do { \
        puts(string); \
    } while (0)

#define printdebug_str(string) \
    do { \
        printf_str(string); \
    } while (0)

#define printdebug_char(c) \
    do { \
        printf_char(c); \
    } while (0)

#define printf_cyan(str) \
    do { \
        printf_str(CYAN); \
        printf_str(str); \
        printf_str(NONE); \
    } while(0)

#define printdebug_buf(buf, len) \
do { \
    for (size_t i = 0; i < len; i++) \
        putchar(buf[i]); \
} while(0) 

#else //NODEBUG

#define printdebug_int(string, integer)
#define printdebug_long(string, long)
#define printdebug(string)
#define printf_cyan(str)
#define printdebug_str(str)
#define printdebug_char(c)
#define printdebug_buf(buf, len)

#endif

#define printerr(string) \
    do { \
        printf_str(RED); \
        puts(string); \
        printf_str(NONE); \
    } while(0)

#define printerr_int(string, i) \
    do { \
        printf_str(RED); \
        printf_int(string, (intptr_t) i); \
        printf_str(NONE); \
    } while(0)

#define printerr_int_int(string, i, j) \
    do { \
        printf_str(RED); \
        printf_int_int(string, (intptr_t) i, (intptr_t) j); \
        printf_str(NONE); \
    } while(0)

#endif // COMMON_H
