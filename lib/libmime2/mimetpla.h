/* -*- Mode: C; tab-width: 4 -*-
   mimetpla.h --- definition of the MimeInlineTextPlain class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMETPLA_H_
#define _MIMETPLA_H_

#include "mimetext.h"

/* The MimeInlineTextHTML class implements the text/plain MIME content type,
   and is also used for all otherwise-unknown text/ subtypes.
 */

typedef struct MimeInlineTextPlainClass MimeInlineTextPlainClass;
typedef struct MimeInlineTextPlain      MimeInlineTextPlain;

struct MimeInlineTextPlainClass {
  MimeInlineTextClass text;
};

extern MimeInlineTextPlainClass mimeInlineTextPlainClass;

struct MimeInlineTextPlain {
  MimeInlineText text;
};

#endif /* _MIMETPLA_H_ */
