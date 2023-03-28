/*
 * secport.c - portability interfaces for security libraries
 *
 * This file abstracts out libc functionality that libsec depends on
 * 
 * NOTE - These are not public interfaces
 *
 * $Id: secport.c,v 1.7.2.1 1997/04/10 08:00:10 jwz Exp $
 */

#include "secport.h"
#include "xp_mem.h"
#include "xp_time.h"
#include "prarena.h"

/* Still need old #define's */
#include "ds.h"

extern int SEC_ERROR_NO_MEMORY;

void *
PORT_Alloc(size_t bytes)
{
    void *rv;

    /* Always allocate a non-zero amount of bytes */
    rv = XP_ALLOC(bytes ? bytes : 1);
    if (!rv) {
	XP_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void *
PORT_Realloc(void *oldptr, size_t bytes)
{
    void *rv;

    rv = XP_REALLOC(oldptr, bytes);
    if (!rv) {
	XP_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void *
PORT_ZAlloc(size_t bytes)
{
    void *rv;

    /* Always allocate a non-zero amount of bytes */
    rv = XP_CALLOC(1, bytes ? bytes : 1);
    if (!rv) {
	XP_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void
PORT_Free(void *ptr)
{
    if (ptr) {
	XP_FREE(ptr);
    }
}

void
PORT_ZFree(void *ptr, size_t len)
{
    if (ptr) {
	XP_MEMSET(ptr, 0, len);
	XP_FREE(ptr);
    }
}

void *
PORT_AllocBlock(size_t bytes)
{
    void * rv;

    /* Always allocate a non-zero amount of bytes */
    rv = XP_ALLOC_BLOCK(bytes ? bytes : 1);
    if (!rv) {
	XP_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void *
PORT_ReallocBlock(void *block, size_t newBytes)
{
    void * rv;

    rv = (void *) XP_REALLOC_BLOCK(block, newBytes);
    if (!rv) {
	XP_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void
PORT_FreeBlock(void * block)
{
    if (block) {
	XP_FREE_BLOCK(block);
    }
}

time_t
PORT_Time(void)
{
    return(XP_TIME());
}

void
PORT_SetError(int value)
{	
    XP_SetError(value);
    return;
}

int
PORT_GetError(void)
{
    return(XP_GetError());
}


PRArenaPool *
PORT_NewArena(unsigned long chunksize)
{
    PRArenaPool *arena;
    
    arena = PORT_ZAlloc(sizeof(PRArenaPool));
    if ( arena != NULL ) {
	PR_InitArenaPool(arena, "security", chunksize, sizeof(double));
    }

    return(arena);
}

void *
PORT_ArenaAlloc(PRArenaPool *arena, size_t size)
{
    void *p;

    PR_ARENA_ALLOCATE(p, arena, size);
    if (p == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    return(p);
}

void *
PORT_ArenaZAlloc(PRArenaPool *arena, size_t size)
{
    void *p;

    PR_ARENA_ALLOCATE(p, arena, size);
    if (p == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    } else {
	PORT_Memset(p, 0, size);
    }

    return(p);
}

/* XXX - need to zeroize!! - jsw */
void
PORT_FreeArena(PRArenaPool *arena, PRBool zero)
{
    PR_FinishArenaPool(arena);
    PORT_Free(arena);
}

void *
PORT_ArenaGrow(PRArenaPool *arena, void *ptr, size_t oldsize, size_t newsize)
{
    PORT_Assert(newsize >= oldsize);
    
    PR_ARENA_GROW(ptr, arena, oldsize, ( newsize - oldsize ) );
    
    return(ptr);
}

void *
PORT_ArenaMark(PRArenaPool *arena)
{
    return PR_ARENA_MARK(arena);
}

void
PORT_ArenaRelease(PRArenaPool *arena, void *mark)
{
    PR_ARENA_RELEASE(arena, mark);
}

char *
PORT_ArenaStrdup(PRArenaPool *arena,char *str) {
    int len = PORT_Strlen(str)+1;
    char *new;

    new = PORT_ArenaAlloc(arena,len);
    if (new) {
        PORT_Memcpy(new,str,len);
    }
    return new;
}

