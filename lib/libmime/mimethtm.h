/* -*- Mode: C; tab-width: 4 -*-
   mimethtm.h --- definition of the MimeInlineTextHTML class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMETHTM_H_
#define _MIMETHTM_H_

#include "mimetext.h"

/* The MimeInlineTextHTML class implements the text/html MIME content type.
 */

typedef struct MimeInlineTextHTMLClass MimeInlineTextHTMLClass;
typedef struct MimeInlineTextHTML      MimeInlineTextHTML;

struct MimeInlineTextHTMLClass {
  MimeInlineTextClass text;
};

extern MimeInlineTextHTMLClass mimeInlineTextHTMLClass;

struct MimeInlineTextHTML {
  MimeInlineText text;
};

#endif /* _MIMETHTM_H_ */
