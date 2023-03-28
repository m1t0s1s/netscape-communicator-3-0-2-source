#ifndef nspr_locks_h___
#define nspr_locks_h___

#if defined(JAVA)
#include "prmon.h"

extern PRMonitor* libnet_asyncIO;

#define LIBNET_LOCK()		PR_EnterMonitor(libnet_asyncIO)
#define LIBNET_UNLOCK()		PR_ExitMonitor(libnet_asyncIO)
#define LIBNET_IS_LOCKED()	PR_InMonitor(libnet_asyncIO)

#else /* !JAVA */

#define LIBNET_LOCK()
#define LIBNET_UNLOCK()
#define LIBNET_IS_LOCKED() 1

#endif /* JAVA */

#endif /* nspr_locks_h___ */
