/**
 * An implementation of the CFS interface that realizes protected shared memory
 * through a malloc implementation on top of a fixed sized Sancus-protected buffer.
 */

#include "../sfs-debug.h"
#include "../cfs/cfs.h"
#include "my_malloc.h"

#ifndef SUCCESS
    #define SUCCESS         1       // a positive value, to indicate success
#endif
#ifndef EOF
    #define EOF             -1
#endif
#ifndef FAILURE
    #define FAILURE         -1     // a negative value, to indicate failure
#endif

// ############################### PARAMETERS ################################

// the system-wide max number of open files; defines open-file-cache size
#define MAX_NB_OPEN_FILES       8

// ############################ DATA STRUCTURES ##############################

struct open_shm_entry {
    struct shm_entry *shm;          // 2 bytes
    cfs_offset_t offset;            // 2 bytes
};

struct shm_entry {
    char name;                      // TODO 2 bytes?
    unsigned char *malloc_ptr;      // 2 bytes
    unsigned int size;              // 2 bytes
    struct shm_entry *next;         // 2 bytes
};

struct shm_entry SM_D("sfs") *shm_list;
struct open_shm_entry SM_D("sfs") *open_fd_cache[MAX_NB_OPEN_FILES];

// ############################ HELPER FUNCTIONS   ##############################

#define CHK_MALLOC(ptr) \
    if (!ptr) \
    { \
        printerror("running out of malloc space; returning..."); \
        return FAILURE; \
    }

#define CHK_FD(fd) \
    if (fd < 0 || fd >= MAX_NB_OPEN_FILES || !open_fd_cache[fd]) \
    { \
        printerror_int("provided shm back-end fd %d invalid; returning...", fd); \
        return FAILURE; \
    }

int SM_F("sfs") open_existing_file(struct shm_entry *shm)
{
    // locate a free file descriptor
    int i;
    for (i = 0; i < MAX_NB_OPEN_FILES; i++)
        if (!open_fd_cache[i])
            break;
            
    if (i == MAX_NB_OPEN_FILES)
    {
        printerror("there are no shm back-end file descriptors left; returning...");
        return FAILURE;
    }
    
    struct open_shm_entry *e = my_malloc(sizeof(struct open_shm_entry)+1); //TODO malloc bug +1
    CHK_MALLOC(e);
    e->shm = shm;
    e->offset = 0;
    open_fd_cache[i] = e;
    
    return i;
}

// ############################ API IMPLEMENTATION ##############################

int SM_F("sfs") cfs_format(void)
{
    printdi_debug("sizeof struct shm_entry is %d", sizeof(struct shm_entry));
    printdi_debug("sizeof struct open_shm_entry is %d", sizeof(struct open_shm_entry));    
    
    struct shm_entry *cur, *next;
    for (cur = shm_list; cur != NULL; cur = next)
    {
        my_free(cur->malloc_ptr);
        next = cur->next;
        my_free(cur);
    }
    shm_list = NULL;
    
    int i;
    for (i = 0; i < MAX_NB_OPEN_FILES; i++)
        open_fd_cache[i] = NULL;
    
    init_free_list();
    
    return SUCCESS;
}

void SM_F("sfs") cfs_dump(void)
{
    struct shm_entry *cur; int i;

    printd_info(FCT("cfs_dump") "dumping shm_entry linked list:");
    puts("\t----------------------------------------------------------------");
    for (cur = shm_list; cur != NULL; cur = cur->next)
    {
        printf_int(BOLD "\tSHM_ENTRY" NONE " with name '%c' ", cur->name);
        printf_int("at %#x; ", (intptr_t) cur);
        printf_int("malloc_ptr = 0x%x; ", (intptr_t) cur->malloc_ptr);
        printf_int("size = %d; ", cur->size);
        printf_int("next_ptr = %#x\n", (intptr_t) cur->next);
    }
    puts("\t----------------------------------------------------------------");

    printd_info(FCT("cfs_dump") "dumping open_shm_entry file descriptor cache:");
    printf_str("\t");
    for (i = 0; i < MAX_NB_OPEN_FILES; i++)
    {
        struct open_shm_entry *e = open_fd_cache[i];
        printf_int_int("(%d, 0x%x", i, (intptr_t) e);
        if (e)
            printf_int_int(" : [0x%x, %d]); ", (intptr_t) e->shm, e->offset);
        else
            printf_str("); ");
    }    
    printf_str("\n");
}

