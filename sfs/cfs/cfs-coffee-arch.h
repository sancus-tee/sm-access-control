/*
 * Copyright (c) 2008, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *	Coffee architecture-dependent header for the Tmote Sky platform.
 * \author
 * 	Nicolas Tsiftes <nvt@sics.se>
 *  Jo Van Bulck: modified platform defined macros to support a MSP430F149
 *  Sancus FPGA with 16MB Micron M25P16 Serial Flash memory
 *
 */

#ifndef CFS_COFFEE_ARCH_H
#define CFS_COFFEE_ARCH_H

//#include "contiki-conf.h"
//#include "dev/xmem.h"
#include "flash_driver.h"

/* Coffee configuration parameters. */
/*
 * jo: from the M25P16 data sheet: "It is organized as 32 sectors, each
 * containing 256 pages. Each page is 256 bytes wide. Memory
 * can be viewed either as 8,192 pages or as 2,097,152 bytes"
 *
 */
#define COFFEE_SECTOR_SIZE		65536UL // 256 (pages/sector) * 256 (bytes/page)
#define COFFEE_PAGE_SIZE		256UL
#define COFFEE_START			0 // TODO COFFEE_SECTOR_SIZE
#define COFFEE_SIZE			    (2097152UL - COFFEE_START)
#define COFFEE_NAME_LENGTH		2       // TODO these parameters should match those of the SFS front-end --> include sfs-config.h
#define COFFEE_MAX_OPEN_FILES   6      
#define COFFEE_FD_SET_SIZE		8
#define COFFEE_LOG_TABLE_LIMIT  256
#ifdef COFFEE_CONF_DYN_SIZE
#define COFFEE_DYN_SIZE			COFFEE_CONF_DYN_SIZE
#else
#define COFFEE_DYN_SIZE         4*1024
#endif
#define COFFEE_LOG_SIZE			1024

#define COFFEE_IO_SEMANTICS		1
#define COFFEE_APPEND_ONLY		0
#define COFFEE_MICRO_LOGS		1

/* Flash operations. */
// jo: "write" is flash-"program" here
#define COFFEE_WRITE(buf, size, offset)				\
        sf_program_page(COFFEE_START + (offset), (char *)(buf), (size))
		//xmem_pwrite((char *)(buf), (size), COFFEE_START + (offset))

#define COFFEE_READ(buf, size, offset)				\
        sf_read(COFFEE_START + (offset),  (char *)(buf), (size))
  		//xmem_pread((char *)(buf), (size), COFFEE_START + (offset))

#define COFFEE_ERASE(sector_nb)					\
        sf_sector_erase(COFFEE_START + (sector_nb) * COFFEE_SECTOR_SIZE)
  		//xmem_erase(COFFEE_SECTOR_SIZE, COFFEE_START + (sector) * COFFEE_SECTOR_SIZE)

// jo: for testing purposes
#define COFFEE_READ_ID(buf_ptr, buf_size)                 \
        sf_read_id(buf_ptr, buf_size)

/* Coffee types. */
// Jo: This data type must be able to hold COFFEE_SIZE/COFFEE_PAGE_SIZE pages.
typedef int16_t coffee_page_t;

#endif /* !COFFEE_ARCH_H */
