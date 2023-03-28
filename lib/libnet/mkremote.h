/*
 * Remote host access routines for Telnet, tn3270, rlogin
 */

#ifndef REMOTE_H
#define REMOTE_H

#ifndef MKGETURL_H
#include "mkgeturl.h"
#endif  /* MKGETURL_H */

extern int NET_RemoteHostLoad (ActiveEntry * cur_entry);

#endif /* REMOTE_H */
