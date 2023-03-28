
#ifndef MKGOPHER_H
#define MKGOPHER_H

extern int NET_GopherLoad (ActiveEntry * cur_entry);
extern int NET_ProcessGopher (ActiveEntry * cur_entry);
extern int NET_InterruptGopher (ActiveEntry *cur_entry);
extern void NET_CleanupGopher (void);

#endif /* MKGOPHER_H */

