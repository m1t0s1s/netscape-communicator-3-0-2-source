/* -*- Mode: C; tab-width: 4 -*-
   mimemdig.c --- definition of the MimeMultipartDigest class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#include "mimemdig.h"

#define MIME_SUPERCLASS mimeMultipartClass
MimeDefClass(MimeMultipartDigest, MimeMultipartDigestClass,
			 mimeMultipartDigestClass, &MIME_SUPERCLASS);

static int
MimeMultipartDigestClassInitialize(MimeMultipartDigestClass *class)
{
  MimeObjectClass    *oclass = (MimeObjectClass *)    class;
  MimeMultipartClass *mclass = (MimeMultipartClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  mclass->default_part_type = MESSAGE_RFC822;
  return 0;
}
