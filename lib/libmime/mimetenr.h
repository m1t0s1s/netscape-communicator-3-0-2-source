/* -*- Mode: C; tab-width: 4 -*-
   mimetenr.h --- definition of the MimeInlineTextEnriched class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMETENR_H_
#define _MIMETENR_H_

#include "mimetric.h"

/* The MimeInlineTextEnriched class implements the text/enriched MIME content
   type, as defined in RFC 1563.  It does this largely by virtue of being a
   subclass of the MimeInlineTextRichtext class.
 */

typedef struct MimeInlineTextEnrichedClass MimeInlineTextEnrichedClass;
typedef struct MimeInlineTextEnriched      MimeInlineTextEnriched;

struct MimeInlineTextEnrichedClass {
  MimeInlineTextRichtextClass text;
};

extern MimeInlineTextEnrichedClass mimeInlineTextEnrichedClass;

struct MimeInlineTextEnriched {
  MimeInlineTextRichtext richtext;
};

#endif /* _MIMETENR_H_ */
