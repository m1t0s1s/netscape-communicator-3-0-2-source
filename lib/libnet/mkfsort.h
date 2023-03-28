
#ifndef MKFSORT_H
#define MKFSORT_H

#include "mksort.h"

#include <time.h>

#ifdef XP_UNIX
#include <sys/types.h>
#endif /* XP_UNIX */

extern void NET_FreeEntryInfoStruct(NET_FileEntryInfo *entry_info);
extern NET_FileEntryInfo * NET_CreateFileEntryInfoStruct (void);
extern int NET_CompareEntryInfoStructs (void *ent1, void *ent2);
extern void NET_DoFileSort (SortStruct * sort_list);

#endif /* MKFSORT_H */
