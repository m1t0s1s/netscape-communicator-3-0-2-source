/*
 * @(#)monitor_cache.c	1.30 95/09/17
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*
 * Code to manage the monitor cache.
 *
 * The monitor cache is a hash table which contains monitors.  The
 * hash function hashes an arbitrary value (typically the address of
 * an object) to a single monitor structure.  The monitor cache and
 * its hash table grow as necessary, and are only limited by available
 * memory.  The hash table is grown when the total number of monitors
 * is HASHTABLE_MULTIPLE times the number of hash table buckets, i.e.
 * when the average hash chain length with all monitors in use would
 * be HASHTABLE_MULTIPLE.
 *
 * Once the cache has expanded, it is never contracted, although doing
 * so would be possible.
 */

#include <javaThreads.h>
#include <monitor.h>
#include <monitor_cache.h>
#include <log.h>

#include <sys_api.h>

/*
 * The following defines should preserve for initialization to work
 *	INIT_MONITOR_COUNT <= EXPANSION_QUANTUM * INIT_MONITOR_COUNT
 */
#define INIT_HASHTABLE_BUCKETS	32 /* Must be a power of two! */
#define INIT_MONITOR_COUNT	32 /* Initial number of monitors */
#define EXPANSION_QUANTUM	16 /* Number of monitors to expand by */
#define HASHTABLE_MULTIPLE	2  /* Length of avg hash chain before expand */
#define HASH_MASK		(monHashTableBuckets - 1)
#define HASH(k)			(((k >> 2) ^ (k >> 10)) & HASH_MASK)

/*
 * When the number of free monitors falls below FREE_MONITOR_THRESHOLD we
 * expand.  This is needed because some platforms need monitors in the
 * expansion code (e.g. Green threads on Solaris does a malloc which
 * may use at least two monitors).  Because the monitor cache is in
 * shared code, this number has to be conservative.
 */
#define FREE_MONITOR_THRESHOLD 5

/*
 * The hash table itself and its size.  We can't dynamically allocate the
 * initial hash table because we would need monitors to do so.  If the
 * table is eventually expanded, we just lose the initial memory.
 */
static monitor_t *initMonHashTable[INIT_HASHTABLE_BUCKETS] = {0};
monitor_t **monHashTable = &initMonHashTable[0];
int monHashTableBuckets = INIT_HASHTABLE_BUCKETS;

/*
 * The total number of monitors in the monitor cache
 */
static int monCount = 0;

/*
 * The monitor cache free list and count of free monitors.  We use
 * the count to determine when we have fewer than FREE_MONITOR_THRESHOLD
 * monitors free, in which case we expand the cache.  We can't wait
 * for the free list to be empty before we expand, as some platforms
 * need monitors to expand (e.g. malloc on Solaris may use as many
 * as two monitors).  We need to overestimate here because this code
 * and FREE_MONITOR_THRESHOLD are shared across all platforms. 
 */
static monitor_t *monFreeList = 0;
static int monFreeCount = 0;

/*
 * This is a simple last used monitor cache, based on the assumption
 * that a single monitor is used repeatedly before another monitor is
 * used.
 */
static monitor_t *lastMonUsed = 0;

#ifdef DEBUG
static int monLookups = 0;
static int monQuickFind = 0;
static int monHashFind = 0;
static int monCreate = 0;
#endif

/*
 * The registered monitor controlling cache access
 */
sys_mon_t *_moncache_lock;

/*
 * Add count monitors to the front of the monitor free list.  This
 * doesn't assume that the monFreeList is empty, and in fact it
 * probably isn't (see comment below when expandMonCache() is
 * called from createMonitor()).  This is called with the monitor
 * cache *unlocked*.
 *
 * There is a crucial and nonobvious connection between the delayed
 * increment of monFreeCount in expandMonCache() and the threshold
 * test in createMonitor().  Once we have started to expand, we want
 * to to get other threads wanting monitors into the createMonitor()
 * code that causes them to wait until expansion finishes.  This
 * avoids nasty problems like grabbing monitors in the unprotected
 * region of createMonitor() between adding new monitors to the
 * cache and expanding the hash table.  By postponing resetting
 * monFreeCount until after all expansion is done, we guarantee that
 * all threads creating monitors will be caught by the single, fast
 * threshold test.  Once inside, further tests decide what to do
 * with them, but the crucial normal case (no expansion) remains fast.
 */
