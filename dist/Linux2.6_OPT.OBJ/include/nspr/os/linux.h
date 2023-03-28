#ifndef nspr_linux_defs_h___
#define nspr_linux_defs_h___

#define PR_LINKER_ARCH	"linux"

#define _MD_GC_VMBASE		0x40000000
#define _MD_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	65536L

#define USE_SETJMP

#ifdef SW_THREADS
#include <setjmp.h>

#define PR_GetSP(_t) (_t)->context[0].__jmpbuf[JB_SP]

#define PR_NUM_GCREGS	6
#define PR_CONTEXT_TYPE	sigjmp_buf

#define CONTEXT(_th) ((_th)->context)

/*
** Initialize a thread context to run "e(o,a)" when started
*/
#define _MD_INIT_CONTEXT(_thread, e, o, a)	  \
{						  \
    (_thread)->asyncCall = e;			  \
    (_thread)->asyncArg0 = o;			  \
    (_thread)->asyncArg1 = a;			  \
    (_thread)->context[0].__jmpbuf[JB_BP] = 0;		  \
    (_thread)->context[0].__jmpbuf[JB_SP] = (unsigned char*) \
	((_thread)->stack->stackTop - 64);	  \
    (_thread)->context[0].__jmpbuf[JB_PC] = HopToadNoArgs;	  \
}

#define _MD_SWITCH_CONTEXT(_thread)  \
    if (!sigsetjmp(CONTEXT(_thread), 1)) { \
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
    siglongjmp(CONTEXT(_thread), 1);    \
}

#endif /* SW_THREADS */

#undef	HAVE_LONG_LONG
#undef	HAVE_ALIGNED_DOUBLES
#undef	HAVE_ALIGNED_LONGLONGS

/*
 * Elf linux supports dl* functions
 */
#ifdef LINUX1_2
#define	HAVE_DLL
#define	USE_DLFCN
#else
#undef	HAVE_DLL
#undef	USE_DLFCN
#endif

#define NEED_TIME_R

#endif /* nspr_linux_defs_h___ */
