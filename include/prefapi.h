/* -*- Mode: C; tab-width: 8 -*-
   prefapi.h --- bare-minimum implementation of 4.0 preferences in 3.01.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 8-Mar-97.
 */
#ifndef _PREFAPI_H_
#define _PREFAPI_H_

#include "xp_core.h"

XP_BEGIN_PROTOS

extern int PREF_SetBoolPref(const char *pref, XP_Bool value);
extern int PREF_SetIntPref(const char *pref, int32 value);
extern int PREF_SetCharPref(const char *pref, const char *value);

extern int PREF_GetBoolPref(const char *pref, XP_Bool *return_val);
extern int PREF_GetIntPref(const char *pref, int32 *return_int);
extern int PREF_GetCharPref(const char *pref, char *return_buf, int *buf_len);

extern int PREF_CopyCharPref(const char *pref, char **return_buf);

extern int PREF_SavePrefFile(void);


XP_END_PROTOS

#endif /* _PREFAPI_H_ */
