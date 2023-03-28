/* -*- Mode: C; tab-width: 4 -*- */

#include "xp_error.h"

PUBLIC int xp_errno = 0;

/* Get rid of macro definition! */
#undef XP_SetError

/*
** Provide a procedural implementation in case people mess up their makefiles
*/
void XP_SetError(int v)
{
    xp_errno = v;
}
