/* -*- Mode: C; tab-width: 4 -*-
   xp_thrmo.h --- Status message text for the thermometer.
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
 */

#ifndef _XP_THRMO_
#define _XP_THRMO_

#include "xp_core.h"

XP_BEGIN_PROTOS
extern const char *
XP_ProgressText (unsigned long total_bytes,
		 unsigned long bytes_received,
		 unsigned long start_time_secs,
		 unsigned long now_secs);
XP_END_PROTOS

#endif /* _XP_THRMO_ */
