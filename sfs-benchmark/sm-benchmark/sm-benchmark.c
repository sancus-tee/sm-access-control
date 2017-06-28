#include "../common.h"
#include "../benchmark.h"
#include <stdint.h>

/********** UTILITY MACROS **********/

#define EXIT            while (1) {}
#define PRINT_SEC(str)  puts("\n==================== " str " ====================");

/********** SM CONFIG **********/

DECLARE_SM(clientA, 0x1234);
#define A               "[clientA] "
#define printa(str)     printdebug(A str)
#define A_ID            1

DECLARE_SM(clientB, 0x1234);
#define B_ID            2

/* =========================== TSC BENCHMARK ====================================
 *
 * to measure and verify the fixed overhead to be substracted for every measurement
 */

void run_tsc_benchmark(void)
{
    PRINT_SEC("tsc_read() overhead verification");
    
    printf("TSC_READ_OVERHEAD is defined as %d\n", TSC_READ_OVERHEAD);
    
    // manually inline tsc_read()
    TSC_CTL = 1;
    the_ts1 = TSC_VAL;
    TSC_CTL = 1;
    the_ts2 = TSC_VAL;
    the_diff = the_ts2 - the_ts1;
    print_tsc_diff("ts1 = TSC_VAL [manually inlined]");
    if (the_diff != TSC_READ_OVERHEAD)
        puts("WARNING: TSC_READ_OVERHEAD seems not to be correctly defined");
    
    // verify tsc_read fct is inlined
    the_ts1 = tsc_read();
    the_ts2 = tsc_read();
    the_diff = the_ts2 - the_ts1;
    print_tsc_diff("tsc_read() [should be inlined]");
    if (the_diff != TSC_READ_OVERHEAD)
        puts("WARNING: tsc_read() seems not to be inlined");
    
    // verify provided macros cancel overhead
    TSC1()
    TSC2("TSC macro call [should be zero]")
    if (the_diff != 0)
        puts("WARNING: TSC macros seem not to cancel tsc_read() fixed overhead");
    
    tsc_t some_val = UINT64_MAX;
    tsc_t *val_ptr = &some_val;
    tsc_t **val_ptr_ptr = &val_ptr;
    TSC1()
    tsc_t i = **val_ptr_ptr;
    TSC2("a_64_bit_int_assignment_from_ptr")
}

/* =========================== FUNCTION CALL BENCHMARK ==========================
 *
 * to measure the overhead of calling a simple void function with or without
 * switching protection domains
 */

void an_unprotected_fct(void)
{
    return;
}

void SM_FUNC("clientA") an_sm_func(void)
{
    return;
}

void SM_ENTRY("clientB") an_sm_entry_b(void)
{
    return;
}


void SM_ENTRY("clientA") an_sm_entry(void)
{
    return;
}

void SM_ENTRY("clientA") call_unprotected(void)
{
    TSC1()
    an_unprotected_fct();
    TSC2("protected_to_unprotected");
}

void SM_ENTRY("clientA") wrapper_call_unprotected(void)
{
    an_unprotected_fct();
}

void SM_ENTRY("clientA") call_sm_func(void)
{
    TSC1()
    an_sm_func();
    TSC2("protected_to_sm_func");
}

void SM_ENTRY("clientA") wrapper_call_protected(void)
{
    an_sm_func();
}

void SM_ENTRY("clientA") call_sm_entry_b(void)
{
    an_sm_entry_b(); // ping
    TSC1()
    an_sm_entry_b();
    TSC2("protected_to_sm_entry_b");
}

void SM_ENTRY("clientB") do_wrapper_calls(void)
{
    an_sm_entry(); // ping
    
    TSC1()
    wrapper_call_unprotected();
    TSC2("protected_to_wrapper_call_unprotected");
    
    TSC1()
    wrapper_call_protected();
    TSC2("protected_to_wrapper_call_protected");
}

void run_fct_call_benchmark(void)
{
    PRINT_SEC("function call benchmark");
    
    TSC1()
    an_unprotected_fct();
    TSC2("unprotected_to_unprotected");
    
    TSC1()
    an_sm_entry();
    TSC2("unprotected_to_protected");
    
    call_sm_func();  
    call_sm_entry_b();
    call_unprotected();
    
    TSC1()
    wrapper_call_unprotected();
    TSC2("unprotected_to_wrapper_call_unprotected");
    
    TSC1()
    wrapper_call_protected();
    TSC2("unprotected_to_wrapper_call_protected");
    
    do_wrapper_calls();
}

int main()
{
    WDTCTL = WDTPW | WDTHOLD;
    uart_init();
    puts("\n---------------\nmain started");
    
    sancus_enable(&clientA);
    sancus_enable(&clientB);
    
    run_tsc_benchmark();
    run_fct_call_benchmark();
    
    puts("\n[main] exiting\n-----------------");
    EXIT
}

void __attribute__((interrupt(SM_VECTOR))) the_isr(void) {
    puts("\nVIOLATION HAS BEEN DETECTED: stopping the system");
    while (1) {}
}
