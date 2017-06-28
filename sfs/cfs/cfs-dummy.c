/*
 * a dummy implementation of the CFS interface to be able to have back-end
 * independent measurements of the SFS front-end.
 *
 * TODO inline or macros for performance?
 *
 * \author Jo Van Bulck
 *
 */

#include "cfs.h"

#ifdef CFS_BACKEND_PROTECTED
    #include <sancus/sm_support.h>
    extern struct SancusModule sfs;
    #define SM_F(str)   SM_FUNC(str)
#else
    #define SM_F(str)
#endif

int fd_count = 0;

int SM_F("sfs") cfs_open(const char *name, int flags, unsigned int size)
{
    return fd_count++;
}

void SM_F("sfs") cfs_close(int fd)
{
    return;
}

int SM_F("sfs") cfs_read(int fd, void *buf, unsigned int len)
{
    return 0;
}

int SM_F("sfs") cfs_write(int fd, const void *buf, unsigned int len)
{
    return 0;
}

cfs_offset_t SM_F("sfs") cfs_seek(int fd, cfs_offset_t offset, int whence)
{
    return 0;
}

int SM_F("sfs") cfs_remove(const char *name)
{
    return 0;
}

int SM_F("sfs") cfs_opendir(struct cfs_dir *dirp, const char *name)
{
    return 0;
}

int SM_F("sfs") cfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent)
{
    return 0;
}

void SM_F("sfs") cfs_closedir(struct cfs_dir *dirp)
{
    return;
}

int SM_F("sfs") cfs_format(void)
{
    // restart assigning file descriptors from zero
    fd_count = 0;
    return 0;
}

void SM_F("sfs") cfs_ping(void)
{
    return;
}


