#ifndef MKHTTP_H
#define MKHTTP_H

#ifndef MKGETURL_H
#include "MKGetURL.h"
#endif /* MKGETURL_H */

#define DEF_HTTP_PORT 80
#define DEF_HTTPS_PORT 443

extern int
NET_HTTPLoad (ActiveEntry * cur_entry);

extern int
NET_ProcessHTTP(ActiveEntry * cur_entry);

extern int 
NET_InterruptHTTP (ActiveEntry *cur_entry);

extern void
NET_CleanupHTTP (void);

#endif /* MKHTTP_H */
