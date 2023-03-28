/* -*- Mode: C; tab-width: 4 -*-
   mimemdig.h --- definition of the MimeMultipartDigest class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMDIG_H_
#define _MIMEMDIG_H_

#include "mimemult.h"

/* The MimeMultipartDigest class implements the multipart/digest MIME 
   container, which is just like multipart/mixed, except that the default
   type (for parts with no type explicitly specified) is message/rfc822
   instead of text/plain.
 */

typedef struct MimeMultipartDigestClass MimeMultipartDigestClass;
typedef struct MimeMultipartDigest      MimeMultipartDigest;

struct MimeMultipartDigestClass {
  MimeMultipartClass multipart;
};

extern MimeMultipartDigestClass mimeMultipartDigestClass;

struct MimeMultipartDigest {
  MimeMultipart multipart;
};

#endif /* _MIMEMDIG_H_ */
