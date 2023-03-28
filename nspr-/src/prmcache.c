/*
** Monitor cache.
*/
#include "prmon.h"
#include "prlog.h"
#include "prexc.h"
#include "prtypes.h"
#include <stddef.h>

#ifdef XP_MAC
#include <stdlib.h>
#endif

#include <string.h>
#include <stdio.h>
#include "prprf.h"
#include "prdump.h"
/*#define MCACHE_DEBUG 1*/

PR_LOG_DEFINE(MONITOR);

/* Monitor used to lock the monitor cache */
PRMonitor *_pr_mcache_lock;
int _pr_mcache_ready;

typedef struct MonitorCacheEntryStr MonitorCacheEntry;

struct MonitorCacheEntryStr {
    MonitorCacheEntry*	next;
    void* 		address;
    PRMonitor*		mon;
    long		cacheEntryCount;
};

static uint hash_mask;
static uint num_hash_buckets;
static uint num_hash_buckets_log2;
static MonitorCacheEntry **hash_buckets;
static MonitorCacheEntry *free_entries;
static uint num_free_entries;
static int expanding;

#define HASH(address)				\
    ((uint) ( ((prword_t)(address) >> 2) ^	\
	    ((prword_t)(address) >> 10) )	\
     & hash_mask)

/*
** Expand the monitor cache. This grows the hash buckets and allocates a
** new chunk of cache entries and throws them on the free list. We keep
** as many hash buckets as there are entries.
**
** Because we call malloc and malloc may need the monitor cache, we must
** ensure that there are several free monitor cache entries available for
** malloc to get. FREE_THRESHOLD is used to prevent monitor cache
** starvation during monitor cache expansion.
*/

#define FREE_THRESHOLD	5

static int ExpandMonitorCache(uint new_size_log2)
{
    MonitorCacheEntry **new_hash_buckets, *new_entries;
    MonitorCacheEntry **old_hash_buckets, *p;
    uint i, entries, old_num_hash_buckets, added;

    entries = (uint)(1L << new_size_log2);

    /*
    ** Expand the monitor-cache-entry free list
    */
    new_entries = (MonitorCacheEntry*)
	malloc(entries * sizeof(MonitorCacheEntry));
    if (!new_entries) {
	/* Total lossage */
	PR_MarkException(PR_MONITOR_CACHE_FULL, newsize_log2);
	return -1;
    }

    /*
    ** Allocate system monitors for the new monitor cache entries. If we
    ** run out of system monitors, break out of the loop.
    */
    for (i = 0, added = 0, p = new_entries; i < entries; i++, p++, added++) {
	p->mon = PR_NewNamedMonitor(0, "mc-                 ");
	p->cacheEntryCount = 0;
	if (!p->mon) {
	    break;
	}
    }
    if (added != entries) {
	if (added == 0) {
	    /* Totally out of system monitors. Lossage abounds */
	    free(new_entries);
	    PR_MarkException(PR_MONITOR_CACHE_FULL, newsize_log2);
	    return -1;
	}

	/*
	** We were able to allocate some of the system monitors. Use
	** realloc to shrink down the new_entries memory
	*/
	p = realloc(new_entries, added * sizeof(MonitorCacheEntry));
	if (p == 0) {
	    /*
	    ** Total lossage. We just leaked a bunch of system monitors
	    ** all over the floor. This should never ever happen.
	    */
	    PR_ASSERT(p != 0);
	    PR_MarkException(PR_MONITOR_CACHE_FULL, 0);
	    return -1;
	}
    }

    /*
    ** Now that we have allocated all of the system monitors, build up
    ** the new free list. We can just update the free_list because we own
    ** the mcache-lock and we aren't calling anyone who might want to use
    ** it.
    */
    for (i = 0, p = new_entries; i < added - 1; i++, p++) {
	p->next = p + 1;
    }
    p->next = free_entries;
    free_entries = new_entries;
    num_free_entries += added;
	
    /* Try to expand the hash table */
    new_hash_buckets = (MonitorCacheEntry**)
	calloc(entries, sizeof(MonitorCacheEntry*));
    if (!new_hash_buckets) {
	/*
	** Partial lossage. In this situation we don't get any more hash
	** buckets, which just means that the table lookups will take
	** longer. This is bad, but not fatal
	*/
	PR_LOG(MONITOR, warn,
	       ("unable to grow monitor cache hash buckets"));
	return 0;
    }

    /*
    ** Compute new hash mask value. This value is used to mask an address
    ** until it's bits are in the right spot for indexing into the hash
    ** table.
    */
    hash_mask = entries - 1;

    /*
    ** Expand the hash table. We have to rehash everything in the old
    ** table into the new table.
    */
    old_hash_buckets = hash_buckets;
    old_num_hash_buckets = num_hash_buckets;
    for (i = 0; i < old_num_hash_buckets; i++) {
	p = old_hash_buckets[i];
	while (p) {
	    MonitorCacheEntry *next = p->next;

	    /* Hash based on new table size, and then put p in the new table */
	    uint hash = HASH(p->address);
	    p->next = new_hash_buckets[hash];
	    new_hash_buckets[hash] = p;

	    p = next;
	}
    }

    /*
    ** Switch over to new hash table and THEN call free of the old
    ** table. Since free might re-enter _pr_mcache_lock, things would
    ** break terribly if it used the old hash table.
    */
    hash_buckets = new_hash_buckets;
    num_hash_buckets = entries;
    num_hash_buckets_log2 = new_size_log2;
    free(old_hash_buckets);

    PR_LOG(MONITOR, warn,
	   ("expanded monitor cache to %d (buckets %d)",
	    num_free_entries, entries));

    return 0;
}

