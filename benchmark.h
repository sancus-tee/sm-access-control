/**
 * A utility header declaring macros that facilitate accurate measurements
 * of the number of CPU cycles
 *
 * e.g. TSC1()
 *      some_fct()
 *      TSC2("some_fct")
 */
#ifndef BENCHMARK_H

/**
 * @note: make sure tsc_read() is inlined to avoid adding the overhead of exiting
 * the calling SM to enter the tsc_read() function to the measurement
 */
#include <sancus_support/tsc.h>

/**
 * The number of cycles to copy a 64 bit register to a memory location (to be
 * substracted for every measurement)
 * 
 * @note: 32 cyles = 8 bytes * 4 cycles/byte
 */
#define TSC_READ_OVERHEAD       32

/**
 * Global unprotected variables to store the intermediate time stamp counts
 */
extern tsc_t the_ts1, the_ts2, the_diff;

/**
 * An unprotected function to allow printing the with printf
 *
 * @note: this function should be unprotected as printf uses the stack for
 * (variable argument) parameter passing
 */
void __attribute__((noinline)) print_tsc_diff(char *str);


#define TSC1() \
    the_ts1 = tsc_read();

#define TSC2(str) \
    the_ts2 = tsc_read(); \
    the_diff = the_ts2 - the_ts1 - TSC_READ_OVERHEAD; \
    print_tsc_diff(str);

#endif
