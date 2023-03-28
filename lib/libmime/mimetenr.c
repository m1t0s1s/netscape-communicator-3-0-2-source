/* -*- Mode: C; tab-width: 4 -*-
   mimetenr.c --- definition of the MimeInlineTextEnriched class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#include "mimetenr.h"

/* All the magic for this class is in mimetric.c; since text/enriched and
   text/richtext are so similar, it was easiest to implement them in the
   same method (but this is a subclass anyway just for general goodness.)
 */

#define MIME_SUPERCLASS mimeInlineTextRichtextClass
MimeDefClass(MimeInlineTextEnriched, MimeInlineTextEnrichedClass,
			 mimeInlineTextEnrichedClass, &MIME_SUPERCLASS);

static int
MimeInlineTextEnrichedClassInitialize(MimeInlineTextEnrichedClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) class;
  MimeInlineTextRichtextClass *rclass = (MimeInlineTextRichtextClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  rclass->enriched_p = TRUE;
  return 0;
}
