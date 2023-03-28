#pragma once
#include <stdlib.h>
#include "xp_core.h"

typedef enum MemoryPriority { fcNone, fcLow, fcMedium, fcHigh, fcTop } MemoryPriority;

#define NEW( CLASS )		( new CLASS )
#define DELETE( OBJECT )	delete OBJECT

XP_BEGIN_PROTOS

extern void*		Flush_Allocate( size_t size, int zero );
extern void*		Flush_Reallocate( void* item, size_t size );
extern void			Flush_Free( void* item );
	
XP_END_PROTOS
