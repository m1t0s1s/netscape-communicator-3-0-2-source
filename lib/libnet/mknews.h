
#ifndef MKNEWS_H
#define MKNEWS_H

#ifndef MKGETURL_H
#include "mkgeturl.h"
#endif /* MKGETURL_H */

extern CONST char * NET_MKGetNewsHost ();
extern int NET_NewsLoad (ActiveEntry * cur_entry, char * ssl_proxy_server);
extern int NET_ProcessNews (ActiveEntry *cur_entry);
extern int NET_InterruptNews (ActiveEntry *cur_entry);
extern void NET_CleanupNews (void);

#endif /* MKNEWS_H */
