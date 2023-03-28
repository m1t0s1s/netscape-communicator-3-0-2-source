/*
 * @(#)sys_api.h	1.39 95/10/25  
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

/*
 * System or Host dependent API.  This defines the "porting layer" for
 * POSIX.1 compliant operating systems.
 */

#ifndef _SYS_API_H_
#define _SYS_API_H_

/*
 * typedefs_md.h includes basic types for this platform;
 * any macros for HPI functions have been moved to "sysmacros_md.h"
 */
#include "typedefs_md.h"

/*
 * Some platforms, notably Windows, require that extra keywords
 * be prepended for things that can be called from other libs.
 * The default is just to declare things "extern".
 */
#ifndef EXPORTED
#define EXPORTED extern
#endif

/*
 * Miscellaneous system utility APIs that fall outside the POSIX.1
 * spec.
 *
 * Until POSIX (P1003.1a) formerly P.4 is standard, we'll use our
 * time struct definitions found in timeval.h.
 */
#include "timeval.h"
EXPORTED long 	sysGetMilliTicks(void);
EXPORTED long    sysTime(long *);
EXPORTED int64_t sysTimeMillis(void);

#include <time.h>
EXPORTED struct tm * sysLocaltime(time_t *, struct tm*);
EXPORTED struct tm * sysGmtime(time_t *, struct tm*);
EXPORTED void sysStrftime(char *, int, char *, struct tm *);
EXPORTED time_t sysMktime(struct tm*);

/*
 * System API for general allocations
 */
EXPORTED void *	sysMalloc(size_t);
EXPORTED void *	sysRealloc(void*, size_t);
EXPORTED void	sysFree(void*);
EXPORTED void *	sysCalloc(size_t, size_t);

/*
 * System API for dynamic linking libraries into the interpreter
 */
EXPORTED char * sysInitializeLinker(void);
EXPORTED int    sysAddDLSegment(char *);
EXPORTED void   sysSaveLDPath(char *);
EXPORTED long   sysDynamicLink(char *, void* *lib);
EXPORTED void	sysBuildLibName(char *, int, char *, char *);

/*
 * System API for threads
 */
typedef struct   sys_thread sys_thread_t;
typedef struct   sys_mon sys_mon_t;
typedef void   * stackp_t;

EXPORTED int 	 sysThreadBootstrap(sys_thread_t **);
EXPORTED void 	 sysThreadInitializeSystemThreads(void);
EXPORTED int 	 sysThreadCreate(long, uint_t flags, 
				     void *(*)(void *), 
				     sys_thread_t **,
	 			     void *);
EXPORTED  void	 sysThreadExit(void);
EXPORTED  sys_thread_t * sysThreadSelf(void);
EXPORTED  void	 sysThreadYield(void);
EXPORTED  int	 sysThreadSuspend(sys_thread_t *);
EXPORTED  int	 sysThreadResume(sys_thread_t *);
EXPORTED  int	 sysThreadSetPriority(sys_thread_t *, int32_t);
EXPORTED  int	 sysThreadGetPriority(sys_thread_t *, int32_t *);
EXPORTED  void * sysThreadStackPointer(sys_thread_t *); 
EXPORTED  stackp_t sysThreadStackBase(sys_thread_t *); 
EXPORTED  int	 sysThreadSingle(void);
EXPORTED  void	 sysThreadMulti(void);
EXPORTED  int    sysThreadEnumerateOver(int (*)(sys_thread_t *, void *), void *);
EXPORTED  void	 sysThreadInit(sys_thread_t *, stackp_t);
EXPORTED  void	 sysThreadSetBackPtr(sys_thread_t *, void *);
EXPORTED  void * sysThreadGetBackPtr(sys_thread_t *);
EXPORTED  int    sysThreadCheckStack(void);
EXPORTED  int    sysInterruptsPending(void);
EXPORTED  void	 sysThreadPostException(sys_thread_t *, void *);
JAVA_PUBLIC_API(void)	 sysThreadDumpInfo(sys_thread_t *, FILE*);

/*
 * System API for monitors
 */
EXPORTED int     sysMonitorSizeof(void);
EXPORTED int     sysMonitorInit(sys_mon_t *, bool_t);
EXPORTED int     sysMonitorEnter(sys_mon_t *);
EXPORTED bool_t  sysMonitorEntered(sys_mon_t *);
EXPORTED int     sysMonitorExit(sys_mon_t *);
EXPORTED int     sysMonitorNotify(sys_mon_t *);
EXPORTED int     sysMonitorNotifyAll(sys_mon_t *);
EXPORTED int 	 sysMonitorWait(sys_mon_t *, int);
EXPORTED int	 sysMonitorDestroy(sys_mon_t *, sys_thread_t *);
EXPORTED void    sysMonitorDumpInfo(sys_mon_t *);

#define SYS_OK	        0
#define SYS_TIMEOUT     1
#define SYS_ERR	       -1
#define SYS_INVALID    -2    /* invalid arguments */
#define SYS_NOTHREAD   -3    /* no such thread */
#define SYS_MINSTACK   -4    /* supplied stack size less than min stack size */
#define SYS_NOMEM      -5
#define SYS_NORESOURCE -6
#define SYS_DESTROY    -7

/*
 * System API for raw memory allocation
 */
EXPORTED void * sysMapMem(long, long *);
EXPORTED void * sysUnmapMem(void *, long, long *);
EXPORTED void * sysCommitMem(void *, long, long *);
EXPORTED void * sysUncommitMem(void *, long, long *);

/*
 * System API for termination
 */
EXPORTED void sysExit(int);
EXPORTED int sysAtexit(void (*func)(void));
EXPORTED void sysAbort(void);

/*
 * System API for filesystem access
 */
EXPORTED int sysOpen(const char *name, int flags, int mode);
EXPORTED long sysRead(int fd, void *buf, long amount);
EXPORTED long sysWrite(int fd, const void *buf, long amount);
EXPORTED long sysSeek(int fd, long offset, int whence);
EXPORTED void sysClose(int fd);

EXPORTED int sysAccess(const char *path, int mode);
struct stat;
EXPORTED int sysStat(const char *path, struct stat *buf);
EXPORTED int sysIsAbsolute(const char *path);
EXPORTED int sysMkdir(const char *path, int mode);
EXPORTED int sysUnlink(const char *path);

/*
 * Include platform specific macros to override these
 */
#include "sysmacros_md.h"
 
#endif /* !_SYS_API_H_ */
