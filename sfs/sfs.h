/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * Author:  Adam Dunkels <adam@sics.se>
 *          Jo Van Bulck :
 *              - renamed cfs_fct() to sfs_fct() to avoid naming conflicts when
 *                  importing CFS back-end
 *              - extended CFS interface with Sancus-module-grained
 *                  access control functions: sfs_chmod(), sfs_attest()
 *              - modified cfs_open semantics:
 *                  - file permissions as the 2nd argument
 *                  - requires a size argument to hint the intial size for a new file
 *                  - replaced CFS_READ, CFS_WRITE, CFS_APPEND with SFS
 *                      permission constants
 *              - replaced cfs_read() and cfs_write() with sfs_getc() and sfs_putc()
 *                  to transfer characters one at a time safely via CPU registers
 *              - removed directory related functions
 *              - annotated CFS functions with appropriate SM_ENTRY("sfs") tags
 *
 */
 
 /**
 * \file
 *         Sancus File System (SFS) header file: a modified CFS interface 
 *         to support Sancus-module-grained access control.
 * \author
 *         Adam Dunkels <adam@sics.se> (CFS)
 *
 *         Jo Van Bulck (SFS modifications)
 *
 */

#ifndef SFS_H_
#define SFS_H_

extern struct SancusModule sfs;

// ######################## CONFIG CONSTANTS ##########################

/**
 * Valid file permissions, used in access control lists assiciated with
 *  a file and client/Sancus SM. Permissions can be combined by bitwise-inclusive
 *  OR-ing the bit flags (in hex; 1 byte) below.
 *
 * SFS_NIL          no permissions at whole
 * SFS_CREATOR      all permissions, automatically assigned to the SM that first
 *                  created the file; non-assignable nor revocable (without deleting
 *                  the file); has all the SFS_ROOT bits (permissions) plus one
 * SFS_ROOT         all permissions, except SFS_CREATOR bit; revocable and assignable
 * SFS_READ         read-only access to the file
 * SFS_WRITE        write-only access to the file
 *
 * \note the permission set presented here can easily be extended to include for
 * example SFS_REMOVE, SFS_APPEND_ONLY, etc. permissions
 *
 * \sa sfs_open(), sfs_chmod()
 */
#define SFS_NIL           0x00
#define SFS_CREATOR       0xFF
#define SFS_ROOT          0x7F
#define SFS_READ          0x01
#define SFS_WRITE         0x02

/**
 * A typedef uniquely identifying a logical file; should be transfered safely through
 * CPU registers.
 *
 * \note cannot be a char* since such a pointer doesn't fit in a CPU register and
 * should thus point to non-protected memory, allowing an attacker to control the
 * filename argument arbitrarily.
 *
 * \sa sfs_open() sfs_remove() sfs_chmod()
 */
typedef char filename_t;

/**
 * Specify that sfs_open() should not create a new file if the file doesn't exist.
 *
 * \sa sfs_open()
 */
#define SFS_OPEN_EXISTING   -1  // any negative value will do

/**
 * Specify that sfs_seek() should compute the offset from the beginning of the file.
 *
 * \sa sfs_seek()
 */
#ifndef SFS_SEEK_SET
#define SFS_SEEK_SET 0
#endif

/**
 * Specify that sfs_seek() should compute the offset from the current position of
 * the file pointer.
 *
 * \sa sfs_seek()
 */
#ifndef SFS_SEEK_CUR
#define SFS_SEEK_CUR 1
#endif

/**
 * Specify that sfs_seek() should compute the offset from the end of the file.
 *
 * \sa sfs_seek()
 */
#ifndef SFS_SEEK_END
#define SFS_SEEK_END 2
#endif

// ######################## SFS API ##########################

/**
 * [NEW FUNCTION]
 * Used for testing: to substract the overhead of verifying the SFS SM first time
 */
void SM_ENTRY("sfs") sfs_ping(void);

/**
 * [NEW FUNCTION]
 * Used for testing: to substract the overhead of initializing the data structures
 * the first time; prints the time stamp count on stdout
 *  
 */
void SM_ENTRY("sfs") sfs_init(void);

/**
 * [NEW FUNCTION]
 * Used for debugging: to print the current access control data structures
 */
void SM_ENTRY("sfs") sfs_dump(void);

