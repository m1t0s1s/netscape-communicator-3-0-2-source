/* -*- Mode: C; tab-width: 4 -*-
   mimeeobj.h --- definition of the MimeExternalObject class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEEOBJ_H_
#define _MIMEEOBJ_H_

#include "mimeleaf.h"

/* The MimeExternalObject class represents MIME parts which contain data
   which cannot be displayed inline -- application/octet-stream and any
   other type that is not otherwise specially handled.  (This is not to
   be confused with MimeExternalBody, which is the handler for the 
   message/external-object MIME type only.)
 */

typedef struct MimeExternalObjectClass MimeExternalObjectClass;
typedef struct MimeExternalObject      MimeExternalObject;

struct MimeExternalObjectClass {
  MimeLeafClass leaf;
};

extern MimeExternalObjectClass mimeExternalObjectClass;

struct MimeExternalObject {
  MimeLeaf leaf;
};

#endif /* _MIMEEOBJ_H_ */
