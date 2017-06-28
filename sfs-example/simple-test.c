#include "../common.h"          // utility functions
#include <sancus/sm_support.h>  // Sancus support
#include "../sfs/sfs.h"         // Sancus File System

#define THE_FILENAME        'a'
#define THE_INIT_FILESIZE   100
#define THE_TEST_DATA       "Lorem ipsum dolor sit amet, consectetur adipiscing elit."

DECLARE_SM(clientB, 0x1234);
#define B   "[clientB] "

void SM_ENTRY("clientB") run_clientB(void)
{
    printf_int_int(B "Hi, I have id %d and was called from %d\n",
        sancus_get_self_id(), sancus_get_caller_id());

    int fd, i = 0; char cur;
    
    puts(B "attempting to open the file and read the data");
    fd = sfs_open(THE_FILENAME, SFS_READ, SFS_OPEN_EXISTING);
    if (fd < 0)
    {
        puts(B "access to the file has been denied");
        return;
    }

    printf_str(B "the chars are: \"");
    while((cur = sfs_getc(fd)) != EOF)
        printf_int("%c", cur);
    printf_str("\"\n");
    
    sfs_close(fd);
}

DECLARE_SM(clientA, 0x1234);
#define A   "[clientA] "

void SM_ENTRY("clientA") run_clientA(void)
{
    printf_int_int(A "Hi, I have id %d and was called from %d\n",
        sancus_get_self_id(), sancus_get_caller_id());
        
    int fd, i; char *str = THE_TEST_DATA;
    sm_id idB = sancus_get_id(clientB.public_start);

    puts(A "creating the file and writing the data");
    fd = sfs_open(THE_FILENAME, SFS_CREATOR, THE_INIT_FILESIZE);
    for(i = 0; str[i]; i++)
        sfs_putc(fd, str[i]);
    
    puts(A "assigning read-only permissions for clientB and dumping ACLs");
    sfs_chmod(THE_FILENAME, idB, SFS_READ);
    sfs_dump();
    
    run_clientB();

    puts(A "revoking permissions for clientB and dumping ACLs");    
    sfs_chmod(THE_FILENAME, idB, SFS_NIL);
    sfs_dump();
 
    run_clientB();
    
    puts(A "closing and removing the logical file");
    sfs_close(fd);
    sfs_remove(THE_FILENAME);
}

int main()
{
    WDTCTL = WDTPW | WDTHOLD;
    uart_init();
    puts("\n---------------\nmain started");
    
    sancus_enable(&sfs);
    sancus_enable(&clientA);
    sancus_enable(&clientB);    

    run_clientA();
    
    puts("[main] exiting\n-----------------");
    while(1) {}
}
