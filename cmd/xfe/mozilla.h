/* -*- Mode: C; tab-width: 8 -*-
   mozilla.h --- generic includes for the X front end.
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Jun-94.
 */
#ifndef __xfe_mozilla_h_
#define __xfe_mozilla_h_

#include "xp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(sun) && !defined(__svr4__)
#include <strings.h>
#endif
#include <errno.h>
#include <memory.h>
#include <time.h>

#if defined(__linux)
extern int putenv (const char *);
#endif

#endif /* __xfe_mozilla_h_ */
