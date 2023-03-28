#include "xpassert.h"
#include "xp_trace.h"

#ifdef __MWERKS__

#include <UDebugging.h>

#ifdef __cplusplus
extern "C" 
#endif
void
XP_Abort (char * message, ...)
{
#ifdef DEBUG
	if (message)
		XP_Trace (message);
	BreakToSourceDebugger_ ();
#endif
}

#endif

