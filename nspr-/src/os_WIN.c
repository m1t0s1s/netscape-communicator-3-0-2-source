#include <windows.h>
#include <prtypes.h>		/* defines prword_t which is needed by mdint.h */
#include <prthread.h>
#include "prlog.h"          /* PR_ASSERT()                                 */
#include <stddef.h>         /* defines size_t which is needed by mdint.h   */
#include "mdint.h"
#include <setjmp.h>
#include "prgc.h"
#include "gcint.h"

/*
** Home the GC registers for the given thread into it's thread
** context area.
*/
#ifdef SW_THREADS
prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
        _MD_SAVE_CONTEXT(t);
    }
#ifdef _WIN32
    *np = PR_NUM_GCREGS;
#else   /* !_WIN32 */
    /*
    ** In Win16 because the premption is "cooperative" it can never be the 
    ** case that a register holds the sole reference to an object.  It
    ** will always have been pushed onto the stack before the thread
    ** switch...  So don't bother to scan the registers...
    */
    *np = 0;
#endif  /* !_WIN32 */

    return (prword_t*) CONTEXT(t);
}
#else
prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np) 
{
    CONTEXT context;
    context.ContextFlags = CONTEXT_INTEGER;

    if (GetThreadContext(t->handle, &context)) {
        t->context[0] = context.Eax;
        t->context[1] = context.Ebx;
        t->context[2] = context.Ecx;
        t->context[3] = context.Edx;
        t->context[4] = context.Esi;
        t->context[5] = context.Edi;
        t->context[6] = context.Esp;
        t->context[7] = context.Ebp;
        *np = PR_NUM_GCREGS;
    } else {
        PR_ASSERT(0);/* XXX */
    }
    return (prword_t*) t->context;
}

#endif /* SW_THREADS */


#if !defined(_WIN32)
#include "swkern.h"

extern void
PRWin16ScanCStack(PRThread *t, prword_t *low, prword_t *high)
{
	prword_t scan;
	prword_t limit;
	
    void (*processRoot)(void GCPTR *p, int32 count);

    processRoot = _pr_gcData.processRoot;
    
    scan = (prword_t) low;
    limit = (prword_t) high;
	while (scan < limit) {
		prword_t *test;

		test = *((prword_t **)scan);
		
		if (GC_IN_HEAP(&_pr_gcData, test)) {
			PR_PROCESS_ROOT_LOG(("Thread %s: sp=%p", t->name, scan));
			(*processRoot)(test, 1);
		} 
		scan += sizeof(uint16);		
	}

}

extern PRThread *_pr_primary_thread;
static int OldPriorityOfPrimaryThread   = -1;
static int TimeSlicesOnNonPrimaryThread =  0;

/*
** Resume execution of the thread t.
*/
void _MD_RESTORE_CONTEXT(PRThread *t)
{
    /*
    ** These variables cannot be on the stack since they are used during the 
    ** stack swapping...
    */
    static char *pTopOfStack;
    static char *pThreadStack;
    static char *pTaskStack;

    /*	
    **	This is a good opportunity to make sure that the main
    **	mozilla thread actually gets some time.  If interrupts
    **	are on, then we know it is safe to check if the main
    **	thread is being starved.  If moz has not been scheduled
    **	for a long time, then then temporarily bump the fe priority 
    **	up so that it gets to run at least one. 
    */	
    if (_pr_primary_thread == t) {
        if (OldPriorityOfPrimaryThread != -1) {
            PR_SetThreadPriority(_pr_primary_thread, OldPriorityOfPrimaryThread);
            OldPriorityOfPrimaryThread = -1;
        }
        TimeSlicesOnNonPrimaryThread = 0;
    } else {
        TimeSlicesOnNonPrimaryThread++;
    }

    if ((TimeSlicesOnNonPrimaryThread >= 20) && (OldPriorityOfPrimaryThread == -1)) {
        OldPriorityOfPrimaryThread = PR_GetThreadPriority(_pr_primary_thread);
        PR_SetThreadPriority(_pr_primary_thread, 31);
        TimeSlicesOnNonPrimaryThread = 0;
    }

    /*
    ** Save the Task Stack into the "cached thread stack" of the current thread
    */
    pTaskStack   = _pr_top_of_task_stack;
    pTopOfStack  = (char *) &t;
    pThreadStack = (char *)_pr_current_thread->stack->stackTop;

    PR_ASSERT( pTaskStack >= pTopOfStack );

    /*
    ** Display the range of the current Task stack and the range
    ** of the cached stack that it is copied to...
    */

    while (pTaskStack >= pTopOfStack) {
        *pThreadStack-- = *pTaskStack--;
    }

    /* Save the amount of the stack that was copied... */
    _pr_current_thread->stack->SP = pThreadStack;

    /* Mark the new thread as the current thread */
    _pr_current_thread = t;

    /*
    ** Now copy the "cached thread stack" of the new thread into the Task Stack
    **
    ** REMEMBER:
    **    After the stack has been copied, ALL local variables in this function
    **    are invalid !!
    */
    pTaskStack   = _pr_top_of_task_stack;
    pTopOfStack  = t->stack->SP;
    pThreadStack = t->stack->stackTop;

    PR_ASSERT( pThreadStack >= pTopOfStack );

    while (pThreadStack >= pTopOfStack) {
        *pTaskStack-- = *pThreadStack--;
    }
    /* 
    ** IMPORTANT:
    ** ----------
    ** SS:SP is now invalid :-( This means that all local variables and
    ** function arguments are invalid and NO function calls can be
    ** made !!! We must fix up SS:SP so that function calls can safely
    ** be made...
    */

    __asm {
        mov     ax, WORD PTR [pTopOfStack + 2]
        mov     ss, ax
        mov     ax, WORD PTR [pTopOfStack]
        mov     sp, ax
    }

    PR_LOG(SCHED, warn, ("Scheduled"));

    errno = (_pr_current_thread)->errcode;
    Throw(CONTEXT(_pr_current_thread), 1);
}

#endif /* !_WIN32 */
