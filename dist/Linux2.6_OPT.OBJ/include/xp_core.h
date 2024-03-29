/*-----------------------------------------------------------------------------
    xp_core.h
    Cross-Platform Core Types
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    Platform-specific defines
        
        XP_WIN              XP_IS_WIN       XP_WIN_ARG(X)
        XP_MAC              XP_IS_MAC         XP_MAC_ARG(X)
        XP_UNIX             XP_IS_UNIX        XP_UNIX_ARG(X)
        XP_CPLUSPLUS        XP_IS_CPLUSPLUS
        defined iff         always defined    defined to nothing
        on that platform    as 0 or 1         or X
        
        Also Bool, Int32, Int16, Int, Uint32, Uint16, Uint, and nil
        And TRUE, FALSE, ON, OFF, YES, NO
-----------------------------------------------------------------------------*/

#ifndef _XP_Core_
#define _XP_Core_

/* which system are we on, get the base macro defined */

#if defined(macintosh) || defined(__MWERKS__) || defined(applec)
#ifndef macintosh
#define macintosh 1
#endif
#endif

#if defined(__unix) || defined(unix) || defined(UNIX) || defined(XP_UNIX)
#ifndef unix
#define unix 1
#endif
#endif

#if !defined(macintosh) && !defined(_WINDOWS) && !defined(unix)
  /* #error xp library can't determine system type */
#endif

/* flush out all the system macros */

#ifdef macintosh
# define XP_MAC 1
# define XP_IS_MAC 1
# define XP_MAC_ARG(x) x
#else
# define XP_IS_MAC 0
# define XP_MAC_ARG(x)
#endif

#ifdef _WINDOWS
# define XP_WIN
# define XP_IS_WIN 1
# define XP_WIN_ARG(x) x
#if defined(_WIN32) || defined(WIN32)
# define XP_WIN32
#else
# define XP_WIN16
#endif
#else
# define XP_IS_WIN 0
# define XP_WIN_ARG(x)
#endif

#ifdef unix
# ifndef XP_UNIX
# define XP_UNIX
# endif
# define XP_IS_UNIX 1
# define XP_UNIX_ARG(x) x
#else
# define XP_IS_UNIX 0
# define XP_UNIX_ARG(x)
#endif

/* what language do we have? */

#if defined(__cplusplus)
# define XP_CPLUSPLUS
# define XP_IS_CPLUSPLUS 1
#else
# define XP_IS_CPLUSPLUS 0
#endif

#if defined(DEBUG) || !defined(XP_CPLUSPLUS)
#define XP_REQUIRES_FUNCTIONS
#endif

/*
	language macros
	
	If C++ code includes a prototype for a function *compiled* in C, it needs to be
	wrapped in extern "C" for the linking to work properly. On the Mac, all code is
	being compiled in C++ so this isn't necessary, and Unix compiles it all in C. So
	only Windows actually will make use of the defined macros.
*/

#if defined(XP_CPLUSPLUS)
# define XP_BEGIN_PROTOS extern "C" {
# define XP_END_PROTOS }
#else
# define XP_BEGIN_PROTOS
# define XP_END_PROTOS
#endif

/* simple common types */

#ifndef XP_MAC
#ifdef XP_WIN
#ifndef RESOURCE_STR
#include <nspr/prtypes.h>
#endif /* RESOURCE_STR */
#else /* XP_WIN */
#include <nspr/prtypes.h>
#endif /* XP_WIN */
#else /* XP_MAC */
#include "prtypes.h"
#endif /* XP_MAC */

#ifdef XP_MAC
#include <Types.h>
	typedef char BOOL;
    typedef char Bool;
    typedef char XP_Bool;
#elif defined(XP_WIN)
    typedef int Bool;
    typedef int XP_Bool;
#else
    typedef char Bool;
    typedef char XP_Bool;
#endif

#ifdef XP_WIN
#ifndef BOOL
#define BOOL Bool
#endif
#define MIN(a, b)     min((a), (b))
#endif

#if defined(XP_UNIX) && !defined(MIN)
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* disable the TRUE redefinition warning
 * TRUE is defined by windows.h in the MSVC
 * development environment, and creates a
 * nasty warning for every file.  The only
 * way to turn it off is to disable all
 * macro redefinition warnings or to not
 * define TRUE here
 */
#if !defined(TRUE) && !defined(XP_WIN)
#define TRUE !FALSE
#endif

#define YES 1
#define NO 0

#define ON 1
#define OFF 0

#ifndef nil
#define nil 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef TRACEMSG
#ifdef DEBUG
#define TRACEMSG(msg)  do { if(MKLib_trace_flag)  XP_Trace msg; } while (0)
#else
#define TRACEMSG(msg)  
#endif /* DEBUG */
#endif /* TRACEMSG */

#define _INT16
#define _UINT16
#define _INT32
#define _UINT32

typedef int         	intn;

/* function classifications */

#define PUBLIC
#define MODULE_PRIVATE
#define PRIVATE static

/* common typedefs */
typedef struct _XP_List XP_List;

/* standard system headers */

#include <assert.h>
#include <ctype.h>
#ifdef __sgi
# include <float.h>
#endif
#ifdef XP_UNIX
#include <stdarg.h>
#endif
#include <limits.h>
#include <locale.h>
#if defined(XP_WIN) && defined(XP_CPLUSPLUS) && defined(_MSC_VER) && _MSC_VER >= 1020
/* math.h under MSVC 4.2 needs C++ linkage when C++. */
extern "C++"    {
#include <math.h>
}
#else
#include <math.h>
#endif
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#endif /* _XP_Core_ */


