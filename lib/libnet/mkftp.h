
#ifndef MKFTP_H
#define MKFTP_H

#include "mkgeturl.h"

extern int NET_FTPLoad (ActiveEntry * cur_entry);
extern int NET_ProcessFTP(ActiveEntry * cur_entry);
extern int NET_InterruptFTP (ActiveEntry *cur_entry);
extern void NET_CleanupFTP (void);

#endif /* HTFTP_H */
