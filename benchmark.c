#include "benchmark.h"
#include <stdio.h>

tsc_t the_ts1, the_ts2, the_diff;

void __attribute__((noinline)) print_tsc_diff(char *str)
{
    printf("number of cycles for %s is %llu\n", str, the_diff);
}
