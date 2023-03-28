/* -*- Mode: C; tab-width: 4 -*-
   composec.h --- the crypto interface to MIME generation (see compose.c.)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 22-Oct-96.
 */

#include "xp.h"

XP_BEGIN_PROTOS

extern int mime_begin_crypto_encapsulation (MWContext *context,
											XP_File file,
											void **closure_return,
											XP_Bool encrypt_p, XP_Bool sign_p,
											const char *recipients,
											XP_Bool saveAsDraft);
extern int mime_finish_crypto_encapsulation (void *closure, XP_Bool abort_p);
extern int mime_crypto_write_block (void *closure, char *buf, int32 size);

XP_END_PROTOS
