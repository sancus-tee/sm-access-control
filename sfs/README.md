## Sancus File System (SFS)

SFS source code, a modified Contiki File System (CFS) interface
(https://github.com/contiki-os/contiki/tree/master/core/cfs)
that provides SM-grained access control for a shared memory buffer or Contiki's Coffee FS

* __sfs.h__: the definitions of the Sancus File System (SFS), a modified Contiki
File System (CFS) interface that support SM-grained access control.

* __sfs-ram.c__: a wrapper SFS implementation that first performs access control using
non-persistent RAM data structures and thereafter translates the call to a
CFS-compatible back-end (pluggable at link time through the Makefile).

* __sfs-debug.h__: debug output macros to support pretty printing and toggling
of fine grained debug level output

* __cfs__: contains the Contiki File System (CFS) definitions and

    * the Coffee file system implementation from Contiki
    (https://github.com/contiki-os/contiki/blob/master/core/cfs/cfs-coffee.c),
    slightly modified to run in unprotected mode on the Sancus FPGA. 
    * the driver for the ST M25P16 flash disk

* __shm__: contains an implementation of the CFS interface that realises
protected shared memory through a malloc implementation on top of a fixed sized
Sancus-protected buffer.
