/* -*- Mode: C; tab-width: 4 -*-
   mimeenc.c --- MIME encoders and decoders, version 2 (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEENC_H_
#define _MIMEENC_H_

#include "xp.h"

/* This file defines interfaces to generic implementations of Base64, 
   Quoted-Printable, and UU decoders; and of Base64 and Quoted-Printable
   encoders.
 */


/* Opaque objects used by the encoder/decoder to store state. */
typedef struct MimeDecoderData MimeDecoderData;
typedef struct MimeEncoderData MimeEncoderData;


XP_BEGIN_PROTOS

/* functions for creating that opaque data.
 */
MimeDecoderData *MimeB64DecoderInit(int (*output_fn) (const char *buf,
													  int32 size,
													  void *closure),
									void *closure);
MimeDecoderData *MimeQPDecoderInit (int (*output_fn) (const char *buf,
													  int32 size,
													  void *closure),
									void *closure);
MimeDecoderData *MimeUUDecoderInit (int (*output_fn) (const char *buf,
													  int32 size,
													  void *closure),
									void *closure);

MimeEncoderData *MimeB64EncoderInit(int (*output_fn) (const char *buf,
													  int32 size,
													  void *closure),
									void *closure);
MimeEncoderData *MimeQPEncoderInit (int (*output_fn) (const char *buf,
													  int32 size,
													  void *closure),
									void *closure);
MimeEncoderData *MimeUUEncoderInit (int (*output_fn) (const char *buf,
													  int32 size,
													  void *closure),
									void *closure);

/* Push data through the encoder/decoder, causing the above-provided write_fn
   to be called with encoded/decoded data. */
int MimeDecoderWrite (MimeDecoderData *data, const char *buffer, int32 size);
int MimeEncoderWrite (MimeEncoderData *data, const char *buffer, int32 size);

/* When you're done encoding/decoding, call this to free the data.  If
   abort_p is FALSE, then calling this may cause the write_fn to be called
   one last time (as the last buffered data is flushed out.)
 */
int MimeDecoderDestroy(MimeDecoderData *data, XP_Bool abort_p);
int MimeEncoderDestroy(MimeEncoderData *data, XP_Bool abort_p);

XP_END_PROTOS

#endif /* _MIMEENC_H_ */
