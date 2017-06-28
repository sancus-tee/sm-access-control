/*
 * my_malloc.h: an interface for dynamic memory allocation inside a fixed sized static
 *  __unprotected__ memory buffer.
 */
#ifndef MALLOC_H
#define MALLOC_H

#include <stdlib.h>         // size_t

/*
 * Returns a pointer in the protected mem buf, to a free chunk of the requested size;
 *  or NULL iff no free chunk of sufficient size found
 */
void *my_malloc(size_t size);

/*
 * Initialises the free_list used by my_malloc. Note this must be done in a function, 
 *  since the HW will overwrite any initialisation before a call to sancus_enable.
 */
void init_free_list(void);

/*
 * Frees the previously allocated memory pointed to by @param(ptr). If ptr is not a 
 *  pointer obtained by a call to my_malloc(), the behaviour is unspecified.
 */
void my_free(void *ptr);

/*
 * Prints the current linked list of free chunks on stdout, iff DEBUG. Used for debugging.
 */
void print_free_list(void);

#endif //my_malloc.h
