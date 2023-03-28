/* -*- Mode: C; tab-width: 4 -*-
   mimemmix.c --- definition of the MimeMultipartMixed class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#include "mimemmix.h"

#define MIME_SUPERCLASS mimeMultipartClass
MimeDefClass(MimeMultipartMixed, MimeMultipartMixedClass,
			 mimeMultipartMixedClass, &MIME_SUPERCLASS);

static int
MimeMultipartMixedClassInitialize(MimeMultipartMixedClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  return 0;
}
