#ifndef prdump_h___
#define prdump_h___

/*
** Internal API to NSPR dump utility routines
*/

#include "prthread.h"
#include "prmon.h"
#include <stdio.h>

NSPR_BEGIN_EXTERN_C

extern PR_PUBLIC_API(void) PR_DumpOneThread(PRThread *t);

extern PR_PUBLIC_API(void) PR_DumpThreads(void);

extern PR_PUBLIC_API(void) PR_DumpOneMonitor(PRMonitor *mon);

extern PR_PUBLIC_API(void) PR_DumpMonitors(void);

extern PR_PUBLIC_API(void) PR_DumpStuff(void);

extern PR_PUBLIC_API(void) PR_Monitor_print(FILE* file, PRMonitor* mon);

extern PR_PUBLIC_API(void) PR_Thread_print(FILE* file, PRThread* t);

extern PR_PUBLIC_API(void) PR_ThreadFun(char* funName, PRThread* thread, 
                                       PRMonitor* mon);

extern PR_PUBLIC_API(void) PR_DumpMonitorCache(void);

/* Formats a time to a buffer, returning the number of characters written: */
extern PR_PUBLIC_API(size_t)
PR_PrintMicrosecondsToBuf(char* buf, size_t buflen, int64 v);

extern PR_PUBLIC_API(void) PR_PrintMicroseconds(FILE* file, int64 v);

typedef void (*PRFileDumper)(FILE *out, PRBool detailed);

extern PR_PUBLIC_API(void)
PR_DumpToFile(char* filename, char* msg, PRFileDumper dump, PRBool detailed);

extern PR_PUBLIC_API(void) PR_DumpStuffToFile(void);

#ifdef DEBUG
/* This hook may be called by some architectures during segv. */
extern void (*PR_ShowStatusHook)(void);
#endif /* DEBUG */

#if defined(XP_UNIX)
#define _PR_DebugPrint    fprintf
#elif defined(XP_PC)
#if defined(_WIN32)
/*
** For the PC, DebugPrint writes to stdout which is caught by NSPR and
** redirected to an output window...
*/
int _PR_DebugPrint(FILE *f, const char *fmt, ...);
#else
#define _PR_DebugPrint    fprintf
#endif
#elif defined(XP_MAC)
// For now, send debug print output for the mac to stderr via fprintf 
#define _PR_DebugPrint    fprintf
#else
#error need _PR_DebugPrint
#endif

NSPR_END_EXTERN_C

#endif /* prdump_h___ */
