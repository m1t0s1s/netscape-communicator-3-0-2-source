/* -*- Mode: C; tab-width: 4 -*-
   mimeiimg.h --- definition of the MimeInlineImage class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEIIMG_H_
#define _MIMEIIMG_H_

#include "mimeleaf.h"

/* The MimeInlineImage class implements those MIME image types which can be
   displayed inline (currently image/gif, image/jpeg, and image/xbm.)
 */

typedef struct MimeInlineImageClass MimeInlineImageClass;
typedef struct MimeInlineImage      MimeInlineImage;

struct MimeInlineImageClass {
  MimeLeafClass leaf;
};

extern MimeInlineImageClass mimeInlineImageClass;

struct MimeInlineImage {
  MimeLeaf leaf;

  /* Opaque data object for the backend-specific inline-image-display code
	 (internal-external-reconnect nastiness.) */
  void *image_data;
};

#endif /* _MIMEIIMG_H_ */
