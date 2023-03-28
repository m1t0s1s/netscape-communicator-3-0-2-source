/*-----------------------------------------------------------------------------
	Private Typesdefs, Globals, Etc
-----------------------------------------------------------------------------*/

#if 1
# define VA_START_UGLINESS
#endif

#include <stdarg.h>

#ifndef _XP_Message_Private_
#define _XP_Message_Private_

#define MessageArgsMax 9

typedef enum { matString, matInteger, matLiteral }  xpm_ArgType;
typedef struct xpm_Args_ xpm_Args;

typedef long xpm_Integer;

#if !defined(DEBUG) && (defined(__cplusplus) || defined(__gcc))
# ifndef INLINE
#  define INLINE inline
# endif
#else
# define INLINE static
#endif

struct xpm_Args_ {
	int				sizes [MessageArgsMax];
	xpm_ArgType		types [MessageArgsMax];
#ifdef VA_START_UGLINESS
	va_list 		stack;
#endif
	const char ** 	start;
	int				max;
};

/*-----------------------------------------------------------------------------
	streams interface to processing format strings
-----------------------------------------------------------------------------*/

typedef struct OutputStream_ OutputStream;

typedef void 
WriteLiteral (OutputStream * o, char c);

typedef void
WriteArgument (OutputStream * o, char c, int argument);

struct OutputStream_ {
	WriteLiteral *	writeLiteral;
	WriteArgument*	writeArgument;
	xpm_Args *		args;
};

#endif /* _XP_Message_Private_ */

