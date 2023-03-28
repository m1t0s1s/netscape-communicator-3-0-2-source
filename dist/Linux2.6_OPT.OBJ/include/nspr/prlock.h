#ifndef prlock_h___
#define prlock_h___

#include "prmacros.h"
#include "prclist.h"

NSPR_BEGIN_EXTERN_C

/*
** Lightweight mutual exclusion locks
** XXX not yet implemented...
*/
typedef struct PRLockStr PRLock;

extern PR_PUBLIC_API(PRLock *)PR_NewLock(int locked);

extern PR_PUBLIC_API(void) PR_InitLock(PRLock *lock, int locked);

extern PR_PUBLIC_API(void) PR_DestroyLock(PRLock *lock);

extern PR_PUBLIC_API(void) PR_Lock(PRLock *lock);

extern PR_PUBLIC_API(void) PR_Unlock(PRLock *lock);

/************************************************************************/

struct PRLockStr {
    PRCList waitQ;
    PRThread *owner;
};

NSPR_END_EXTERN_C

#endif /* prlock_h___ */
