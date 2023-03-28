/*
 * Locking and queue management primatives
 *
 */
#include "seccomon.h"
#include "prlock.h"
#include "prmon.h"
#include "secmod.h"
#include "secmodi.h"
#include "prlong.h"

#define ISREADING 1
#define ISWRITING 2
#define WANTWRITE 4
#define ISLOCKED 3

/*
 * create a new lock for a Module List
 */
SECMODListLock *SECMOD_NewListLock() {
    SECMODListLock *modLock;

    modLock = PORT_Alloc(sizeof(SECMODListLock));
#ifdef PKCS11_USE_THREADS
    modLock->mutex = PR_NewLock(0);
    modLock->monitor = PR_NewMonitor();
#else
    modLock->mutex = NULL;
    modLock->monitor = NULL;
#endif
    modLock->state = 0;
    modLock->count = 0;
    return modLock;
}

/*
 * destroy the lock
 */
void SECMOD_DestroyListLock(SECMODListLock *lock) {
    PK11_USE_THREADS(PR_DestroyMonitor(lock->monitor);)
    PK11_USE_THREADS(PR_DestroyLock(lock->mutex);)
    PORT_Free(lock);
}


/*
 * Lock the List for Read: NOTE: this assumes the reading isn't so common
 * the writing will be starved.
 */
void SECMOD_GetReadLock(SECMODListLock *modLock) {
#ifdef PKCS11_USE_THREADS
    /* Sigh need NSPR 2.0  for this 
    PR_Lock(modLock->mutex);
     * use this instead */
    PR_EnterMonitor(modLock->monitor);
    while (modLock->state & ISWRITING) {
	PR_Wait(modLock->monitor, LL_ZERO); /* wait until woken up */
    }
    modLock->state |= ISREADING;
    modLock->count++;
    /*PR_UnLock(modLock->mutex);*/
    PR_ExitMonitor(modLock->monitor);
#endif
}

/*
 * Release the Read lock
 */
void SECMOD_ReleaseReadLock(SECMODListLock *modLock) {
#ifdef PKCS11_USE_THREADS
    /*PR_Lock(modLock->mutex);*/
    PR_EnterMonitor(modLock->monitor);
    modLock->count--;
    if (modLock->count == 0) {
	modLock->state &= ~ISREADING;
	if (modLock->state & WANTWRITE) {
	    PR_Notify(modLock->monitor);  /* only one writer at a time */
	}
    }
    /*PR_UnLock(modLock->mutex);*/
    PR_ExitMonitor(modLock->monitor);
#endif
}


/*
 * lock the list for Write
 */
void SECMOD_GetWriteLock(SECMODListLock *modLock) {
#ifdef PKCS11_USE_THREADS
    /*PR_Lock(modLock->mutex);*/
    PR_EnterMonitor(modLock->monitor);
    while (modLock->state & ISLOCKED) {
	modLock->state |= WANTWRITE;
	PR_Wait(modLock->monitor, LL_ZERO); /* wait until woken up */
    }
    modLock->state = ISWRITING;
    /*PR_UnLock(modLock->mutex);*/
    PR_ExitMonitor(modLock->monitor);
#endif
}


/*
 * Release the Write Lock: NOTE, this code is pretty inefficient if you have
 * lots of write collisions.
 */
void SECMOD_ReleaseWriteLock(SECMODListLock *modLock) {
#ifdef PKCS11_USE_THREADS
    /*PR_Lock(modLock->mutex);*/
    PR_EnterMonitor(modLock->monitor);
    modLock->state = 0;
    PR_NotifyAll(modLock->monitor); /* enable all the readers */
    /*PR_UnLock(modLock->mutex);*/
    PR_ExitMonitor(modLock->monitor);
#endif
}


/*
 * must Hold the Write lock
 */
void
SECMOD_RemoveList(SECMODModuleList **parent, SECMODModuleList *child) {
    *parent = child->next;
    child->next = NULL;
}

/*
 * if lock is not specified, it must already be held
 */
void
SECMOD_AddList(SECMODModuleList *parent, SECMODModuleList *child, 
							SECMODListLock *lock) {
    if (lock) { SECMOD_GetWriteLock(lock); }

    child->next = parent->next;
    parent->next = child;

   if (lock) { SECMOD_ReleaseWriteLock(lock); }
}
