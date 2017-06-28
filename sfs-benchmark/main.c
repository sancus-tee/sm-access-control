#include "../../common.h"
#include "../sfs/sfs.h"
#include "sfs-benchmark.h"

int main()
{
    WDTCTL = WDTPW | WDTHOLD;
    uart_init();
    puts("\n---------------\nmain started");
    printdebug_int("[main] I have id %d\n", sancus_get_self_id());
    
    sancus_enable(&sfs);
    sancus_enable(&sfsBenchmarkSm);
    sancus_enable(&sfsBenchmarkHelperSm);
    
#ifdef RUN_FILES_BENCHMARK    
    run_files_benchmark();
#endif

#ifdef RUN_ACL_BENCHMARK
    run_acl_benchmark();
#endif

    sfs_ping();
    
    puts("[main] exiting\n-----------------");
    while (1) {}
}

void __attribute__((interrupt(SM_VECTOR))) the_isr(void) {
    puts("\nVIOLATION HAS BEEN DETECTED: stopping the system");
    while (1) {}
}
