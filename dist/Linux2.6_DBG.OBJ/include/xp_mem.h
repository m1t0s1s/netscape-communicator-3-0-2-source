/*-----------------------------------------------------------------------------
    XPMem.h
    Cross-Platform Memory API
-----------------------------------------------------------------------------*/
#ifndef _XP_MEM_
#define _XP_MEM_

#include "xp_core.h"

#ifdef XP_MAC
#include "macmem.h"
#endif

#ifdef XP_WIN16
#include <malloc.h>
#endif

/*-----------------------------------------------------------------------------
Allocating Structures
-----------------------------------------------------------------------------*/

#ifndef XP_MAC

#define XP_NEW( x )       	(x*)malloc( sizeof( x ) )
#define XP_DELETE( p )  	free( p )

#else /* XP_MAC */

#define XP_NEW( s ) 			((s*)Flush_Allocate( sizeof(s), FALSE ) )
#define XP_DELETE( p )			Flush_Free( p )

#endif /* XP_MAC */

/*-----------------------------------------------------------------------------
Mallocs
NOTE: this uses the same malloc as the structure allocator so it is
ok and safe to use XP_DELETE or XP_FREE interchangeably!
-----------------------------------------------------------------------------*/

#ifdef XP_MAC

#define XP_ALLOC( s )			Flush_Allocate( s, FALSE )
#define XP_FREE( p )			Flush_Free( p )
#define XP_REALLOC( p , s )		Flush_Reallocate( p, s )
#define XP_CALLOC( n, s )		Flush_Allocate( (n)*(s), TRUE )
#define XP_NEW_ZAP( t )			((t*)Flush_Allocate( sizeof(t), TRUE ) )

#else /* !XP_MAC */
/* normal win and unix */

#ifdef XP_WIN16

XP_BEGIN_PROTOS
extern void * WIN16_realloc(void * ptr, unsigned long size);
extern void * WIN16_malloc(unsigned long size);
XP_END_PROTOS

#define XP_REALLOC(ptr, size)   WIN16_realloc(ptr, size)
#define XP_ALLOC(size)          WIN16_malloc(size)
#else

#if defined(DEBUG) && defined(MOZILLA_CLIENT)
/* Check that we never allocate anything greater than 64K.  If we ever tried,
   Win16 would choke, and we'd like to find out about it on some other platform
   (like, one where we have a working debugger). */
/* This code used to call abort. Unfortunately, on Windows, abort() doesn't
 * go to the debugger. Instead, it silently quits the program.
 * So use XP_ASSERT(FALSE) instead.
 */

#define XP_CHECK_ALLOC_SIZE(size)	((size) < 64000 ? size : (XP_ASSERT(FALSE), (size)))
#else
#define XP_CHECK_ALLOC_SIZE(size)	size
#endif

#define XP_REALLOC(ptr, size)   realloc(ptr, XP_CHECK_ALLOC_SIZE(size))
#define XP_ALLOC(size)          malloc(XP_CHECK_ALLOC_SIZE(size))
#endif


#ifdef DEBUG
#define XP_CALLOC(num, sz)	(((num)*(sz))<64000 ? calloc((num),(sz)) : (XP_ASSERT(FALSE), calloc((num),(sz))))
#else
#define XP_CALLOC(num, sz)      calloc((num), (sz))
#endif

#define XP_FREE(ptr)            free(ptr)
#define XP_NEW_ZAP(TYPE)        ( (TYPE*) calloc (1, sizeof (TYPE) ) )
#endif /* !XP_MAC */


/* --------------------------------------------------------------------------
  16-bit windows requires space allocated bigger than 32K to be of
  type huge.  For example:

  int HUGE * foo = halloc(100000);
-----------------------------------------------------------------------------*/

/* There's no huge realloc because win16 doesn't have a hrealloc,
 * and there's no API to discover the original buffer's size.
 */
#ifdef XP_WIN16
#define XP_HUGE __huge
#define XP_HUGE_ALLOC(SIZE) halloc(SIZE,1)
#define XP_HUGE_FREE(SIZE) hfree(SIZE)
#define XP_HUGE_MEMCPY(DEST, SOURCE, LEN) hmemcpy(DEST, SOURCE, LEN)
#else
#define XP_HUGE
#define XP_HUGE_ALLOC(SIZE) malloc(SIZE)
#define XP_HUGE_FREE(SIZE) free(SIZE)
#define XP_HUGE_MEMCPY(DEST, SOURCE, LEN) memcpy(DEST, SOURCE, LEN)
#endif

#define XP_HUGE_CHAR_PTR char XP_HUGE *

/*-----------------------------------------------------------------------------
Allocating Large Buffers
NOTE: this does not interchange with XP_ALLOC/XP_NEW/XP_FREE/XP_DELETE
-----------------------------------------------------------------------------*/

#if defined(XP_UNIX) || defined(XP_WIN32) 

/* don't typedef this to void* unless you want obscure bugs... */
typedef unsigned long *	XP_Block;