/**
 * [MODIFIED SEMANTICS]
 * \brief       Open a new or existing file.
 * \param name  The name of the file.
 * \param flags bitwise inclusive OR of the permission flags defined above
 * \param size  When wanting to open an existing file, SFS_OPEN_EXISTING flag; else
 *              the desired intial file size (>=0) for the new back-end file.
 *              Note that the intial size is merely a hint, the SFS back-end
 *              implementation is free to ignore this or not.
 * \return      A file descriptor, if the file could be opened, or -1 if
 *              the file could not be opened.
 *
 *              For a new file, the calling SM will be the owner (SFS_CREATOR perm)
 *              of the newly created empty file.
 *              For an existing file, the specified permission flags are checked
 *              against the associated file permissions for the calling SM.
 *              
 *              On successfull opening, the file descriptor can be used for future
 *              accesses and the internal file_pos is at the start of the file.
 *
 * \note        When the SFS_OPEN_EXISTING flag is provided through the size argument,
 *              this function will return failure for a non-existing file. If the
 *              caller wants to be sure a new file is created on success, he should
 *              provide SFS_CREATOR through the flags argument and a positive size.
 *
 * \note        When creating a new SFS file, any existing CFS file of the same name
 *              is first removed in the back-end. This reflects the non-persistent
 *              character of SFS files (i.e. remove any traces of previous runs).
 *
 * \sa          SFS_NIL, SFS_CREATOR, SFS_ROOT, SFS_READ, SFS_WRITE
 * \sa          sfs_close()
 */
int SM_ENTRY("sfs") sfs_open(filename_t name, int flags, int size);

/**
 * [MODIFIED SEMANTICS]
 * \brief      Close an open file.
 * \param fd   The file descriptor of the open file.
 * \return     A value >=0 on sucess; a negative value on failure (e.g. file
 *             descriptor out of range, calling SM not owner of file descriptor).
 *
 *             This function first performs access control on the calling SM and on
 *             success closes a file that has previously been opened with sfs_open().
 */
int SM_ENTRY("sfs") sfs_close(int fd);

/**
 * [NEW FUNCTION]
 * \brief      Read a single byte from an open file.
 * \param fd   The file descriptor of the open file.
 * \return     The read character (converted to an unsigned int);
 *             else EOF if the request could not be satisfied.
 *
 *             The caller must have SFS_READ permission on the open file.
 *             Since the provided char is transfered via a CPU register,
 *             this function can be used to safely (i.e. without 3th party
 *             interference) read confidential data from a file. 
 */
int SM_ENTRY("sfs") sfs_getc(int fd);

/**
 * [NEW FUNCTION]
 * \brief      Write data to an open file.
 * \param fd   The file descriptor of the open file.
 * \param c    The char to write into the file.
 * \return     The character written (converted to an unsigned int)
 *             else EOF if the request could not be satisfied.
 *
 *             The caller must have SFS_WRITE permission on the open file.
 *             Since the provided char is transfered via a CPU register,
 *             this function can be used to safely (i.e. without 3th party
 *             interference) write confidential data into a file.
 */
int SM_ENTRY("sfs") sfs_putc(int fd, unsigned char c);

/**
 * \brief      Seek to a specified position in an open file.
 * \param fd   The file descriptor of the open file.
 * \param offset A position, either relative or absolute, in the file.
 * \param whence Determines how to interpret the offset parameter.
 * \return     The new position in the file, or EOF if the seek failed.
 *
 *             This function moves the file position to the specified
 *             position in the file. The next byte that is read from
 *             or written to the file will be at the position given 
 *             determined by the combination of the offset parameter 
 *             and the whence parameter.
 *
 * \sa         SFS_SEEK_CUR
 * \sa         SFS_SEEK_END
 * \sa         SFS_SEEK_SET
 */
int SM_ENTRY("sfs") sfs_seek(int fd, int offset, int whence);

/**
 * [MODIFIED SEMANTICS]
 * \brief      Remove a file.
 * \param name The name of the file.
 * \retval 0   If the file was removed.
 * \return -1  If the file could not be removed or if it doesn't exist.
 *
 *             Removes the file, associated with name in the back-end.
 *             This function checks the calling SM's permissions and returns
 *             unsuccessfully if either some client still has an open file
 *             connection, the file doesn't exists or the caller doesn't
 *             have the SFS_ROOT permission.
 */
int SM_ENTRY("sfs") sfs_remove(filename_t name);

/**
 * [NEW FUNCTION]
 * \brief       Change permissions of a SM to a file.
 * \param name          The name for the file to access.
 * \param id            The id of the SM to change the permissions for.
 * \param perm_flags    The permissions to assign.
 * \return      A value >= 0 on success; a negative value if the
 *              request could not be satisfied.
 * 
 *              This function return unsuccessfully iff the specified file
 *              doesn't exist or the caller doesn't have the SFS_ROOT permission.
 *              The assigned permission_flags will replace any already existing
 *              permissions for the SM on the file. On success, any future access by
 *              the SM id on the file, not allowed by the specified permissions, will
 *              be denied.
 *
 * \note        Revoking/refining permissions on a file that is already opened by 
 *              the specified SM, still has direct effect.
 */
int SM_ENTRY("sfs") sfs_chmod(filename_t name, sm_id id, int perm_flags);

/**
 * [NEW FUNCTION]
 * \brief        Check a file exists and is created by a specified SM.
 * \param name   The name of the file to check.
 * \param owner  The id of the Sancus Module that should have created the file.
 * \return       A value >= 0 on success; -1 on failure.
 *
 *               Looks up the file @p(name) and verifies it was created by @p(owner).
 */
int SM_ENTRY("sfs") sfs_attest(filename_t name, sm_id owner);

#endif /* SFS_H_ */
