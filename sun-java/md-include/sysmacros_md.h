/*
 * @(#)sysmacros_md.h	1.1 95/10/06  
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#ifndef SYSMACROS_MD_H_
#define SYSMACROS_MD_H_

#include "prosdep.h"
#include "prfile.h"
#include "prlog.h"
#include "prthread.h"

#define sysMalloc(size) (malloc(size))
#define sysFree(ptr)	(free(ptr))
#define sysCalloc(num, size) (calloc(num, size))
#define sysRealloc(ptr, size) (realloc(ptr, size))

#define sysAssert(ex) PR_ASSERT(ex)

/************************************************************************/

#ifdef XP_UNIX
#include <dirent.h>
#include <fcntl.h>
#include <sys/param.h>

#define sysOpenDir(path)                PR_OpenDirectory(path)
#define sysReadDir(dir)                 PR_ReadDirectory(dir, 0)
#define sysCloseDir(dir)                PR_CloseDirectory(dir)

#define sysRead(fd,buf,n)               read(fd,buf,n)
#define sysWrite(fd,buf,n)              write(fd,buf,n)
#define sysSeek(fd,off,whence)			lseek(fd,off,whence)
#define sysClose(fd)                    close(fd)
#define sysCloseSocket(fd)              close(fd)
#define sysAccess(path,mode)            access(path,mode)
#define sysStat(path,buf)               stat(path,buf)
#define sysOpen                         open
extern int sysAvailable(int fd, long *bytes);
#define sysIsAbsolute(path)             (*(path) == '/')
#define sysUnlink(path)					unlink(path)

#endif  /* XP_UNIX */

#ifdef XP_PC
#ifdef _WIN32
#define MAXPATHLEN                      MAX_PATH
#else
#define MAXPATHLEN                      260
#endif

#define S_ISDIR( m )                    (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG( m )                    (((m) & S_IFMT) == S_IFREG)

#define sysOpenDir(path)                PR_OpenDirectory(path)
#define sysReadDir(dir)                 PR_ReadDirectory(dir, 0)
#define sysCloseDir(dir)                PR_CloseDirectory(dir)

#define sysOpen(name,mode,mask)         PR_OpenFile(name,mode|O_BINARY,mask)
#define sysRead(fd,buf,n)               PR_READ(fd,buf,n)
#define sysWrite(fd,buf,n)              PR_WRITE(fd,buf,n)
#define sysSeek(fd,off,whence)			lseek(fd,off,whence)
#define sysClose(fd)                    PR_CLOSE(fd)
#define sysCloseSocket(fd)              closesocket(fd)
#define sysIsAbsolute(path)             (*(path) == '\\')
#define sysUnlink(path)					unlink(path)

#define sysStat(path,buf)               stat(path,buf)

/* #define printf                          PR_PRINTF */

/* Symbolic constants for the access() function */
#define R_OK    PR_AF_READ_OK           /*  Test for read permission    */
#define W_OK    PR_AF_WRITE_OK          /*  Test for write permission   */
#define F_OK    PR_AF_EXISTS            /*  Test for existence of file  */

#define sysAccess(path,mode)            PR_AccessFile(path,mode)

#endif  /* XP_PC */

#ifdef XP_MAC

#include <unistd.h>
#include <stdio.h>
#include "xp_sock.h"

#define DIR                             PRDir
#define dirent                          PRDirEntryStr

#define sysOpen(name,mode,mask)         PR_OpenFile(name, mode, mask)
#define fopen(name, mode)				_OS_FOPEN(name, mode)
#define sysRead(fd,buf,n)               read(fd, buf, n)
#define sysWrite(fd,buf,n)              write(fd, buf, n)
#define sysSeek(fd,off,whence)			PR_Seek(fd,off,whence)
#define sysClose(fd)                    close(fd)
#define sysCloseSocket(fd)              XP_SOCK_CLOSE(fd)

#define sysStat(path,buf)               PR_Stat(path,buf)

#define sysOpenDir(path)                PR_OpenDirectory(path)
#define sysReadDir(dir)                 PR_ReadDirectory(dir, 0)
#define sysCloseDir(dir)                PR_CloseDirectory(dir)

#define sysUnlink(path)					PR_Unlink(path)

/* Symbolic constants for the access() function */
#define R_OK    PR_AF_READ_OK           /*  Test for read permission    */
#define W_OK    PR_AF_WRITE_OK          /*  Test for write permission   */
#define F_OK    PR_AF_EXISTS            /*  Test for existence of file  */

#define sysAccess(path,mode)            PR_AccessFile(path,mode)

#define sysIsAbsolute(path)             (*(path) == '/')

#endif  /* XP_MAC */

#define sysMkdir(path,mode)             PR_Mkdir(path, mode)

/************************************************************************/

/*
 * Check whether an exception occurred.  This also gives us the oppor-
 * tunity to use the already-required exception check as a trap for other
 * system-specific conditions.
 */

#ifdef PR_NO_PREEMPT
#define sysCheckException(ee)			    		\
	if ((ee)->exceptionKind == EXCKIND_NONE) {  	\
	    continue;				    				\
	}					    						\
	if ((ee)->exceptionKind == EXCKIND_YIELD) { 	\
	    (ee)->exceptionKind = EXCKIND_NONE;	    	\
		fb = 0;				    					\
		mb = 0;				    					\
		mbt = 0;				    				\
		location = 0;				    			\
		cb = 0;				    					\
		array_cb = 0;				    			\
		o = 0;				    					\
		h = 0;				    					\
		d = 0;				    					\
		current_class = 0;				    		\
		superClass = 0;				    			\
		retO = 0;				    				\
		retH = 0;				    				\
		arr = 0;				    				\
		signature = 0;				    			\
		retArray = 0;				    			\
	    PR_Yield();				    				\
	    continue;				   					\
	}												
#else
#define sysCheckException(ee) 		\
	if (!exceptionOccurred(ee)) { 	\
	   continue; \
	}
#endif /* PR_NO_PREEMPT */


#endif /*SYSMACROS_MD_H_*/
