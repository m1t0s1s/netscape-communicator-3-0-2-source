/* -*- Mode: C; tab-width: 4 -*-

xp.h

This file is left around for backwards compatability.  Nothing new should
be added to this file.  Rather, add it to the client specific section or
the cross-platform specific area depending on what is appropriate.

-------------------------------------------------------------------------*/
#ifndef _XP_H_
#define _XP_H_

#include "xp_mcom.h"
#include "client.h"

#ifdef HEAPAGENT

#define MEM_DEBUG         1
#define DEFINE_NEW_MACRO  1
#include <heapagnt.h>

#endif /* HEAPAGENT */

#endif /* !_XP_H_ */   
																  
