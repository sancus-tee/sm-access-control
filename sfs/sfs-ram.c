/**
 * A wrapper implementation of the Sancus File System (SFS) interface that first 
 * performs Sancus-module-grained access control using non-persistent RAM data
 * structures and thereafter translates the SFS call to a CFS-compatible back-end
 * call (pluggable at link time).
 *
 * \author Jo Van Bulck
 */

#include <sancus/sm_support.h>
#include <stdbool.h>
#include "sfs-debug.h"
#include "sfs.h"
#include "cfs/cfs.h"

#ifndef SUCCESS
    #define SUCCESS         1       // a positive value, to indicate success
#endif
#ifndef EOF
    #define EOF             -1
#endif
#ifndef FAILURE
    #define FAILURE         -1     // a negative value, to indicate failure
#endif

#ifdef MEASURE_CFS_BACKEND
    #include "../../benchmark.h"
#else
    #define TSC1()
    #define TSC2(str)
#endif

DECLARE_SM(sfs, 0x1234);

// ############################### FILE SYS PARAM #################################

// the system-wide max number of open files; defines open-file-cache size
#define MAX_NB_OPEN_FILES       8
// the system-wide max number of different files; defines file_pool size
#define MAX_NB_FILES            5
// the system-wide max number of assignable file permissions; defines perm_pool size
#define MAX_NB_PERMS            10

// ######################### GLOBAL PROTECTED DATA STRUCTURES #####################

// used to pass a character pointer to the uprotected back-end file system
char buf;
// used to translate the filename_t to a string ptr for the CFS backend
char public_str[2];

/**
 * two-phase lookup linked list structs to lookup by filename and sm_id
 *  (un)populated by sfs_creat() and sfs_remove()
 *
 * @invar(file->acl != NULL): a file has exactely one creator ACL entry
 *  on creation: sfs_creat(): file->acl->flags == SFS_CREATOR
 *  on revocation: sfs_chmod(): no adding/altering/removing of an SFS_CREATOR entry
 */

// these structs are aligned on a power of 2 so they can be indexed by the Sancus
// compiler, but actually need less space (e.g. byte flags instead of ints, etc)
struct FILE_PERM {
    sm_id sm_id;                        // 2 bytes
    int flags;                          // 2 bytes
    struct OPEN_FILE *file;             // 2 bytes
    struct FILE_PERM *next;             // 2 bytes
};

struct OPEN_FILE {
    filename_t name;                    // 2 bytes
    int open_count;                     // 2 bytes
    struct FILE_PERM *acl;              // 2 bytes
    struct OPEN_FILE *next;             // 2 bytes
};

// protected memory pools
struct FILE_PERM SM_DATA("sfs") perm_pool[MAX_NB_PERMS];
struct OPEN_FILE SM_DATA("sfs") file_pool[MAX_NB_FILES];

// open file cache: index with fd into the cache to get the corresponding permission
// (un)populated by sfs_open() and sfs_close()
struct FILE_PERM SM_DATA("sfs") *fd_cache[MAX_NB_OPEN_FILES];

// list management pointers
struct FILE_PERM SM_DATA("sfs") *free_perm_list;
struct OPEN_FILE SM_DATA("sfs") *free_file_list;
struct OPEN_FILE SM_DATA("sfs") *open_file_list;

// indicates data structures are intialized; set to false (zero) on SM creation
bool SM_DATA("sfs") INIT_DONE;

// ############################# HELPER MACROS ####################################

/******************* file descriptor checks *******************/

#define IS_VALID_FD(fd) \
    (fd >= 0 && fd < MAX_NB_OPEN_FILES)

#define CHK_FD(fd, sm) \
    if (!IS_VALID_FD(fd) || !fd_cache[fd] || fd_cache[fd]->sm_id != sm) \
    { \
        printerror_int_int("the provided file descriptor %d isn't valid or " \
            "doesn't belong to calling SM %d", fd, sm); \
        return FAILURE; \
    }

#define CLOSE_FD(fd) \
do { \
    fd_cache[fd]->file->open_count--; \
    fd_cache[fd] = NULL; \
    cfs_close(fd); \
} while(0)

/********************** permission checks *********************/

#define CHK_PERM(p_have, p_want) \
do { \
    if ((p_have & p_want) != p_want) { \
        printerror_int_int("permission check failed p_have=0x%x ; p_want = 0x%x", \
            p_have, p_want); \
        return FAILURE; \
    } \
} while(0)