/*
** Lookup a monitor cache entry by address. Return a pointer to the
** pointer to the monitor cache entry on success, null on failure.
*/
static MonitorCacheEntry **LookupMonitorCacheEntry(void *address)
{
    uint hash;
    MonitorCacheEntry **pp, *p;

    hash = HASH(address);
    pp = hash_buckets + hash;
    while ((p = *pp) != 0) {
	if (p->address == address) {
	    if (p->cacheEntryCount > 0)
		return pp;
	    else
		return NULL;
	}
	pp = &p->next;
    }
/*    PR_MarkException(PR_CACHED_MONITOR_NOT_FOUND, address);*/
    return 0;
}

/*
** Lookup a monitor by address to the object being locked. Return a
** pointer to the system monitor on success, null on failure.
*/
static PRMonitor *FindCachedMonitor(void *address)
{
    uint hash;
    MonitorCacheEntry **pp, *p;

    hash = HASH(address);
    pp = hash_buckets + hash;
    while ((p = *pp) != 0) {
	if (p->address == address) {
	    if (p->cacheEntryCount > 0)
		return p->mon;
	    else
		return NULL;
	}
	pp = &p->next;
    }
/*    PR_MarkException(PR_CACHED_MONITOR_NOT_FOUND, address);*/
    return NULL;
}

/*
** Try to create a new cached monitor. If it's already in the cache,
** great - return it. Otherwise get a new free cache entry and set it
** up. If the cache free space is getting low, expand the cache.
*/
static PRMonitor *CreateMonitor(void *address)
{
    uint hash;
    MonitorCacheEntry **pp, *p;

    hash = HASH(address);
    pp = hash_buckets + hash;
    while ((p = *pp) != 0) {
	if (p->address == address) {
	    goto gotit;
	}
	pp = &p->next;
    }

    /* Expand the monitor cache if we have run out of free slots in the table */
    if (num_free_entries < FREE_THRESHOLD) {
	/* Expand monitor cache */
	if (!expanding) {
	    int rv;

	    expanding = 1;
	    rv = ExpandMonitorCache(num_hash_buckets_log2 + 1);
	    expanding = 0;
	    if (rv < 0) {
	    /* Uh oh */
	    return 0;
	}

	    /* redo the hash because it'll be different now */
	    hash = HASH(address);
	} else {
	    /*
	    ** We are in process of expanding and we need a cache
	    ** monitor.  Make sure we have enough!
	    */
	    PR_ASSERT(num_free_entries > 0);
	}
    }

    /* Make a new monitor */
    p = free_entries;
    free_entries = p->next;
    num_free_entries--;
    p->address = address;
    p->next = hash_buckets[hash];
    hash_buckets[hash] = p;
    PR_ASSERT(p->cacheEntryCount == 0);

  gotit:
#ifdef DEBUG
    /* Overlay name area with latest name */
    PR_snprintf(p->mon->name, 16, "mc-%x", address);
#endif
    p->cacheEntryCount++;

    /*
    ** Reset stickyCount for the monitor because it is conceptually a new
    ** monitor.
    */
    p->mon->stickyCount = 0;

    return p->mon;
}

