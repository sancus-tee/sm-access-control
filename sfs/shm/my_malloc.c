/**
 * A proof-of-concept (crappy) malloc implementation allocating memory blocks in
 *  a private Sancus-protected memory buffer.
 */

#include "my_malloc.h"

#ifdef MY_MALLOC_DEBUG
    #include "../../../common.h"
    #define MALL        CYAN "\t\tmy_malloc: " NONE
    #define FREE        CYAN "\t\tmy_free: " NONE
#else
    #define printdebug(str)
    #define printdebug_int(str, int)
    #define printdebug_int_int(str, i, j)
    #define printerr(str)
    #define MALL
    #define FREE
#endif

// #################### MALLOC DATASTRUCTURES ########################

#define INIT_BUF_SIZE   1000    // device has 10 KB (10240B) data memory
// request a protected buf for malloc
char SM_D("sfs") buf[INIT_BUF_SIZE];

// linked list of free chunks to be allocated
// Note the overhead per free_chunk: save new free_chunk and keep old free_chunk
struct FREE_CHUNK {
    size_t size;
    struct FREE_CHUNK *next;
    // max allocatable free space = &(free_chunk->next)+1 until &(free_chunk->next) + size
};
struct FREE_CHUNK SM_D("sfs") *free_list_head;


// ###########################  MALLOC IMPLEMENTATION ##########################

/*
 * Returns a pointer in the protected char array, to a free chunk of the requested size;
 *  or NULL iff no free chunk of sufficient size found
 */
void SM_F("sfs") *my_malloc(size_t size) {
    //printdebug(MALL "hi from my_malloc!");

    if (!free_list_head) init_free_list();
    // walk through the list of free_chunks until one is found that is large enough to 
    // hold the request (i.e. "first-fit")
    struct FREE_CHUNK *prev = free_list_head;
    struct FREE_CHUNK *cur = free_list_head;
    while (cur != NULL && cur->size < size) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printerr(MALL "the end of the free list was reached; " \
            "no free_chunk of sufficient size found. Returning null...");
        return NULL;
    }
    
    // allocate the memory and create a new free_chunk for the remainder, if any
    // note: save the old free_chunk and the allocated size just above the returned pointer
    void *ret = (void*) cur + sizeof(struct FREE_CHUNK) + 1;
    int remainder = cur->size - size - sizeof(struct FREE_CHUNK);
    
    if (remainder > 0) {
        // create a new free_chunk for the remainder
        cur->size = size;
        struct FREE_CHUNK *new = (void*) cur + sizeof(struct FREE_CHUNK) + size + 1;
        new->size = remainder;
        new->next = cur->next;
        prev->next = new;
    } else {
        printdebug_int(MALL "no remainder in the allocated free_chunk 0x%x\n", cur);
        // allocate all to the result
        prev->next = cur->next;
    }
    printdebug_int_int(MALL "returning address 0x%x with size %d\n", ret, size);
    return (void*) ret;
}

/*
 * Initialises the free_list used by my_malloc. Note this must be done in a function, 
 *  since the HW will overwrite any initialisation before a call to sancus_enable.
 */
void SM_F("sfs") init_free_list(void) {
    free_list_head = (struct FREE_CHUNK*) buf;
    free_list_head->size = 0; // to keep the free_list_head pointer static
    
    struct FREE_CHUNK *new = (struct FREE_CHUNK*) (buf + sizeof(struct FREE_CHUNK));
    free_list_head->next = new;
    new->size = INIT_BUF_SIZE - sizeof(struct FREE_CHUNK)*2;
    new->next = NULL; //XXX cirkular? optimal?
    printdebug(MALL "the init free list is:");
    print_free_list();
}

/*
 * Frees the previously allocated memory pointed to by @param(ptr). If ptr is not a 
 *  pointer obtained by a call to my_malloc(), the behaviour is unspecified.
 */
void SM_F("sfs") my_free(void *ptr) {
    if (!ptr || ptr < (void*) &buf || ptr > (void*) &buf+INIT_BUF_SIZE-1) {
        printerr(FREE "the given pointer is outside the buf boundaries. Returning...");
        return;
    }
    
    // re-access the old free_chunk, saved at allocation time
    struct FREE_CHUNK *new  = (struct FREE_CHUNK*) (ptr - sizeof(struct FREE_CHUNK) - 1);
    printdebug_int(FREE "the size to free is: %d\n", new->size);
    
    // search the (sorted) free list to insert the free block
    struct FREE_CHUNK *cur = free_list_head;
    while (cur != NULL && !(ptr > (void*) cur && (cur->next == NULL || ptr < (void*) cur->next)))
        cur = cur->next;
    if (!cur) {
        printerr(FREE "insertion failed; no free chunk found");
        return;
    }
    
    // check if merging is possible
    // TODO TODO check if merged chunk can again be merged...
    if (ptr+new->size == (void*) cur->next) {
        printdebug_int(FREE "merging before 0x%x\n", cur->next);
        new->next = cur->next->next;
        new->size += sizeof(struct FREE_CHUNK) + cur->next->size;
        cur->next = new;
    }
    else if (cur != free_list_head && (void*) cur + sizeof(struct FREE_CHUNK)*2 + cur->size == ptr) {
        // don't merge with the (static) head free_chunk
        printdebug_int(FREE "merging after 0x%x\n", cur);
        cur->size += sizeof(struct FREE_CHUNK) + new->size;
    }
    else {
        printdebug_int(FREE "inserting new free_chunk after 0x%x\n", cur);
        new->next = cur->next;
        cur->next = new;
    }
    //print_free_list();
}

/*
 * Prints the current linked list of free chunks on stdout, iff DEBUG. Used for debugging.
 */
void SM_F("sfs") print_free_list(void) {
    printdebug("\t\t------------------------------");
    struct FREE_CHUNK *cur = free_list_head;
    for (; cur != NULL; cur = cur->next) {
        printdebug_int("\t\tfree_chunk; size=%d", cur->size);
        printdebug_int("\t addr=0x%x", cur);
        printdebug_int("\t next=0x%x\n", cur->next);
    }
    printdebug("\t\t------------------------------");
}