int SM_F("sfs") cfs_open(const char *name, int flags, unsigned int size)
{
    char the_name = *name;
    
    // check whether already exists
    struct shm_entry *cur;
    for (cur = shm_list; cur != NULL; cur = cur->next)
        if (cur->name == the_name)
            return open_existing_file(cur);
    
    // request malloc block of desired size
    void *the_shm_block = my_malloc(size);
    CHK_MALLOC(the_shm_block)
    
    // insert new shm_entry struct in front
    cur = my_malloc(sizeof(struct shm_entry)+1); //TODO malloc bug +1
    if (!cur)
    {
        printerror("not enough malloc space left for internal data structures; returning...");
        my_free(the_shm_block);
        return FAILURE;
    }
    cur->name = the_name;
    cur->malloc_ptr = the_shm_block;
    cur->size = size;
    cur->next = shm_list;
    shm_list = cur;
    
    int fd = open_existing_file(cur);
    if (fd == FAILURE)
    {
        printerror("could not open the new file; returning...");
        my_free(the_shm_block);
        my_free(cur);
        return FAILURE;
    }
    return fd;
}

void SM_F("sfs") cfs_close(int fd)
{
    if (fd < 0 || fd >= MAX_NB_OPEN_FILES || !open_fd_cache[fd])
    {
        printdi_warning("provided shm back-end fd %d invalid for closing; returning...", fd);
        return;
    }
    
    my_free(open_fd_cache[fd]);
    open_fd_cache[fd] = NULL;
}

int SM_F("sfs") cfs_read(int fd, void *buf, unsigned int len)
{
    CHK_FD(fd);
    
    struct open_shm_entry *cur = open_fd_cache[fd];
    if (cur->offset >= cur->shm->size)
    {
        printdi_warning("reached EOF for file descriptor %d", fd);
        return EOF;
    }
    
    // TODO ignore the length argument for now and always read one char
    char *buff = (char *) buf;
    *buff = *(cur->shm->malloc_ptr + cur->offset++);
    return 1;
}

int SM_F("sfs") cfs_write(int fd, const void *buf, unsigned int len)
{
    CHK_FD(fd);
    
    struct open_shm_entry *cur = open_fd_cache[fd];
    if (cur->offset >= cur->shm->size)
    {
        printdi_warning("reached EOF for file descriptor %d", fd);
        return EOF;
    }
    
    // TODO ignore the length argument for now and always read one char
    char *buff = (char *) buf;
    *(cur->shm->malloc_ptr + cur->offset++) = *buff;
    return 1;
}

cfs_offset_t SM_F("sfs") cfs_seek(int fd, cfs_offset_t offset, int whence)
{
    CHK_FD(fd);
    struct open_shm_entry *fdp = open_fd_cache[fd];
    cfs_offset_t new_offset;
    
    if (whence == CFS_SEEK_SET)
        new_offset = offset;
    else if (whence == CFS_SEEK_CUR)
        new_offset = fdp->offset + offset;
    else
    {
        printerror_int("unknown or unsupported offset type '%d' for shm back-end", whence);
        return (cfs_offset_t)-1;
    }    
    
    if (new_offset < 0 || new_offset >= fdp->shm->size)
    {
        printerror_int("calculated new_offset '%d' is out of range", new_offset);
        return (cfs_offset_t)-1;
    }
    
    fdp->offset = new_offset;
    return SUCCESS;
}

// the front-end ensures a logical file is not removed when there are still open fds
int SM_F("sfs") cfs_remove(const char *name)
{
    char the_name = *name;
    struct shm_entry *cur, *prev;
    for (cur = prev = shm_list; cur != NULL; prev = cur, cur = cur->next)
        if (cur->name == the_name)
        {
            prev->next = cur->next;
            if (cur == shm_list)
                shm_list = cur->next;
                
            // TODO the implementation could zero-out the memory before freeing
            /*int i;
            for (i = 0; i < cur->size; i++)
                cur->malloc_ptr[i] = 0;
            */
            my_free(cur);
            return SUCCESS;
        }
    
    // file not found -> postcondition is ok
    return SUCCESS;
}

int SM_F("sfs") cfs_opendir(struct cfs_dir *dirp, const char *name)
{
    return -1;
}

int SM_F("sfs") cfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent)
{
    return -1;
}

void SM_F("sfs") cfs_closedir(struct cfs_dir *dirp)
{
    return;
}

void SM_F("sfs") cfs_ping(void)
{
    printdii_info(FCT("cfs_ping") "Hi from shared memory back-end; I have id %d " \
        "and was called by %d", sancus_get_self_id(), sancus_get_caller_id());
    return;
}

