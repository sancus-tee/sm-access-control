#include "../common.h"
#include "../benchmark.h"
#include <stdint.h>
#include "my_malloc.h"

/********** UTILITY MACROS **********/

#define EXIT            while (1) {}
#define PRINT_SEC(str)  puts("\n==================== " str " ====================");

#define ASSERT(cond) \
do { \
    if(!(cond)) \
    { \
    puts("assertion failed; exiting..."); \
    EXIT \
    } \
} while(0)

/********** SM CONFIG **********/

DECLARE_SM(clientA, 0x1234);
#define A               "[clientA] "
#define printa(str)     printdebug(A str)
#define A_ID            1

void SM_ENTRY("clientA") run_malloc_benchmark(void)
{
    printf_int_int("Hi from A, I was called by %d and I have id %d\n",
    sancus_get_caller_id(), sancus_get_self_id());
    
    PRINT_SEC("INIT_FREE_LIST")
    
    TSC1()
    init_free_list();
    TSC2("init_free_list")
    
    PRINT_SEC("MY_MALLOC")
    int i; char *ptrs[5];
    for (i = 0; i < 5; i++)
    {
        TSC1()
        ptrs[i] = (char*) my_malloc(100);
        TSC2("my_malloc_100");
        ASSERT(ptrs[i]);
        printf_int(A "got ptr 0x%x\n", (unsigned int) ptrs[i]);
    }
    
    PRINT_SEC("PUTC")
    for (i = 0; i < 5; i++)
    {
        char c = 'a' + i;
        TSC1()
        *(ptrs[0] + i) = c;
        TSC2("putc_idx");
    }
    
    PRINT_SEC("GETC")
    for (i = 0; i < 5; i++)
    {
        char c;
        TSC1()
        c = *(ptrs[0] + i);
        TSC2("putc_idx_cst");
        printf_int("got char '%c'\n", c);
    }

    PRINT_SEC("MY_FREE")    
    for (i = 0; i < 5; i++)
    {
        TSC1()
        my_free(ptrs[i]);
        TSC2("my_free");
    }
    
    puts(A "now exiting");
}


int main()
{
    WDTCTL = WDTPW | WDTHOLD;
    uart_init();
    puts("\n---------------\nmain started");
    
    sancus_enable(&clientA);
    
    run_malloc_benchmark();
    
    puts("\n[main] exiting\n-----------------");
    EXIT
}

void __attribute__((interrupt(SM_VECTOR))) the_isr(void) {
    puts("\nVIOLATION HAS BEEN DETECTED: stopping the system");
    while (1) {}
}
