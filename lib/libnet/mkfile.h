
#ifndef MKFILE_H
#define MKFILE_H

#include "mkfsort.h"

extern int
NET_FileLoad (ActiveEntry * cur_entry);

extern int
NET_ProcessFile (ActiveEntry * cur_entry);

extern int
NET_InterruptFile(ActiveEntry * cur_entry);

extern void
NET_CleanupFile (void);

extern int 
NET_PrintDirectory(SortStruct **sort_base, NET_StreamClass * stream, char * path);

extern NET_StreamClass *
net_CloneWysiwygLocalFile(MWContext *window_id, URL_Struct *URL_s,
						  uint32 nbytes);

#endif /* MKFILE_H */