#define XP_ALLOC_BLOCK(SIZE)            malloc ((SIZE))
#define XP_FREE_BLOCK(BLOCK)            free ((BLOCK))
#ifdef XP_UNIXu
  /* On SunOS, realloc(0,n) ==> 0 */
# define XP_REALLOC_BLOCK(BLOCK,SIZE)    ((BLOCK) \
					  ? realloc ((BLOCK), (SIZE)) \
					  : malloc ((SIZE)))
#else /* !XP_UNIX */
# define XP_REALLOC_BLOCK(BLOCK,SIZE)    realloc ((BLOCK), (SIZE))
#endif /* !XP_UNIX */
#define XP_LOCK_BLOCK(PTR,TYPE,BLOCK)   PTR = ((TYPE) (BLOCK))
#ifdef DEBUG
#define XP_UNLOCK_BLOCK(BLOCK)          (void)BLOCK
#else
#define XP_UNLOCK_BLOCK(BLOCK)
#endif
#endif /* XP_UNIX || XP_WIN32 */

#ifdef XP_WIN16
typedef unsigned char * XP_Block;
#define XP_ALLOC_BLOCK(SIZE)            WIN16_malloc((SIZE))
#define XP_FREE_BLOCK(BLOCK)            free ((BLOCK))
#define XP_REALLOC_BLOCK(BLOCK,SIZE)    ((BLOCK) \
					  ? WIN16_realloc ((BLOCK), (SIZE)) \
					  : WIN16_malloc ((SIZE)))
#define XP_LOCK_BLOCK(PTR,TYPE,BLOCK)   PTR = ((TYPE) (BLOCK))
#ifdef DEBUG
#define XP_UNLOCK_BLOCK(BLOCK)          (void)BLOCK
#else
#define XP_UNLOCK_BLOCK(BLOCK)
#endif

#endif /* XP_WIN16 */


#ifdef XP_MAC

typedef float*						XP_Block;
#define XP_ALLOC_BLOCK( s )			((XP_Block)Flush_Allocate( s, FALSE ) )
#define XP_FREE_BLOCK( b )			Flush_Free( b )
#define	XP_REALLOC_BLOCK( b, s )	((XP_Block)Flush_Reallocate( b, s ) )
#define XP_LOCK_BLOCK( p, t, b )	(p = ( t )( b ))
#define	XP_UNLOCK_BLOCK( b )

#endif /* XP_MAC */

#define	PA_Block					XP_Block
#define	PA_ALLOC(S)					XP_ALLOC_BLOCK(S)
#define PA_FREE(B)					XP_FREE_BLOCK(B)
#define PA_REALLOC(B,S)				XP_REALLOC_BLOCK(B,S)
#define PA_LOCK(P,T,B)				XP_LOCK_BLOCK(P,T,B)
#define PA_UNLOCK(B)				XP_UNLOCK_BLOCK(B)

/*-----------------------------------------------------------------------------
Allocating many small structures.

If allocating many small structures, it is often more efficient to allocate
an array of a bunch of them, and maintain a free list of them.  These
utilities do that for you.

You must provide a XP_AllocStructInfo structure which describes what
it is you are trying to allocate it.  If statically defined, use the
XP_INITIALIZE_ALLOCSTRUCTINFO macro to initialize it; if you prefer to
initialize it at runtime, use the XP_InitAllocStructInfo() routine.

If you free everything you've ever allocated for a given
XP_AllocStructInfo, all the memory used will be freed.  Or, if you're
*really sure* you're done with everything you've allocated for a given 
XP_AllocStructInfo, you can just call the scary XP_FreeAllStructs() routine.

Don't mix calls to XP_AllocStruct/XP_FreeStruct and XP_ALLOC/XP_FREE !!!

XP_AllocStructZero is the same as XP_AllocStruct, but it also zeros out
the allocated memory.

An example:

struct foo {
  int a;
  int b;
};

static XP_AllocStructInfo FooAlloc =
  { XP_INITIALIZE_ALLOCSTRUCTINFO(sizeof(struct foo)) };

  .
  .
  .


  struct foo* ptr = (struct foo*) XP_AllocStruct(&FooAlloc);
      .
      .
      .
  XP_FreeStruct(FooAlloc, ptr);
-----------------------------------------------------------------------------*/

typedef struct XP_AllocStructInfo {
  int size;
  void* curchunk;
  int leftinchunk;
  void* firstfree;
  void* firstchunk;
  int numalloced;
} XP_AllocStructInfo;


#define XP_INITIALIZE_ALLOCSTRUCTINFO(size) ((size + sizeof(void*) - 1) / sizeof(void*)) * sizeof(void*)


XP_BEGIN_PROTOS

void XP_InitAllocStructInfo(XP_AllocStructInfo* info, int size);
void* XP_AllocStruct(XP_AllocStructInfo* info);
void* XP_AllocStructZero(XP_AllocStructInfo* info);
void XP_FreeStruct(XP_AllocStructInfo* info, void* ptr);
void XP_FreeAllStructs(XP_AllocStructInfo* info); /* Danger!  Use with care! */

XP_END_PROTOS




#endif /* _XP_MEM_ */


