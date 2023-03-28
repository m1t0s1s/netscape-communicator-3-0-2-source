/*-----------------------------------------------------------------------------
	Position String Formatting
	%ns		argument n is a string
	%ni		argument n is an integer
	%%		literal %
	
	n must be from 1 to 9
	
	XP_MessageLen returns the length of the formatted message, including 
	the terminating NULL. 
	
	XP_Message formats the message into the given buffer, not exceeding
	bufferLen (which includes the terminating NULL). If there isn't enough 
	space, XP_Message will truncate the text and terminate it (unlike
	strncpy, which will truncate but not terminate).
	
	XP_StaticMessage is like XP_Message but maintains a private buffer 
	which it resizes as necessary.
-----------------------------------------------------------------------------*/

#ifndef _XP_Message_
#define _XP_Message_

#include "xp_core.h"

XP_BEGIN_PROTOS

int
XP_MessageLen (const char * format, ...);

void
XP_Message (char * buffer, int bufferLen, const char * format, ...);

const char *
XP_StaticMessage (const char * format, ...);

XP_END_PROTOS

#endif /* _XP_Message_ */

