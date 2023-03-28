#ifndef nspr_aix_defs_h___
#define nspr_aix_defs_h___

#define PR_LINKER_ARCH	"aix"

#define _MD_GC_VMBASE		0x40000000L
#define _MD_STACK_VMBASE	0x50000000L
#define _MD_DEFAULT_STACK_SIZE	65536L

#define USE_SETJMP

#ifdef SW_THREADS

#include <setjmp.h>

#define PR_GetSP(_t)    (_t)->context[3]

#define PR_NUM_GCREGS	_JBLEN
#define PR_CONTEXT_TYPE	jmp_buf

#define CONTEXT(_th) ((_th)->context)

#define _MD_INIT_CONTEXT(_thread, e, o, a)			  \
{								  \
    if (setjmp(CONTEXT(_thread))) {				  \
	HopToadNoArgs();					  \
    }								  \
    (_thread)->asyncCall = e;					  \
    (_thread)->asyncArg0 = o;					  \
    (_thread)->asyncArg1 = a;					  \
    thread->context[3] = (int) ((_thread)->stack->stackTop - 64); \
}

#define _MD_SWITCH_CONTEXT(_thread)  \
    if (!setjmp(CONTEXT(_thread))) { \
	(_thread)->errcode = errno;  \
	_PR_Schedule();		     \
    }

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread) \
{				     \
    _pr_current_thread = _thread;    \
    PR_LOG(SCHED, warn, ("Scheduled")); \
    errno = (_thread)->errcode;	     \
    longjmp(CONTEXT(_thread), 1);    \
}

#endif /* SW_THREADS */

#define NEED_TIME_R

#undef  HAVE_STACK_GROWING_UP
/*
 *    IBM's xlC (version 2) doesn't like "long long", even though cc does!
 *    According to IBM propoganda this is fixed in version 3. For now,
 *    just build the whole editor with the no long long scheme.
 */
#ifdef EDITOR
#undef  HAVE_LONG_LONG
#else
#define	HAVE_LONG_LONG
#endif  /*__cplusplus*/
#undef	HAVE_ALIGNED_DOUBLES
#undef	HAVE_ALIGNED_LONGLONGS
#undef	HAVE_WEAK_IO_SYMBOLS
#undef	HAVE_WEAK_MALLOC_SYMBOLS
#undef	HAVE_DLL
#undef	USE_DLFCN

#endif /* nspr_aix_defs_h___ */
