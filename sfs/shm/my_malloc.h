/*
 * my_malloc.h: an interface for dynamic memory allocation inside a fixed sized static
 *  Sancus-protected memory buffer.
 */
#ifndef MALLOC_H
#define MALLOC_H

#include <stdlib.h>         // size_t
#include "../cfs/cfs.h"     // SM_F an SM_D macros to enable/disable back-end protection

/*
 * Returns a pointer in the protected mem buf, to a free chunk of the requested size;
 *  or NULL iff no free chunk of sufficient size found
 */
void SM_F("sfs") *my_malloc(size_t size);

/*
 * Initialises the free_list used by my_malloc. Note this must be done in a function, 
 *  since the HW will overwrite any initialisation before a call to sancus_enable.
 */
void SM_F("sfs") init_free_list(void);

/*
 * Frees the previously allocated memory pointed to by @param(ptr). If ptr is not a 
 *  pointer obtained by a call to my_malloc(), the behaviour is unspecified.
 */
void SM_F("sfs") my_free(void *ptr);

/*
 * Prints the current linked list of free chunks on stdout, iff DEBUG. Used for debugging.
 */
void SM_F("sfs") print_free_list(void);

#endif //my_malloc.h