static void
expandMonCache(int count)
{
    int nbytes = count * (sizeof(monitor_t) + sysMonitorSizeof());
    monitor_t *mon;
    monitor_t *start;
    int i;

    if ((mon = (monitor_t *) sysMalloc(nbytes)) == 0) {
	out_of_memory();
    }

    Log2(2, "Expanding monitor cache by %d monitors to %d\n",
	 count, monCount+count);

    start = mon;
    memset((char *) mon, 0, nbytes);
    CACHE_LOCK();
    monCount += count;
    /* monFreeCount += count; Postponed!  See comment above */
    i = count;
    while (--i > 0) {
	monitor_t *next = (monitor_t *)
	    ((char *)mon + (sizeof(monitor_t) + sysMonitorSizeof()));
	monitorInit(mon);
	mon->next = next;
	mon = next;
    }
    monitorInit(mon);
    mon->next = monFreeList;
    monFreeList = start;

    /*
     * If after expansion the monCount is more than HASHTABLE_MULTIPLE
     * times greater than the hash table size, expand the hash table as
     * well.  That is the same as saying that we expand when the average
     * hash chain with all monitors in use is HASHTABLE_MULTIPLE long.
     */
    if (monCount <= HASHTABLE_MULTIPLE * monHashTableBuckets) {
	monFreeCount += count; 		/* Increment when done expanding */
	CACHE_UNLOCK();
    } else {
	/* Expand the hash table: size must be a multiple of two */
	int hash, i;
	int oldMonHashTableBuckets = monHashTableBuckets;
	monitor_t **oldMonHashTable = monHashTable;
	monitor_t **newMonHashTable;
	monitor_t **hashp;

	Log2(2, "Expanding monitor cache hash table from %d to %d buckets\n",
	     monHashTableBuckets, 2*monHashTableBuckets);
	nbytes = 2 * monHashTableBuckets * sizeof(monitor_t *);
	/*
	 * Remember that malloc may use a monitor, so needs to have
	 * a consistent monHashTable and monHashTableBuckets.  After
	 * we reset monHashTable and monHashTableBuckets we need to
	 * be sure not to enter any cache monitors before we've
	 * rehashed everything.
	 */
        CACHE_UNLOCK();
	if ((newMonHashTable = (monitor_t **) sysMalloc(nbytes)) == 0) {
	    out_of_memory();
	}
	CACHE_LOCK();
	monHashTable = newMonHashTable;
	monHashTableBuckets *= 2;
	memset((char *) monHashTable, 0, nbytes);
	for (i = 0; i < oldMonHashTableBuckets; i++) {
	    mon = oldMonHashTable[i];
	    while (mon != 0) {
		monitor_t *next = mon->next;
		hash = HASH(mon->key);	/* must have new size set here */
		hashp = &monHashTable[hash];
		mon->next = *hashp;
		*hashp = mon;
		mon = next;
	    }
	}
	monFreeCount += count; 		/* Increment when done expanding */
	CACHE_UNLOCK();
        if (oldMonHashTable != &initMonHashTable[0]) {
            sysFree(oldMonHashTable);
        }
    }
}


/* Initialize the monitor cache
 * 
 * Called during bootstrapping, so should be no chance of interruption.
 *
 * We need a sys_-level monitor initialization step.  We don't need
 * a sys_-level monitor destruction, as we never really destroy a
 * cache monitor -- they are just left in the table to be used again.
 */

bool_t  monitorsInitialized = FALSE;

