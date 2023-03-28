/* -*- Mode: C; tab-width: 4 -*-
  mimemapl.h --- definition of the MimeMultipartAppleDouble class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMAPL_H_
#define _MIMEMAPL_H_

#include "mimemult.h"

/* The MimeMultipartAppleDouble class implements the multipart/appledouble
   MIME container, which provides a method of encapsulating and reconstructing
   a two-forked Macintosh file.
 */

typedef struct MimeMultipartAppleDoubleClass MimeMultipartAppleDoubleClass;
typedef struct MimeMultipartAppleDouble      MimeMultipartAppleDouble;

struct MimeMultipartAppleDoubleClass {
  MimeMultipartClass multipart;
};

extern MimeMultipartAppleDoubleClass mimeMultipartAppleDoubleClass;

struct MimeMultipartAppleDouble {
  MimeMultipart multipart;
};

#endif /* _MIMEMAPL_H_ */
