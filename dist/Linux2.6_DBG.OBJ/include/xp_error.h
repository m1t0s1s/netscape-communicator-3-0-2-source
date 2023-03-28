#ifndef __XP_ERROR_h_
#define __XP_ERROR_h_


#include "xp_core.h"
#include <errno.h>


XP_BEGIN_PROTOS

extern int xp_errno;

/*
** Return the most recent set error code.
*/
#ifdef XP_WIN
#define XP_GetError() xp_errno
#else
#define XP_GetError() xp_errno
#endif

/*
** Set the error code
*/
#ifdef DEBUG
extern void XP_SetError(int value);
#else
#define XP_SetError(v) xp_errno = (v)
#endif


XP_END_PROTOS


#endif /* __XP_ERROR_h_ */