void
monitorCacheInit()
{
    _moncache_lock = (sys_mon_t *) sysMalloc(sysMonitorSizeof());
    memset((char *) _moncache_lock, 0, sysMonitorSizeof());
    CACHE_LOCK_INIT();

    expandMonCache(INIT_MONITOR_COUNT);
    monitorsInitialized = TRUE;
}

/*
 * Look up a monitor in the hash table.  Return MID_NULL if it is not
 * there.  Assumes we have control of the monitor cache mutex.
 */
monitor_t *
lookupMonitor(unsigned int key)
{
    monitor_t *mon, *ret;
    int	hash;

    CACHE_LOCK();
    
#ifdef DEBUG
    monLookups += 1;
#endif

    if ((mon = lastMonUsed) != 0 && mon->key == key) {
	sysAssert((mon->flags & MON_IN_USE) != 0);
#ifdef DEBUG
	monQuickFind += 1;
#endif
	ret = mon;
	goto unlock;
    }

    hash = HASH(key);
    mon = monHashTable[hash];
    while (mon != 0) {
	/* All monitors in hash table are in use */
	sysAssert((mon->flags & MON_IN_USE) != 0);
	if (mon->key == key) {
#ifdef DEBUG
	    monHashFind += 1;
#endif
	    ret = (lastMonUsed = mon);
	    goto unlock;
	}
	mon = mon->next;
    }
    ret = MID_NULL;
unlock:
    CACHE_UNLOCK();
    return ret;
}


/*
 * Create a monitor for the specified key.  If it already exists,
 * just return it.
 */
monitor_t *
createMonitor(unsigned int key)
{
    int	hash;
    monitor_t **hashp, *mon;
    monitor_t *ret;

    CACHE_LOCK();

#ifdef DEBUG
    monLookups += 1;
#endif

RetryCreate:
    if ((mon = lastMonUsed) != 0 && mon->key == key) {
	sysAssert((mon->flags & MON_IN_USE) != 0);
#ifdef DEBUG
	monQuickFind += 1;
#endif
	ret = mon;
	goto unlock;
    }

    /* First look for the monitor, in case it's in use */
    hash = HASH(key);
    mon = monHashTable[hash];
    while (mon != 0) {
	/* All monitors in hash table are in use */
	sysAssert((mon->flags & MON_IN_USE) != 0);
	if (mon->key == key) {
#ifdef DEBUG
	    monHashFind += 1;
#endif
	    ret = (lastMonUsed = mon);
	    goto unlock;
	}
	mon = mon->next;
    }

    /*
     * If we get here, a monitor for key has not been allocated.  Now
     * we grab one from the free list, and add it to the hash table.  If
     * handing over the next monitor pushes us below FREE_MONITOR_THRESHOLD
     * free monitors, expand the monitor cache *first*.  On systems that
     * don't use native threads, like on Solaris with Green threads, 
     * sysMalloc() is probably made thread safe using a monitor from the
     * monitor cache, and in order to expand the free list, we need to
     * call sysMalloc().  So we make sure we have some small number of
     * monitors available when we expand.  We also have to play some
     * tricks to avoid deadlock between the malloc and monitor cache
     * monitors.
     */
    {   static sys_thread_t *expandingFreeList = 0;
	if (monFreeCount < FREE_MONITOR_THRESHOLD) {
	    if (!expandingFreeList) {
		expandingFreeList = sysThreadSelf();
		CACHE_UNLOCK();
		expandMonCache(EXPANSION_QUANTUM);
		CACHE_LOCK();
		expandingFreeList = 0;
    		CACHE_NOTIFY();   /* Wake up anyone waiting for expansion */
		goto RetryCreate;
	    } else if (expandingFreeList != sysThreadSelf()) {
		/* Somebody is already expanding the free list, so wait */
		while (expandingFreeList) {
		    CACHE_WAIT();
		}
		/*
		 * Someone might have tried to create a monitor but was
		 * forced to wait because someone else was expanding the
		 * cache.  During that time the expander might have created
		 * a monitor for the waiter's key, so when the waiter wakes
		 * up he needs to start over at the top.
		 */
		goto RetryCreate;
	    }   /*
	         * Else *I* am expanding the list, so let me continue using
	         * monitors out of the FREE_MONITOR_THRESHOLD buffer.
	         */
	}
    }

    sysAssert(monFreeList != 0);
    /* We don't want to recreate anything that was created by expansion: */
    sysAssert(lookupMonitor(key) == MID_NULL);

#ifdef DEBUG
    monCreate += 1;
#endif
    mon = monFreeList;
    monFreeList = mon->next;
    monFreeCount--;
    sysAssert((mon->flags & MON_IN_USE) == 0);

    mon->key = key;
    mon->flags |= MON_IN_USE;
    hashp = &monHashTable[hash];
    mon->next = *hashp;
    *hashp = mon;
    /* REMIND: Why don't we put the new monitor in lastMonUsed??? */
    ret = mon;

unlock:
    ret->uninit_count++;
    CACHE_UNLOCK();
    return ret;
}

