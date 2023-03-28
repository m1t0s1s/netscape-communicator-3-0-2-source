#ifndef sun_java_monitor_md_h___
#define sun_java_monitor_md_h___

#include "nspr_md.h"
#include "prmon.h"
#include "threads_md.h"

#define SYS_TIMEOUT_INFINITY -1

/* XXX this makes GC synchronous! */
#define SYS_INTERRUPTS_PENDING() 0

extern int sysMonitorDestroy(sys_mon_t *, sys_thread_t *);

#endif /* sun_java_monitor_md_h___ */
