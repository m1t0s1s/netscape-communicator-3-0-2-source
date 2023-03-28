#ifndef __prdbmem_h__
#define __prdbmem_h__

#include <malloc.h>

#ifdef _USE_PR_DEBUG_MEMORY

#define malloc(n)   PR_DebugMalloc ((n), __FILE__##" line:", __LINE__)
#define calloc(s,n) PR_DebugCalloc ((s), (n), __FILE__##" line:", __LINE__)
#define relloc(p,n) PR_DebugRealloc((p), (n), __FILE__##" line:", __LINE__)
#define free(p)     PR_DebugFree   ((p), __FILE__##" line:", __LINE__)
#define strdup(s)   PR_DebugStrdup ((s), __FILE__##" line:", __LINE__)

#endif

PR_PUBLIC_API(void *) PR_DebugMalloc(size_t size, const char *where, long line);
PR_PUBLIC_API(void *) PR_DebugCalloc(size_t size, size_t count, const char *where, long line);
PR_PUBLIC_API(void *) PR_DebugRealloc(void *oldp, size_t newsize, const char *where, long line);
PR_PUBLIC_API(void) PR_DebugFree(void *p, const char *where, long line);
PR_PUBLIC_API(char *) PR_DebugStrdup(const char *str, const char *where, long line);


#endif __prdbmem_h__
