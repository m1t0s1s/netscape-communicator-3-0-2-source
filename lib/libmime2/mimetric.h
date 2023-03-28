/* -*- Mode: C; tab-width: 4 -*-
   mimetric.h --- definition of the MimeInlineTextRichtext class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMETRIC_H_
#define _MIMETRIC_H_

#include "mimetext.h"

/* The MimeInlineTextRichtext class implements the (obsolete and deprecated)
   text/richtext MIME content type, as defined in RFC 1341, and also the
   text/enriched MIME content type, as defined in RFC 1563.
 */

typedef struct MimeInlineTextRichtextClass MimeInlineTextRichtextClass;
typedef struct MimeInlineTextRichtext      MimeInlineTextRichtext;

struct MimeInlineTextRichtextClass {
  MimeInlineTextClass text;
  XP_Bool enriched_p;	/* Whether we should act like text/enriched instead. */
};

extern MimeInlineTextRichtextClass mimeInlineTextRichtextClass;

struct MimeInlineTextRichtext {
  MimeInlineText text;
};


extern int
MimeRichtextConvert (char *line, int32 length,
					 int (*output_fn) (char *buf, int32 size, void *closure),
					 void *closure,
					 char **obufferP,
					 int32 *obuffer_sizeP,
					 XP_Bool enriched_p);

#endif /* _MIMETRIC_H_ */