void _PR_InitMonitorCache(void)
{
    _pr_mcache_lock = PR_NewNamedMonitor(0, "monitor-cache-lock");
    ExpandMonitorCache(5);
    _pr_mcache_ready = 1;
}

void
pr_DumpMonitorCache(FILE* out)
{
    MonitorCacheEntry **pp, *p;
    uint i;
    _PR_DebugPrint(out, "\n--Monitor-Cache----------------------------------------------------------------");
    for (i = 0; i < num_hash_buckets; i++) {
	pp = hash_buckets + i;
	while ((p = *pp) != 0) {
	    if (p->mon->owner ||
		!PR_CLIST_IS_EMPTY(&p->mon->condQ) ||
		!PR_CLIST_IS_EMPTY(&p->mon->lockQ)) {
		_PR_DebugPrint(out, "\n  %0x -> ", p->address);
		PR_Monitor_print(out, p->mon);
		if (HASH(p->address) != i)
		    _PR_DebugPrint(out, " (Warning: monitor in bucket %d but should be in bucket %d)", i, HASH(p->address));
	    }
	    pp = &p->next;
	}
    }
    _PR_DebugPrint(out, "\n-------------------------------------------------------------------------------\n");
}

PR_PUBLIC_API(void)
PR_DumpMonitorCache(void)
{
    pr_DumpMonitorCache(stderr);
}

/******************************************************************************/
#ifdef MCACHE_DEBUG

#include <stdio.h>
#include "prthread.h"

struct { void* addr; PRMonitor* mon; PRThread* cur; } monStack[100];
int monStackTop = 0;

void printMonStack() {
    int i;
    fprintf(stderr, "In use:\n");
    for (i=0; i<monStackTop; i++) {
	fprintf(stderr, "%d\taddr=%x, hash=%3d, mon=%x, thr=%x\n",
		i, monStack[i].addr, HASH(monStack[i].addr),
		monStack[i].mon, monStack[i].cur);
    }
    fprintf(stderr, "Cache:\n");
    for (i=0; i<num_hash_buckets; i++) {
	MonitorCacheEntry **pp, *p;
	fprintf(stderr, "\tbucket %d:\n", i);
	pp = &hash_buckets[i];
	while ((p = *pp) != 0) {
	    fprintf(stderr, "\t\taddr=%x, hash=%3d, mon=%x\n", p->address,
		    HASH(p->address), p->mon);
	    pp = &p->next;
	}
    }
}

#endif /* MCACHE_DEBUG */

/******************************************************************************/

#if defined( XP_PC ) && defined( _WIN32 )
#define LOCK_MCACHE() EnterCriticalSection(&_pr_mcache_lock->mutexHandle)
#define UNLOCK_MCACHE() LeaveCriticalSection(&_pr_mcache_lock->mutexHandle)
#else
#define LOCK_MCACHE() PR_EnterMonitor(_pr_mcache_lock)
#define UNLOCK_MCACHE() PR_ExitMonitor(_pr_mcache_lock)
#endif

PR_PUBLIC_API(int) PR_CEnterMonitor(void *address)
{
    PRMonitor *mon;

    /*
    ** Create monitor for address. Don't enter the monitor while we have
    ** the mcache locked because we might block!
    */
    LOCK_MCACHE();
    mon = CreateMonitor(address);
#ifdef MCACHE_DEBUG
    monStack[monStackTop].addr = address;
    monStack[monStackTop].mon = mon;
    monStack[monStackTop++].cur = PR_CurrentThread();
    PR_LOG_BEGIN(MONITOR, debug, 
		 ("CEnterMonitor addr=%x, mon=%x thread=%x (%s)\n",
		  address, mon, PR_CurrentThread(), PR_CurrentThread()->name));
#endif /* MCACHE_DEBUG */
    UNLOCK_MCACHE();
    if (!mon) {
	return -1;
    }
    PR_EnterMonitor(mon);
    return 0;
}

