/* -*- Mode:C; tab-width: 8 -*-
 * xfe_dns.h --- hooking X Mozilla into the portable nonblocking DNS code.
 * Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-96.
 */

#ifndef __XFE_DNS_H__
#define __XFE_DNS_H__

#include "xp.h"
#include <X11/Intrinsic.h>	/* for XtAppContext */

/* Call this first thing in main().
   (It might not return, but might exec some other program at the other
   end of a fork, so don't do *anything* before you call it.)
 */
void XFE_InitDNS_Early(int argc, char **argv);

/* Call this some time later, when it's finally safe to call XtAppAddInput().
 */
void XFE_InitDNS_Late(XtAppContext app);

#endif /* __XFE_DNS_H__ */
