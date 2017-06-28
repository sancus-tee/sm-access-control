#include "../../common.h"
#include "../../benchmark.h"
#include "../sfs/sfs.h"

/********** BENCHMARK PARAMETERS **********/

#ifndef NB_BENCHMARK_FILES
    #define NB_BENCHMARK_FILES            1
#endif

#ifndef MAX_ACL_BENCHMARK_LENGTH
    #define MAX_ACL_BENCHMARK_LENGTH      1
#endif

#ifndef INIT_BENCHMARK_FILE_SIZE
    #define INIT_BENCHMARK_FILE_SIZE      10
#endif

#ifndef MAX_ACL_BENCHMARK_LENGTH
    #define MAX_ACL_BENCHMARK_LENGTH      1
#endif

#define filename_start      'f'
//#define DO_DUMP

/********** UTILITY MACROS **********/

#define EXIT            while (1) {}
#define ASSERT(cond) \
do { \
    if(!(cond)) \
    { \
    puts("assertion failed; exiting..."); \
    EXIT \
    } \
} while(0)

#ifdef DO_DUMP
    #define DUMP        sfs_dump();
#else
    #define DUMP
#endif

#define PRINT_SEC(str)  puts("\n==================== " str " ====================");
#define PRINT_SEP       puts("--------------------");

/********** SM CONFIG **********/

DECLARE_SM(sfsBenchmarkSm, 0x1234);
#define A               "[sfsBenchmarkSm] "
#define printa(str)     printdebug(A str)
#define A_ID            2

DECLARE_SM(sfsBenchmarkHelperSm, 0x1234);
#define B_ID            3

#ifdef RUN_FILES_BENCHMARK

void SM_ENTRY("sfsBenchmarkHelperSm") ping_helper(void)
{
    return;
}

/**
 * NB_BENCHMARK_FILES files, 1 acl entry
 */
void SM_ENTRY("sfsBenchmarkSm") run_files_benchmark(void)
{
    sm_id my_id = sancus_get_self_id();
    printdebug_int(A "Hi from benchmark SM, I have id %d\n", my_id);
    ASSERT(my_id == A_ID);
    ping_helper();
    ASSERT(B_ID == sancus_get_id(sfsBenchmarkHelperSm.public_start));

    PRINT_SEC("PING")
    TSC1()
    sfs_ping();
    TSC2("sfs_ping_1st")
    TSC1()
    sfs_ping();
    TSC2("sfs_ping_2nd")
    
    PRINT_SEC("INIT")
    TSC1()
    sfs_init();
    TSC2("sfs_init_1st")
    DUMP
    
    TSC1()
    sfs_init();
    TSC2("sfs_init_2nd")

    PRINT_SEC("OPEN 1st")
    int fds[NB_BENCHMARK_FILES];
    int i;
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        char name = filename_start + i;
        TSC1()
        fds[i] = sfs_open(name, SFS_CREATOR, INIT_BENCHMARK_FILE_SIZE);
        TSC2("sfs_open_1st")
        sfs_close(fds[i]);
    }
    DUMP
    
    // open in the same order, to get worse case measurements
    PRINT_SEC("OPEN 2nd")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        char name = filename_start + i;
        TSC1()
        fds[i] = sfs_open(name, SFS_ROOT, SFS_OPEN_EXISTING);
        TSC2("sfs_open_2nd")
    }

    PRINT_SEC("SEEK")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    { 
        int fd = fds[i];
        TSC1()
        sfs_seek(fd, 0, SFS_SEEK_SET);
        TSC2("sfs_seek")
    }
    
    PRINT_SEC("GETC")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    { 
        int fd = fds[i];
        TSC1()
        sfs_getc(fd);
        TSC2("sfs_getc")
    }
    
    PRINT_SEC("PUTC")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    { 
        int fd = fds[i];
        TSC1()
        sfs_putc(fd, 'a');
        TSC2("sfs_putc")
    }

    DUMP

    PRINT_SEC("ADD_ACL")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        char name = filename_start + i;
        TSC1()
        sfs_chmod(name, B_ID, SFS_READ);
        TSC2("sfs_chmod (add_acl)")
    }
    DUMP
    
    // revoke_acl is worst case, since B has no open file descriptors
    PRINT_SEC("REVOKE_ACL")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        char name = filename_start + i;
        TSC1()
        sfs_chmod(name, B_ID, SFS_NIL);
        TSC2("sfs_chmod (revoke_acl)")
    }
    DUMP

    PRINT_SEC("ATTEST")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        char name = filename_start + i;
        TSC1()
        sfs_attest(name, sfsBenchmarkSm.id);
        TSC2("sfs_attest")
    }

    PRINT_SEC("CLOSE")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        int fd = fds[i];
        TSC1()
        sfs_close(fd);
        TSC2("sfs_close")
    }

    //acl_size =  1 (because permissions are revoked)
    PRINT_SEC("REMOVE")
    for (i = 0; i < NB_BENCHMARK_FILES; i++)
    {
        char name = filename_start + i;
        TSC1()
        sfs_remove(name);
        TSC2("sfs_remove")
    }
    DUMP
}
#endif // RUN_FILES_BENCHMARK    

#ifdef RUN_ACL_BENCHMARK

void SM_ENTRY("sfsBenchmarkHelperSm") call_b(char filename)
{
    sfs_ping();
    int fd;
    
    TSC1()
    fd = sfs_open(filename, SFS_READ, SFS_OPEN_EXISTING);
    TSC2("sfs_open_from_sm_b")

    sfs_close(fd);

    // double effect: B is last entry -> CHECK_ROOT searches entire ACL
    // then again entire ACL to search existing entry
    TSC1()
    sfs_chmod(filename, B_ID+1, SFS_READ);
    TSC2("sfs_chmod_sm_b")
    DUMP

    TSC1()
    sfs_remove(filename);
    TSC2("sfs_remove_from_sm_b")
}

/**
 * A will now make sure B has to search the ACL till the end for an
 * increasing length (new permissions are appended in acl)
 */
void SM_FUNC("sfsBenchmarkSm") construct_acl_for_b(char filename, int length)
{
    int i;
    
    // construct the ACL with B permission at the end
    for (i = length; i > 1; i--)
    {
        sfs_chmod(filename, B_ID+i, SFS_READ);
    }
    
    // A is first entry -> CHECK_ROOT is quick, but then search entire ACL for
    // existing entry
    TSC1()
    sfs_chmod(filename, B_ID, SFS_ROOT);
    TSC2("sfs_chmod_sm_a")
    DUMP
    
    call_b(filename);
}

/**
 * 1 file; up to MAX_ACL_BENCHMARK_LENGTH ACL measurements
 */
void SM_ENTRY("sfsBenchmarkSm") run_acl_benchmark(void)
{
    sm_id my_id = sancus_get_self_id();
    printdebug_int(A "Hi from benchmark SM, I have id %d\n", my_id);
    ASSERT(my_id == A_ID);
    ASSERT(B_ID == sancus_get_id(sfsBenchmarkHelperSm.public_start));
    
    PRINT_SEC("GROWING ACL")

    int i;
    for (i = 1; i <= MAX_ACL_BENCHMARK_LENGTH; i++)
    {
        int fd = sfs_open(filename_start, SFS_CREATOR, 10);
        sfs_close(fd);
        construct_acl_for_b(filename_start, i);
        // SM b will remove the file
        PRINT_SEP
    }
}

#endif // RUN_ACL_BENCHMARK