// on success, p will point to the corresponding FILE_PERM struct
#define CHK_ROOT(name, id, p) \
    if (two_phase_lookup(name, SFS_ROOT, id, &p) < 0) \
        return FAILURE; \

// on success, p will point to the corresponding FILE_PERM struct
// and prev will point to the preceding OPEN_FILE struct
#define CHK_ROOT_PREV(name, id, p, prev) \
    if (two_phase_lookup_prev(name, SFS_ROOT, id, &p, &prev) < 0) \
        return FAILURE; \

/***************** permission list management *****************/

#define ALLOC_PERM(p, the_id, the_flags, the_file, the_next) \
do { \
    if (!free_perm_list) \
    { \
        printerror("no more FILE_PERM structs left"); \
        return FAILURE; \
    } \
    p = free_perm_list; \
    free_perm_list = free_perm_list->next; \
    p->sm_id = the_id; \
    p->flags = the_flags; \
    p->file = the_file; \
    p->next = the_next; \
} while(0)

#define FREE_PERM(p) \
do { \
    p->next = free_perm_list; \
    free_perm_list = p; \
} while(0)

// cur will point to the FILE_PERM struct if found; else NULL
#define LOOKUP_PERM(file, id, cur) \
do { \
    for (cur = file->acl; cur != NULL; cur = cur->next) \
        if (cur->sm_id == id) \
            break; \
} while (0)

/******************** file list management ********************/

#define ALLOC_FILE(the_file, the_name, the_acl, the_next) \
do { \
    if (!free_file_list) \
    { \
        printerror("no more OPEN_FILE structs left"); \
        return FAILURE; \
    } \
    the_file = free_file_list; \
    free_file_list = free_file_list->next; \
    the_file->name = the_name; \
    the_file->open_count = 0; \
    the_file->acl = the_acl; \
    the_file->next = the_next; \
} while(0)

#define FREE_FILE(f) \
do { \
    f->next = free_file_list; \
    free_file_list = f; \
} while(0)

// cur will point to the OPEN_FILE struct if found; else NULL
#define LOOKUP_FILE(name, cur) \
do { \
    for (cur = open_file_list; cur != NULL; cur = cur->next) \
        if (cur->name == name) \
            break; \
} while (0)

// cur will point to the OPEN_FILE struct if found; else NULL
// and prev will point to the preceding OPEN_FILE struct; or the last one
#define LOOKUP_FILE_PREV(name, cur, prev) \
do { \
    for (cur = prev = open_file_list; cur != NULL; prev = cur, cur = cur->next) \
        if (cur->name == name) \
            break; \
} while (0)

/*************************** various **************************/

#define DO_INIT() \
    if (!INIT_DONE) { \
        init_data_structures(); \
    }

#define TO_PUBLIC_STR(char_name) \
do { \
    public_str[0] = char_name; \
    /* optimalisation: following line is not needed (done on init_data_structures): \
     * public_str[1] = '\0'; \
     */ \
} while(0)

// ############################# HELPER FUNCTIONS #################################

void SM_FUNC("sfs") init_data_structures(void)
{
    printd_info("initializing data structures");
    printdi_debug("struct FILE_PERM size is %d", sizeof(struct FILE_PERM));
    printdi_debug("struct OPEN_FILE size is %d", sizeof(struct OPEN_FILE));

    // populate free_file_list linked list
    free_file_list = file_pool;
    int i; struct OPEN_FILE *cur_f = file_pool;
    for (i = 1; i < MAX_NB_FILES; i++, cur_f = cur_f->next)
        cur_f->next = &file_pool[i];
    cur_f->next = NULL;
    
    // populate free_perm_list linked list
    free_perm_list = perm_pool;
    struct FILE_PERM *cur_p = perm_pool;
    for (i = 1; i < MAX_NB_PERMS; i++, cur_p = cur_p->next)
        cur_p->next = &perm_pool[i];
    cur_p->next = NULL;
    
    // empty open_fd_cache
    // when using compiler optimalisations, requires MAX_NB_OPEN_FILES a power of 2
    for (i=0; i < MAX_NB_OPEN_FILES; i++)
        fd_cache[i] = 0;
    
    // empty open_file_list
    open_file_list = NULL;
    
    // init the null-termination character of te one-character string for cfs backend
    public_str[1] = '\0';
    
#ifndef NO_CFS_FORMAT
    printd_info("calling cfs_format");
    TSC1()
    cfs_format();
    TSC2("cfs_format")
#endif
    
    INIT_DONE = 1;
}

