#ifndef nspr_ncr_defs_h___
#define nspr_ncr_defs_h___

#define PR_LINKER_ARCH	"ncr"

#define _MD_GC_VMBASE		0x40000000
#define _MD_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	65536L

#ifdef SW_THREADS
#include <setjmp.h>

#if 1
#include <signal.h>
#endif

#define PR_CONTEXT_TYPE         sigjmp_buf
#define PR_GetSP(_t)            ((_t)->context)[26]
#define PR_NUM_GCREGS	_SIGJBLEN

#define CONTEXT(_th) ((_th)->context)

#define _MD_INIT_CONTEXT(_thread, e, o, a)			  \
{								  \
    if (sigsetjmp(CONTEXT(_thread),1)) {				  \
	HopToadNoArgs();					  \
    }								  \
    (_thread)->asyncCall = e;					  \
    (_thread)->asyncArg0 = o;					  \
    (_thread)->asyncArg1 = a;					  \
    PR_GetSP(_thread) = (int) ((_thread)->stack->stackTop - 64); \
}

#if 1
#define _MD_SWITCH_CONTEXT(_thread)  \
    if (!sigsetjmp(CONTEXT(_thread),1)) { \
	(_thread)->errcode = errno;  \
	_PR_Schedule();		     \
    }
#else
#define _MD_SWITCH_CONTEXT(_thread)  \
    if (!sigsetjmp(CONTEXT(_thread),1)) { \
        sigset_t set, oset; \
        unsigned int mask; \
        extern FILE* real_stderr; \
        mask = getsigmask(); \
        sigfillset(&set); \
        sigprocmask(SIG_BLOCK, &set, &oset); \
        fprintf(real_stderr, "after switch-> %X\n", mask); \
        sigprocmask(SIG_SETMASK, &oset, NULL); \
	(_thread)->errcode = errno;  \
	_PR_Schedule();		     \
    }
#endif

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread) \
{				     \
    _pr_current_thread = _thread;  \
    errno = (_thread)->errcode;	     \
    siglongjmp(CONTEXT(_thread), 1);    \
}

#endif /* SW_THREADS */

#undef	HAVE_LONG_LONG
#undef	HAVE_ALIGNED_DOUBLES
#undef	HAVE_ALIGNED_LONGLONGS
#undef	HAVE_DLL
#undef	USE_DLFCN

#define NEED_TIME_R
/* No DLL capability */

#endif /* nspr_ncr_defs_h___ */
