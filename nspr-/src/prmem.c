/*
** Thread safe versions of memory allocation routines
*/
#include "prmon.h"
#include "prmem.h"
#include <memory.h>
#include <string.h>

#define NSPR	/* XXX for now */
#ifdef NSPR /* Enough of this */
/*
** We only use these malloc wrappers on specific systems.
*/
#if (defined(XP_UNIX) && !defined(SOLARIS) && !defined(__linux) && !defined(PURIFY)) || defined(WIN32)

extern PRMonitor *_pr_heap_lock;

#define LOCK_HEAP() if (_pr_heap_lock) PR_EnterMonitor(_pr_heap_lock)
#define UNLOCK_HEAP() if (_pr_heap_lock) PR_ExitMonitor(_pr_heap_lock)

extern void *unlocked_malloc(size_t size);
extern void unlocked_free(void *ptr);
extern void *unlocked_realloc(void *ptr, size_t size);
extern void *unlocked_calloc(size_t n, size_t elsize);

/************************************************************************/

/*
** Thread safe versions of malloc, free, realloc, calloc and cfree.
*/
void *malloc(size_t size)
{
    void *p;

    LOCK_HEAP();
    p = unlocked_malloc(size);
    UNLOCK_HEAP();
    return p;
}

void free(void *ptr)
{
    LOCK_HEAP();
    unlocked_free(ptr);
    UNLOCK_HEAP();
}

void *realloc(void *ptr, size_t size)
{
    void *p;

    LOCK_HEAP();
    p = unlocked_realloc(ptr, size);
    UNLOCK_HEAP();
    return p;
}

void *calloc(size_t n, size_t elsize)
{
    void *p;

    LOCK_HEAP();
    p = unlocked_calloc(n, elsize);
    UNLOCK_HEAP();
    return p;
}

void cfree(void *p)
{
    LOCK_HEAP();
    unlocked_free(p);
    UNLOCK_HEAP();
}
#endif /* XP_UNIX */
#endif /* NSPR */
