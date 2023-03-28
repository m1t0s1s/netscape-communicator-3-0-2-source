#ifndef mozjava_h___
#define mozjava_h___

#if defined(NSPR) || defined(JAVA)
#include <nspr/prthread.h>
#include <nspr/prmon.h>
#include <nspr/prfile.h>
#include <nspr/prlog.h>
#include <nspr/prglobal.h>
#include <nspr/prevent.h>
#include "java.h"
#endif

#ifdef JAVA /*  jwz */
extern PRMonitor *fdset_lock;
extern PRMonitor *pr_asyncIO;
extern PRThread *mozilla_thread;

#define LOCK_FDSET() PR_EnterMonitor(fdset_lock)
#define UNLOCK_FDSET() PR_ExitMonitor(fdset_lock)

#else
#define LOCK_FDSET()
#define UNLOCK_FDSET()
#endif

#endif /* mozjava_h___ */