/**
 * Searches for the file associated with @param(name) and then checks permissions
 * @param(flags) for SM with id @param(sm). On success, zero is returned and
 * @param(p_ptr) points to the matching FILE_PERM struct. On failure -1 is returned.
 */
int SM_FUNC("sfs") two_phase_lookup_prev (filename_t name, int flags, sm_id sm,
    struct FILE_PERM **p_ptr, struct OPEN_FILE **prev_ptr)
{
    printdname_debug("two_phase_lookup for file", name);
    printdii_debug(" with permission flags 0x%x and SM %d", flags, sm);

    struct OPEN_FILE *file, *prev;
    if (prev_ptr)
    {
        LOOKUP_FILE_PREV(name, file, prev);
        *prev_ptr = prev;
    }
    else
        LOOKUP_FILE(name, file);
    
    if (!file)
        return FAILURE;
    
    printdi_debug("found open_file struct at address %#x; now searching ACL", file);
    struct FILE_PERM *p;
    LOOKUP_PERM(file, sm, p);
    if (!p)
    {
        printerror_int("found no perm entry for SM %d", sm);
        return FAILURE;
    }
    printdi_debug("found file_perm struct at address %#x", p);
    
    *p_ptr = p;
    CHK_PERM(p->flags, flags); // might return failure
    
    return SUCCESS;
}

#define two_phase_lookup(name, flags, sm, p) \
    two_phase_lookup_prev(name, flags, sm, p, NULL)

/**
 * Attempts to open the file @p(name) in the CFS back-end and populates the
 * corresponding fd_cache entry with @p(p) iff a valid back-end fd is returned.
 */
int SM_FUNC("sfs") open_back_end_file(char name, int size, struct FILE_PERM *p)
{
    // open with read write in the back-end; front-end will take care of finer
    // grained permissions
    printd_debug("opening file in back-end");
    TO_PUBLIC_STR(name);
    TSC1()
    int fd = cfs_open(public_str, CFS_WRITE | CFS_READ, size);
    TSC2("cfs_open")
    printdi_debug("the returned back-end fd is %d", fd);
    
    if (!IS_VALID_FD(fd))
    {
        printerror_int("returned back-end fd %d out of range", fd);
        cfs_close(fd);
        return FAILURE;
    }
    p->file->open_count++;
    fd_cache[fd] = p;
    
    return fd;
}

/**
 * Helper function to revoke the ACL entry for SM @param(id) on file @param(file)
 * iff non-SFS_CREATOR ACL entry (to satisfy invar).
 */
int SM_FUNC("sfs") revoke_acl(struct OPEN_FILE *file, sm_id id)
{
    // walk the ACL to check for existing entry
    struct FILE_PERM *cur, *prev; int i;
    for (cur = prev = file->acl; cur != NULL; prev = cur, cur = cur->next)
        if (cur->sm_id == id)
        {
            if (cur->flags == SFS_CREATOR)
            {
                printerror("SFS_CREATOR permission is non-revocable");
                return FAILURE;
            }
            else
            {
                // free the permission entry; close any open file descriptors first
                for (i = 0; i < MAX_NB_OPEN_FILES; i++)
                    if (fd_cache[i] == cur)
                    {
                        printdi_warning("ACL entry currently open; now closing fd %d", i);
                        CLOSE_FD(i);                        
                    }
            
                printdi_warning("removing ACL entry at address %#x", cur);
                prev->next = cur->next;
                FREE_PERM(cur);
                return SUCCESS;
            }
        }
    
    // no existing entry in the ACL; postcondition is ok
    return SUCCESS;
}

/**
 * Helper function to add/override the ACL entry with permissions @param(perm_flags)
 * for SM @param(id) on file @param(file) iff non-SFS_CREATOR (to satisfy invar).
 */
int SM_FUNC("sfs") add_acl(struct OPEN_FILE *file, sm_id id, int perm_flags)
{
    // walk the access control list; override any existing permission
    struct FILE_PERM *p, *prev;
    for (p = prev = file->acl; p != NULL; prev = p, p = p->next)
        if (p->sm_id == id)
        {
            if (p->flags == SFS_CREATOR)
            {
                printerror("SFS_CREATOR permission is non-overrideable");
                return FAILURE;
            }
            else
            {
                printdi_warning("overriding existing ACL entry flag %#x", p->flags);
                p->flags = perm_flags;
                return SUCCESS;
            }
        }
        
    // no existing entry in the access control list; try to append new one
    printd_debug("appending additional ACL entry");
    if (perm_flags == SFS_CREATOR)
    {
        printerror("SFS_CREATOR permission is non-assignable");
        return FAILURE;
    }
    
    ALLOC_PERM(p, id, perm_flags, file, NULL);
    // prev != NULL because a file always has at least the owner ACL entry (invar)
    prev->next = p;

    return SUCCESS;   
}

