#include "xp_mem.h"
#include "xp_mcom.h"

#define ALLOCSIZE	4096

void
XP_InitAllocStructInfo(XP_AllocStructInfo* info, int size)
{
  info->size = XP_INITIALIZE_ALLOCSTRUCTINFO(size);
  info->curchunk = NULL;
  info->leftinchunk = 0;
  info->firstfree = NULL;
  info->firstchunk = NULL;
  info->numalloced = 0;
}


void*
XP_AllocStruct(XP_AllocStructInfo* info)
{
  void* result;
  if (info->firstfree) {
    result = info->firstfree;
    info->firstfree = *((void**) info->firstfree);
  } else {
    if (info->curchunk == NULL) {
      info->leftinchunk = (ALLOCSIZE - sizeof(void*)) / info->size;
      if (info->leftinchunk < 1) info->leftinchunk = 1;
      info->curchunk = XP_ALLOC(info->size * info->leftinchunk +
				sizeof(void*));
      while (info->curchunk == NULL) {
	info->leftinchunk /= 2;
	if (info->leftinchunk < 1) return NULL;
	info->curchunk = XP_ALLOC(info->size * info->leftinchunk +
				  sizeof(void*));
      }
      *((void**) info->curchunk) = info->firstchunk;
      info->firstchunk = info->curchunk;
      info->curchunk = ((uint8*) info->curchunk) + sizeof(void**);
    }

    result = info->curchunk;
    info->leftinchunk--;
    if (info->leftinchunk <= 0) {
      info->curchunk = NULL;
    } else {
      info->curchunk = ((uint8*) info->curchunk) + info->size;
    }
  }

  info->numalloced++;
  return result;
}

void*
XP_AllocStructZero(XP_AllocStructInfo* info)
{
  void* result = XP_AllocStruct(info);
  if (result) {
    XP_BZERO(result, info->size);
  }
  return result;
}

void
XP_FreeStruct(XP_AllocStructInfo* info, void* ptr)
{
  *((void**) ptr) = info->firstfree;
  info->firstfree = ptr;
  info->numalloced--;
  if (info->numalloced <= 0) {
    XP_FreeAllStructs(info);
  }
}

void
XP_FreeAllStructs(XP_AllocStructInfo* info)
{
  void* next;
  for (; info->firstchunk; info->firstchunk = next) {
    next = *((void**) info->firstchunk);
    XP_FREE(info->firstchunk);
  }
  info->curchunk = NULL;
  info->leftinchunk = 0;
  info->firstfree = NULL;
  info->numalloced = 0;
}