#if defined(XP_UNIX)
/* Just like PR_CEnterMonitor except we return the underlying monitor */
PRMonitor *_PR_CGetMonitor(void *address)
{
    PRMonitor *mon;

    /*
    ** Create monitor for address. Don't enter the monitor while we have
    ** the mcache locked because we might block!
    */
    LOCK_MCACHE();
    mon = CreateMonitor(address);
#ifdef MCACHE_DEBUG
    monStack[monStackTop].addr = address;
    monStack[monStackTop].mon = mon;
    monStack[monStackTop++].cur = PR_CurrentThread();
    PR_LOG(MONITOR, debug, 
           ("CEnterMonitor addr=%x, mon=%x thread=%x (%s)\n",
	    address, mon, PR_CurrentThread(), PR_CurrentThread()->name));
#endif /* MCACHE_DEBUG */
    UNLOCK_MCACHE();
    if (!mon) {
	return 0;
    }
    PR_EnterMonitor(mon);
    return mon;
}
#endif

PR_PUBLIC_API(int) PR_CExitMonitor(void *address)
{
    int rv;
    MonitorCacheEntry **pp, *p;

    LOCK_MCACHE();
    pp = LookupMonitorCacheEntry(address);
    if (!pp) {
	/* whoops */
/*	PR_ASSERT(pp != 0);*/
	rv = -1;
	goto unlock;
    }

    p = *pp;
    p->cacheEntryCount--;
    PR_ASSERT((p != 0) && (p->mon != 0));
#ifdef MCACHE_DEBUG
    PR_LOG_END(MONITOR, debug, 
	       ("CExitMonitor addr=%x, mon=%x (%s) thread=%x (%s)\n",
		address, p->mon, p->mon->owner->name, PR_CurrentThread(),
		PR_CurrentThread()->name));
    {
	int i;
	for (i = 0; i < monStackTop; i++) {
	    if (monStack[i].addr == address) {
		int j;
		for (j = i; j < monStackTop; j++) {
		    monStack[j] = monStack[j+1];
		}
		break;
	    }
	}
	monStackTop--;
    }
#endif /* MCACHE_DEBUG */
    rv = PR_ExitMonitor(p->mon);
    if (p->cacheEntryCount == 0) {
#ifdef MCACHE_DEBUG
	int count = 0, i;
	for (i = 0; i < monStackTop; i++) {
	    if (monStack[i].addr == address) count++;
	}
	if (count > 1) {
	    PR_LOG(MONITOR, error, 
		   ("!!! freeing monitor %x but it's still on the stack %d times.\n",
		    p->mon, count));
	}
	PR_LOG(MONITOR, debug, 
	       ("CExitMonitor removing from cache: addr=%x, mon=%x thread=%x (%s)\n",
		address, p->mon, PR_CurrentThread(), PR_CurrentThread()->name));
#endif /* MCACHE_DEBUG */
	/*
	** Nobody is using the system monitor. Put it on the cached free
	** list. We are safe from somebody trying to use it because we
	** have the mcache locked.
	*/
	*pp = p->next;				/* unlink from hash_buckets */
	p->next = free_entries;			/* link into free list */
	free_entries = p;
	num_free_entries++;			/* count it as free */
    }

  unlock:
    UNLOCK_MCACHE();
    return rv;
}

PR_PUBLIC_API(int) PR_CWait(void *address, int64 howlong)
{
    PRMonitor *mon;

    LOCK_MCACHE();
    mon = FindCachedMonitor(address);
    UNLOCK_MCACHE();

    return mon ? PR_Wait(mon, howlong) : -1;
}

PR_PUBLIC_API(int) PR_CNotify(void *address)
{
    PRMonitor *mon;

    LOCK_MCACHE();
    mon = FindCachedMonitor(address);
    UNLOCK_MCACHE();

    return mon ? PR_Notify(mon) : -1;
}

PR_PUBLIC_API(int) PR_CNotifyAll(void *address)
{
    PRMonitor *mon;

    LOCK_MCACHE();
    mon = FindCachedMonitor(address);
    UNLOCK_MCACHE();

    return mon ? PR_NotifyAll(mon) : -1;
}
