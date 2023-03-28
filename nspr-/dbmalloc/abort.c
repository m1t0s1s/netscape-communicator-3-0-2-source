/*
 * (c) Copyright 1990, 1991, 1992 Conor P. Cahill (cpcahil@virtech.vti.com)
 *
 * This software may be distributed freely as long as the following conditions
 * are met:
 * 		* the distribution, or any derivative thereof, may not be
 *		  included as part of a commercial product
 *		* full source code is provided including this copyright
 *		* there is no charge for the software itself (there may be
 *		  a minimal charge for the copying or distribution effort)
 *		* this copyright notice is not modified or removed from any
 *		  source file
 */
#include <stdio.h>
#include "mallocin.h"
#include "debug.h"

/*
 * Function:	malloc_abort()
 *
 * Purpose:	abort routine which cause a core dump.  This routine is 
 *		put here so that the user can override it with thier own
 *		abort routine. 
 *
 * Arguments:	none
 *
 * Returns:	nothing of any value
 *
 * Narrative:
 *		call abort routine to cause core dump
 */
#ifndef lint
static
char rcs_hdr[] = "$Id: abort.c,v 1.1 1996/06/18 03:29:12 warren Exp $";
#endif

VOIDTYPE
malloc_abort()
{

	VOIDCAST abort();

} /* malloc_abort(... */
