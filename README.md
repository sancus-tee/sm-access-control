# Secure SM-grained Resource Sharing

This repository contains the source code accompanying the paper "Secure Resource Sharing for Embedded Protected Module Architectures" which appeared in the 2015 WISTP International Conference on Information Security Theory and Practice. A copy of the paper is available at <https://lirias.kuleuven.be/bitstream/123456789/513666/1/paper.pdf>.

> Van Bulck J., Noorman J., Mühlberg T., Piessens F. Secure resource sharing for embedded protected module architectures. In 9th WISTP International Conference on Information Security Theory and Practice (WISTP 2015), LNCS: Vol. 9311, pp. 71–87, Springer, 2015.

## Paper Abstract

Low-end embedded devices and the Internet of Things (IoT) are becoming increasingly important for our lives. They are being used in domains such as infrastructure management, and medical and healthcare systems, where business interests and our security and privacy are at stake. Yet, security mechanisms have been appallingly neglected on many IoT platforms.

In this paper we present a secure access control mechanism for extremely lightweight embedded microcontrollers. Being based on Sancus, a hardware-only Trusted Computing Base and Protected Module Architecture for the embedded domain, our mechanism allows for multiple software modules on an IoT-node to securely share resources. We implement and evaluate our approach for two application scenarios, a shared memory system and a shared flash drive. Our implementation is based on a Sancus-enabled TI MSP430 microcontroller. We show that our mechanism can give high security guarantees at small runtime overheads and a moderately increased size of the Trusted Computing Base.

## Source Code Overview

Instructions for building and running the programs are available at <https://distrinet.cs.kuleuven.be/software/sancus/wistp2015/>. The source code is organized as follows:

* __sfs__: contains the Sancus File System (SFS) implementation, a modified
[Contiki File System (CFS)](https://github.com/contiki-os/contiki/tree/master/core/cfs) interface
that provides SM-grained access control for a shared protected memory buffer or Contiki's Coffee file system for embedded peripheral flash driver.

* __sfs-benchark__: contains a test program measuring and printing the number
of CPU cycles needed for the various sfs functions

* __sfs-example__: contains a simple test setup and Makefile to compile and use the
sfs interface

* ./benchmark.h and ./common.h are utility headers