// ######################### SFS API IMPLEMENTATION ###############################

void SM_ENTRY("sfs") sfs_ping(void)
{   
    printdii_info(FCT("sfs_ping") "Hi from sfs-ram; I have id %d and was called " \
     "by %d; now calling cfs_ping()", sancus_get_self_id(), sancus_get_caller_id());

#if !defined(NODEBUG) || defined(MEASURE_CFS_BACKEND)
    TSC1()
    cfs_ping();
    TSC2("cfs_ping()")
#endif

    return;
}

void SM_ENTRY("sfs") sfs_init(void)
{
    printd_info(FCT("sfs_init") "calling DO_INIT macro");
    DO_INIT()
}

void SM_ENTRY("sfs") sfs_dump(void)
{
#ifndef NODEBUG
    struct OPEN_FILE *f; struct FILE_PERM *p; int i;
    DO_INIT()
        
    printd_info(FCT("sfs_dump") "dumping global protected ACL data structures:");
    puts("\t----------------------------------------------------------------");
    for (f = open_file_list; f != NULL; f = f->next)
    {
        printf_int(BOLD "\tFILE" NONE " with name '%c'", f->name);
        printf_int(" at %#x", (intptr_t) f);
        printf_int("; open_count = %d; ", f->open_count);
        printf_int("next_ptr = %#x\n", (intptr_t) f->next);
        for (p = f->acl; p != NULL; p = p->next)
        {
            printf_int(BOLD "\t\tPERM" NONE " (%d", p->sm_id);
            printf_int(", 0x%02x) ", p->flags);
            printf_int("at %#x; ", (intptr_t) p);
            printf_int("file_ptr = %#x; ", (intptr_t) p->file);
            printf_int("next_ptr = %#x\n", (intptr_t) p->next);
        }
    }
    puts("\t----------------------------------------------------------------");
    
    printd_info(FCT("sfs_dump") "dumping global protected file descriptor cache:");
    printf_str("\t");
    for (i = 0; i < MAX_NB_OPEN_FILES; i++)
        printf_int_int("(%d, 0x%x); ", i, (intptr_t) fd_cache[i]);
    printf_str("\n");
    
    for (i = 0, f = free_file_list; f != NULL; i++, f = f->next)
        ;
    printdii_debug("free file list size is %d max is %d\n", i, MAX_NB_FILES);
    
    for (i = 0, p = free_perm_list; p!=NULL; i++, p = p->next)
        ;
    printdii_debug("free perm list size is %d max is %d\n", i, MAX_NB_PERMS);


#ifdef CFS_DUMP    
    printd_info(FCT("sfs-dump") "now calling cfs_dump");
    cfs_dump();
#endif

#else // NODEBUG
    return;
#endif
}

int SM_ENTRY("sfs") sfs_open(filename_t name, int flags, int size)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()
    
    struct OPEN_FILE *file; struct FILE_PERM *p;
    LOOKUP_FILE(name, file);
    if (!file)
    {
        if (size < 0)
        {
            printerror_int(FCT("sfs_open") "caller requested not to create new " \
            "non-existing file '%c'; returning failure...", (int) name);
            return FAILURE;
        }
        printdname_info(FCT("sfs_open") "creating new file with name", name);
        
        /* 
        printd_debug("removing any existing file of the same name in the back-end");
        TO_PUBLIC_STR(name);
        cfs_remove(public_str);
        */
    
        if (!free_perm_list || !free_file_list)
        {
            printerror("need a free OPEN_FILE struct and FILE_PERM struct");
            return FAILURE;
        }
    
        // create the new file and add it in front of open_file_list
        ALLOC_FILE(file, name, NULL, open_file_list);
        open_file_list = file;
        ALLOC_PERM(p, caller_id, SFS_CREATOR, file, NULL);
        file->acl = p;
    
        return open_back_end_file(name, size, p);
    }

    printdname_info(FCT("sfs_open") "opening existing file with name", name);
    LOOKUP_PERM(file, caller_id, p);
    if (!p)
    {
        printerror_int("found no permission entry for SM %d", caller_id);
        return FAILURE;
    }
    CHK_PERM(p->flags, flags); // might return failure
    
    return open_back_end_file(name, size, p);
}

