/*-----------------------------------------------------------------------------
	XPUtil.h
	Cross-Platform Debugging
	
	These routines are NOT for handling expected error conditions! They are
	for detecting *program logic* errors. Error conditions (such as running
	out of memory) cannot be predicted at compile-time and must be handled
	gracefully at run-time.
-----------------------------------------------------------------------------*/
#ifndef _XPDebug_
#define _XPDebug_

#include "xp_core.h"
#include "xpassert.h"
#include "xp_trace.h"

/*-----------------------------------------------------------------------------
DEBUG (macro)
-----------------------------------------------------------------------------*/

#ifdef DEBUG
	extern int Debug;
#else
#define Debug 0
#endif

#endif /* _XPDebug_ */

