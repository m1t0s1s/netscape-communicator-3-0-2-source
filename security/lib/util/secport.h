/*
 * secport.h - portability interfaces for security libraries
 *
 * This file abstracts out libc functionality that libsec depends on
 * 
 * NOTE - These are not public interfaces
 *
 * $Id: secport.h,v 1.11.2.4 1997/04/05 04:10:18 jwz Exp $
 */

#ifndef _SECPORT_H_
#define _SECPORT_H_

/*
 * define XP_MAC, XP_WIN, or XP_UNIX, in case they are not defined
 * by anyone else
 */
#ifdef macintosh
# ifndef XP_MAC
# define XP_MAC 1
# endif
#endif

#ifdef _WINDOWS
# ifndef XP_WIN
# define XP_WIN
# endif
#if defined(_WIN32) || defined(WIN32)
# ifndef XP_WIN32
# define XP_WIN32
# endif
#else
# ifndef XP_WIN16
# define XP_WIN16
# endif
#endif
#endif

#ifdef unix
# ifndef XP_UNIX
# define XP_UNIX
# endif
#endif

#ifdef XP_MAC
#include <types.h>
#else
#include <sys/types.h>
#endif

#include <ctype.h>
#include <string.h>
#include "prarena.h"
#ifndef NSPR20
#include "prglobal.h"
#else
#include "prtypes.h"
#include "prlog.h"	/* for PR_ASSERT */
#endif /* NSPR20 */

#include "ds.h"

extern void *PORT_Alloc(size_t len);
extern void *PORT_Realloc(void *old, size_t len);
extern void *PORT_AllocBlock(size_t len);
extern void *PORT_ReallocBlock(void *old, size_t len);
extern void PORT_FreeBlock(void *ptr);
extern void *PORT_ZAlloc(size_t len);
extern void PORT_Free(void *ptr);
extern void PORT_ZFree(void *ptr, size_t len);
extern time_t PORT_Time(void);
extern void PORT_SetError(int value);
extern int PORT_GetError(void);

extern PRArenaPool *PORT_NewArena(unsigned long chunksize);
extern void *PORT_ArenaAlloc(PRArenaPool *arena, size_t size);
extern void *PORT_ArenaZAlloc(PRArenaPool *arena, size_t size);
extern void PORT_FreeArena(PRArenaPool *arena, PRBool zero);
extern void *PORT_ArenaGrow(PRArenaPool *arena, void *ptr,
			    size_t oldsize, size_t newsize);
extern void *PORT_ArenaMark(PRArenaPool *arena);
extern void PORT_ArenaRelease(PRArenaPool *arena, void *mark);
extern char *PORT_ArenaStrdup(PRArenaPool *arena, char *str);

#define PORT_Assert PR_ASSERT

/* Please, keep these defines sorted alphbetically.  Thanks! */

#ifdef XP_STRING_FUNCS

#define PORT_Atoi 	XP_ATOI

#define PORT_Memcmp 	XP_MEMCMP
#define PORT_Memcpy 	XP_MEMCPY
#define PORT_Memmove 	XP_MEMMOVE
#define PORT_Memset 	XP_MEMSET

#define PORT_Strcasecmp XP_STRCASECMP
#define PORT_Strcat 	XP_STRCAT
#define PORT_Strchr 	XP_STRCHR
#define PORT_Strcmp 	XP_STRCMP
#define PORT_Strcpy 	XP_STRCPY
#define PORT_Strdup 	XP_STRDUP
#define PORT_Strlen(s) 	XP_STRLEN(s)
#define PORT_Strncasecmp XP_STRNCASECMP
#define PORT_Strncat 	strncat
#define PORT_Strncmp 	XP_STRNCMP
#define PORT_Strncpy 	strncpy
#define PORT_Strstr 	XP_STRSTR
#define PORT_Strtok 	XP_STRTOK_R

#define PORT_Tolower 	XP_TO_LOWER

#else /* XP_STRING_FUNCS */

#define PORT_Atoi 	atoi

#define PORT_Memcmp 	memcmp
#define PORT_Memcpy 	memcpy
#define PORT_Memmove 	memmove
#define PORT_Memset 	memset

#define PORT_Strcasecmp strcasecomp
#define PORT_Strcat 	strcat
#define PORT_Strchr 	strchr
#define PORT_Strcmp 	strcmp
#define PORT_Strcpy 	strcpy
#define PORT_Strdup 	strdup
#define PORT_Strlen(s) 	strlen(s)
#define PORT_Strncasecmp strncasecomp
#define PORT_Strncat 	strncat
#define PORT_Strncmp 	strncmp
#define PORT_Strncpy 	strncpy
#define PORT_Strstr 	strstr
#define PORT_Strtok 	strtok

#define PORT_Tolower 	tolower

#endif /* XP_STRING_FUNCS */

#endif /* _SECPORT_H_ */
