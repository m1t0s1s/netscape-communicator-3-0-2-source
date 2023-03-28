/* -*- Mode: C; tab-width: 4 -*-
  mimemalt.h --- definition of the MimeMultipartAlternative class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMALT_H_
#define _MIMEMALT_H_

#include "mimemult.h"
#include "mimepbuf.h"

/* The MimeMultipartAlternative class implements the multipart/alternative
   MIME container, which displays only one (the `best') of a set of enclosed
   documents.
 */

typedef struct MimeMultipartAlternativeClass MimeMultipartAlternativeClass;
typedef struct MimeMultipartAlternative      MimeMultipartAlternative;

struct MimeMultipartAlternativeClass {
  MimeMultipartClass multipart;
};

extern MimeMultipartAlternativeClass mimeMultipartAlternativeClass;

struct MimeMultipartAlternative {
  MimeMultipart multipart;			/* superclass variables */

  MimeHeaders *buffered_hdrs;		/* The headers of the currently-pending
									   part. */
  MimePartBufferData *part_buffer;	/* The data of the current-pending part
									   (see mimepbuf.h) */
};

#endif /* _MIMEMALT_H_ */