int SM_ENTRY("sfs") sfs_close(int fd)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()
    printdi_info(FCT("sfs_close") "file with fd %d", fd);

    CHK_FD(fd, caller_id)    
    fd_cache[fd]->file->open_count--;
    fd_cache[fd] = NULL;

    TSC1()
    cfs_close(fd);
    TSC2("cfs_close")
    
    return SUCCESS;
}

/**
 * this implementation refuses to remove a file that is still open
 * by some SM; an alternative would be that the implementation closes
 * any remaining open file connection on behalf of the caller (owner of the file)
 */
int SM_ENTRY("sfs") sfs_remove(filename_t name)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()
    printdname_info(FCT("sfs_remove") "trying to remove file", name);
    
    // only root can remove a file
    struct FILE_PERM *p; struct OPEN_FILE *cur, *prev;
    CHK_ROOT_PREV(name, caller_id, p, prev);
    cur = p->file;
    
    if (cur->open_count)
    {
        printerror_int("there are %d remaining open file connections;" \
        " close them first", cur->open_count);
        return FAILURE;
    }
    
    // remove associated access control data structures
    prev->next = cur->next;
    struct FILE_PERM *curp, *nextp;
    for (curp = cur->acl; curp != NULL; curp = nextp)
    {
        nextp = curp->next;
        FREE_PERM(curp);
    }
    if (open_file_list == cur)
        open_file_list = cur->next;
    FREE_FILE(cur);
    
    printd_debug("now removing file in CFS back-end");
    TO_PUBLIC_STR(name);
    TSC1()
    int rv = cfs_remove(public_str);
    TSC2("cfs_remove")
    
    return rv;
}

int SM_ENTRY("sfs") sfs_getc(int fd)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()
    printdi_info(FCT("sfs_getc") "read a char from file with fd %d", fd);
   
    CHK_FD(fd, caller_id)
    CHK_PERM(fd_cache[fd]->flags, SFS_READ);

    TSC1()
    int rv = cfs_read(fd, &buf, 1);
    TSC2("cfs_read_one_char")

    printdi_debug("cfs_read returned %d", rv);
    return (rv > 0)? buf : EOF;
}

int SM_ENTRY("sfs") sfs_putc(int fd, unsigned char c)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()
    printdi_info(FCT("sfs_putc") "write a char to file with fd %d", fd);

    CHK_FD(fd, caller_id)
    CHK_PERM(fd_cache[fd]->flags, SFS_WRITE);
    
    TSC1()
    buf = c; // this is part of the back-end function call overhead...
    int rv = cfs_write(fd, &buf, 1);
    TSC2("cfs_write_one_char")

    printdi_debug("cfs_write returned %d", rv);
    return (rv > 0)? buf : EOF;
}

int SM_ENTRY("sfs") sfs_seek(int fd, int offset, int origin)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()
    printdi_info(FCT("sfs_seek") "now trying to seek in file with fd %d", fd);
    CHK_FD(fd, caller_id)

    TSC1()
    int rv = cfs_seek(fd, offset, origin);
    TSC2("cfs_seek")
    
    return rv;
}

int SM_ENTRY("sfs") sfs_chmod(filename_t name, sm_id id, int perm_flags)
{
    sm_id caller_id = sancus_get_caller_id();
    DO_INIT()

    printdname_info(FCT("sfs_chmod") "trying to modify ACL for file", name);
    printdi_debug("wanted permission entry for SM %d", id);
    printdi_debug("and flags %#x", perm_flags);

    // only root can chmod
    struct FILE_PERM *p_caller;
    CHK_ROOT(name, caller_id, p_caller)
    
    if (perm_flags == SFS_NIL)
        return revoke_acl(p_caller->file, id);
   
    return add_acl(p_caller->file, id, perm_flags);
}

int SM_ENTRY("sfs") sfs_attest(filename_t name, sm_id owner)
{
    printdii_info(FCT("sfs_attest") "validating file '%c' was created by SM %d",
        (char) name, owner);

    struct OPEN_FILE *file; struct FILE_PERM *p;
    LOOKUP_FILE(name, file);
    if (!file)
        return FAILURE;
    
    LOOKUP_PERM(file, owner, p);
    if (!p)
        return FAILURE;
    
    CHK_PERM(p->flags, SFS_CREATOR);
    
    return SUCCESS;
}
