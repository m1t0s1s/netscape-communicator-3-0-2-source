/* -*- Mode: C; tab-width: 4 -*-
   mimemmix.h --- definition of the MimeMultipartMixed class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMMIX_H_
#define _MIMEMMIX_H_

#include "mimemult.h"

/* The MimeMultipartMixed class implements the multipart/mixed MIME container,
   and is also used for any and all otherwise-unrecognised subparts of
   multipart/.
 */

typedef struct MimeMultipartMixedClass MimeMultipartMixedClass;
typedef struct MimeMultipartMixed      MimeMultipartMixed;

struct MimeMultipartMixedClass {
  MimeMultipartClass multipart;
};

extern MimeMultipartMixedClass mimeMultipartMixedClass;

struct MimeMultipartMixed {
  MimeMultipart multipart;
};

#endif /* _MIMEMMIX_H_ */