/*
 * Destroy the specified monitor.  Note that it is not actually reclaimed,
 * just taken out of the hash table of the monitor cache.
 */
void
monitorDestroy(monitor_t *mon, unsigned int key)
{
    int	hash;
    monitor_t **hp;

    sysAssert(CACHE_LOCKED());

    if (mon->uninit_count != 0) {
	return;
    }

    sysAssert(mon == lookupMonitor(key));
    sysAssert(mon != 0);

    mon->flags = 0;
    mon->key = 0;
    hash = HASH(key);
    for (hp = &monHashTable[hash]; *hp != 0; hp = &((*hp)->next)) {
	if (*hp == mon) {
	    *(hp) = mon->next;
	    break;
	}
    }

    if (mon == lastMonUsed) {
	lastMonUsed = 0;
    }
    mon->next = monFreeList;
    monFreeList = mon;
    monFreeCount++;

    /*
     * Here we tell anyone who is waiting for a free monitor that a
     * new one is available.
     */
    CACHE_NOTIFY();

    return;
}

void
monitorEnumerate(void (*fcn)(monitor_t *, void *), void *cookie)
{
    monitor_t **hashtable = monHashTable;
    monitor_t *mon, *next;
    int	i;

    CACHE_LOCK();
    for (i = monHashTableBuckets; --i >= 0; ) {
	mon = hashtable[i];
	while (mon != 0) {
	    next = mon->next;
	    fcn(mon, cookie);
	    mon = next;
	}
    }
    CACHE_UNLOCK();
}

#ifdef DEBUG

void DumpMonCacheInfo()
{
    int i;
    monitor_t *mon;

    fprintf(stderr, "Monitor cache info:\n");
    fprintf(stderr, "  Initial monitor count: %d\n", INIT_MONITOR_COUNT);    
    fprintf(stderr, "  Initial hash table buckets: %d\n",
	    INIT_HASHTABLE_BUCKETS);  
    fprintf(stderr, "  Hash table expansion multiple: %d\n", HASHTABLE_MULTIPLE);
    fprintf(stderr, "  Minimum number of free monitors before expansion: %d\n",
	    FREE_MONITOR_THRESHOLD);    
    fprintf(stderr, "  Number of new monitors added per expansion: %d\n",
	    EXPANSION_QUANTUM);    
    fprintf(stderr, "  Current total number of monitors: %d\n", monCount);
    fprintf(stderr, "  Current number of free monitors: %d\n", monFreeCount);
    fprintf(stderr, "  Current number of hash table buckets (power of 2!): %d\n",
	    monHashTableBuckets);

    fprintf(stderr, "  Current hash table bucket dump:\n");
    for (i = 0; i < monHashTableBuckets; i++) {
	mon = monHashTable[i];
	if (mon != 0) {
	    fprintf(stderr, "    Bucket %d: ", i);
	    while (mon != 0) {
	        fprintf(stderr, "0x%lx ", mon);
	        mon = mon->next;
	    }
	    fprintf(stderr, "\n");
	}
    }
}

#endif
