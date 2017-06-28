## Logical File Access Control

The source code on Sancus-module-grained access control:

* __sfs__: contains the Sancus File System (SFS) implementation, a modified
Contiki File System (CFS) interface (https://github.com/contiki-os/contiki/tree/master/core/cfs)
that provides SM-grained access control for a shared memory buffer or Contiki's Coffee FS

* __sfs-benchark__: contains a test program measuring and printing the number
of CPU cycles needed for the various sfs functions

* __sfs-example__: contains a simple test setup and Makefile to compile and use the
sfs interface

* ./benchmark.h and ./common.h are utility headers
