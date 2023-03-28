#include "prmacros.h"
#include <stddef.h>

#include <Types.h>

NSPR_BEGIN_EXTERN_C

void 				MacintoshInitializeMemory(void);
Boolean 			Memory_ReserveInMacHeap(size_t spaceNeeded);
Boolean 			Memory_ReserveInMallocHeap(size_t spaceNeeded);
Boolean 			InMemory_ReserveInMacHeap();
size_t 				Memory_FreeMemoryRemaining();

unsigned char 		GarbageCollectorCacheFlusher(size_t size);


NSPR_END_EXTERN_C
