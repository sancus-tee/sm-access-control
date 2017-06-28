#include <sancus/sm_support.h>
#include "sfs.h"

DECLARE_SM(sfs, 0x1234);

void SM_ENTRY("sfs") sfs_ping(void)
{
    return;
}

void SM_ENTRY("sfs") sfs_init(void)
{
    return;
}

void SM_ENTRY("sfs") sfs_dump(void)
{
    return;
}

int SM_ENTRY("sfs") sfs_creat(filename_t name)
{
    return 0;
}

int SM_ENTRY("sfs") sfs_open(filename_t name, int flags, int size)
{
    return 0;
}

int SM_ENTRY("sfs") sfs_close(int fd)
{
    return 0;
}

int SM_ENTRY("sfs") sfs_getc(int fd)
{
    return 1;
}

int SM_ENTRY("sfs") sfs_putc(int fd, unsigned char c)
{
    return 1;
}

int SM_ENTRY("sfs") sfs_seek(int fd, int offset, int whence)
{
    return 0;
}

int SM_ENTRY("sfs") sfs_remove(filename_t name)
{
    return 0;
}

int SM_ENTRY("sfs") sfs_chmod(filename_t name, sm_id id, int perm_flags)
{
    return 0;
}

int SM_ENTRY("sfs") sfs_attest(filename_t name, sm_id owner)
{
    return 0;
}
