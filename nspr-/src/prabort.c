#include "prlog.h"
#include "prprf.h"
#include "prfile.h"

#if defined(XP_MAC) || defined(XP_UNIX)
#include <string.h>
#endif

#ifdef XP_MAC
#include <Types.h>
#include <unistd.h>
#include <fcntl.h>
#include <Memory.h>
#endif /* XP_MAC */

#ifdef SW_THREADS
#include "swkern.h"
#else
#include "hwkern.h"
#endif

/*
** Abort the system
*/
PR_PUBLIC_API(void) PR_Abort(void)
{
    PR_LOG_FLUSH();
    abort();
}

/*
** Assertion failure
*/
PR_PUBLIC_API(void) _PR_Assert(const char *ex, const char *file, int line)
{
    char buf[400];

    /* Stop all other threads from running and prepare to die */
#ifdef SW_THREADS
    (void) _PR_IntsOff();
#else
#endif

    PR_snprintf(buf, sizeof(buf),
		"Assertion failure: %s, in file \"%s\" line %d\n",
		ex, file, line);

#ifdef XP_UNIX
    _OS_WRITE(2, buf, strlen(buf));
#endif

#if defined(XP_PC) && defined(DEBUG)
    PR_DumpStuff();
    DebugBreak();
    MessageBox((HWND)NULL, buf, "NSPR Assertion", MB_OK);
#endif /* XP_PC && DEBUG */

#ifdef XP_MAC
    {
    	Str255		debugStr;
    	uint32		length;
    	
    	length = strlen(buf);
    	if (length > 255)
    		length = 255;
    	debugStr[0] = length;
    	BlockMoveData(buf, debugStr + 1, length);
    	DebugStr(debugStr);    
    }
#endif

    PR_Abort();
}
