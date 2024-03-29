/* -*- Mode: C; tab-width: 4 -*-
   mimempar.c --- definition of the MimeMultipartParallel class (see mimei.h)
   Copyright � 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#include "mimempar.h"

#define MIME_SUPERCLASS mimeMultipartClass
MimeDefClass(MimeMultipartParallel, MimeMultipartParallelClass,
			 mimeMultipartParallelClass, &MIME_SUPERCLASS);

static int
MimeMultipartParallelClassInitialize(MimeMultipartParallelClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  return 0;
}
