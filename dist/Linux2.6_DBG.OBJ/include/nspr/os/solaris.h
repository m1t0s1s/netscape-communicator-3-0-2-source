#ifndef nspr_solaris_defs_h___
#define nspr_solaris_defs_h___

#define PR_LINKER_ARCH		"solaris"

#define _MD_GC_VMBASE		0x40000000
#define _MD_STACK_VMBASE	0x50000000
#define _MD_DEFAULT_STACK_SIZE	(2*65536L)

#ifdef SW_THREADS
#include <ucontext.h>
#include <sys/stack.h>

#define CONTEXT(_thread)	(&(_thread)->context)

#ifdef sparc

#define PR_CONTEXT_TYPE		ucontext_t
#define PR_GetSP(_t)		(_t)->context.uc_mcontext.gregs[REG_SP]

/*
** Sparc's use register windows. the _MD_GetRegisters for the sparc's
** doesn't actually store anything into the argument buffer; instead the
** register windows are homed to the stack. I assume that the stack
** always has room for the registers to spill to...
*/
#define PR_NUM_GCREGS		0

/*
** Initialize the thread context preparing it to execute "e(o,a)". This
** is done the hard way because makecontext doesn't work in solaris...
*/
#define _MD_INIT_CONTEXT(thread, e, o, a)				       \
{									       \
    ucontext_t *uc = CONTEXT(thread);					       \
    unsigned char *sp;							       \
									       \
    /* Force sp to be double aligned! */				       \
    sp = (unsigned char*) thread->stack->stackTop - WINDOWSIZE - SA(MINFRAME); \
    sp = (unsigned char *)((unsigned long)(sp) & 0xfffffff8);		       \
    uc->uc_stack.ss_sp = (char *) sp;					       \
    uc->uc_stack.ss_size = thread->stack->stackSize;			       \
    uc->uc_stack.ss_flags = 0; /* ? */					       \
									       \
    uc->uc_mcontext.gregs[REG_SP] = (unsigned int) sp;			       \
    uc->uc_mcontext.gregs[REG_PC] = (unsigned int) HopToadNoArgs;	       \
    uc->uc_mcontext.gregs[REG_nPC] = (unsigned int) ((char*)HopToadNoArgs)+4;  \
									       \
    thread->asyncCall = e;						       \
    thread->asyncArg0 = o;						       \
    thread->asyncArg1 = a;						       \
									       \
    uc->uc_flags = UC_ALL;						       \
}

#define _MD_SWITCH_CONTEXT(_thread)	 \
    if (!getcontext(CONTEXT(_thread))) { \
	(_thread)->errcode = errno;	 \
	_PR_Schedule();			 \
    }

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread)   \
{				       \
    ucontext_t *uc = CONTEXT(_thread); \
    uc->uc_mcontext.gregs[11] = 1;     \
    _pr_current_thread = _thread;      \
    PR_LOG(SCHED, warn, ("Scheduled")); \
    errno = (_thread)->errcode;	       \
    setcontext(uc);		       \
}

#else  /* x86 solaris */

#define PR_NUM_GCREGS		_JBLEN

#define WINDOWSIZE		0
 
#define PR_CONTEXT_TYPE		unsigned int edi; sigset_t oldMask, blockMask; ucontext_t
#define PR_GetSP(_t)		(_t)->context.uc_mcontext.gregs[USP]
 
#include <signal.h>
 
int getedi(void);
void setedi(int);
 
#define _MD_INIT_CONTEXT(thread, e, o, a)					\
{										\
	ucontext_t *uc = CONTEXT(thread);					\
	unsigned char *sp;							\
	getcontext(uc);								\
	/* Force sp to be double aligned! */					\
	sp = (unsigned char*) thread->stack->stackTop - WINDOWSIZE - SA(MINFRAME);	\
	sp = (unsigned char *)((unsigned long)(sp) & 0xfffffff8);		\
	uc->uc_mcontext.gregs[USP] = (unsigned int) sp;				\
	uc->uc_mcontext.gregs[PC] = (unsigned int) HopToadNoArgs;		\
	thread->asyncCall = e;							\
	thread->asyncArg0 = o;							\
	thread->asyncArg1 = a;							\
	PR_LOG(THREAD, out, ("_MD_INIT_CONTEXT e = %x, o = %x, a = %x",e,o,a));	\
}

#define _MD_SWITCH_CONTEXT(contextp)						\
	PR_LOG(THREAD, out, ("_MD_SWITCH_CONTEXT"));				\
	sigfillset(&((contextp)->blockMask));					\
	sigprocmask(SIG_BLOCK, (const sigset_t *)&((contextp)->blockMask),	\
		&((contextp)->oldMask));					\
	(contextp)->edi = getedi();						\
	if (! getcontext(&((contextp)->context))) {				\
		sigprocmask(SIG_SETMASK, (const sigset_t *)&((contextp)->oldMask), NULL);	\
		((contextp)->context).uc_mcontext.gregs[EDI] = (contextp)->edi;	\
		(contextp)->errcode = errno;					\
		_PR_Schedule();							\
	} else {								\
		sigprocmask(SIG_SETMASK, &((contextp)->oldMask), NULL);		\
		setedi((contextp)->edi);					\
	}

/*
** Restore a thread context, saved by _MD_SWITCH_CONTEXT
*/
#define _MD_RESTORE_CONTEXT(_thread)						\
{										\
	ucontext_t *uc = CONTEXT(_thread);					\
	PR_LOG(THREAD, out, ("_MD_RESTORE_CONTEXT"));				\
	uc->uc_mcontext.gregs[EAX] = 1;						\
        _pr_current_thread = _thread;                                           \
        PR_LOG(SCHED, warn, ("Scheduled"));                                     \
	errno = (_thread)->errcode;						\
	setcontext(uc);								\
}

#endif /* sparc */

#endif /* SW_THREADS */

#undef  HAVE_STACK_GROWING_UP
#define	HAVE_LONG_LONG
#define	HAVE_ALIGNED_DOUBLES
#define	HAVE_ALIGNED_LONGLONGS
#undef	HAVE_WEAK_IO_SYMBOLS
#undef	HAVE_WEAK_MALLOC_SYMBOLS
#define	HAVE_DLL
#define	USE_DLFCN

/*
** Missing function prototypes
*/
extern int gethostname (char *name, int namelen);

#endif /* nspr_solaris_defs_h___ */
