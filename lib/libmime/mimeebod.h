/* -*- Mode: C; tab-width: 4 -*-
   mimeext.h --- definition of the MimeExternalBody class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEEBOD_H_
#define _MIMEEBOD_H_

#include "mimeobj.h"

/* The MimeExternalBody class implements the message/external-body MIME type.
   (This is not to be confused with MimeExternalObject, which implements the
   handler for application/octet-stream and other types with no more specific
   handlers.)
 */

typedef struct MimeExternalBodyClass MimeExternalBodyClass;
typedef struct MimeExternalBody      MimeExternalBody;

struct MimeExternalBodyClass {
  MimeObjectClass object;
};

extern MimeExternalBodyClass mimeExternalBodyClass;

struct MimeExternalBody {
  MimeObject object;			/* superclass variables */
  MimeHeaders *hdrs;			/* headers within this external-body, which
								   describe the network data which this body
								   is a pointer to. */
  char *body;					/* The "phantom body" of this link. */
};

#endif /* _MIMEEBOD_H_ */
