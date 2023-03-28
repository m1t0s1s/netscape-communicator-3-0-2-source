/* -*- Mode: C; tab-width: 4 -*-
   mimempar.h --- definition of the MimeMultipartParallel class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMPAR_H_
#define _MIMEMPAR_H_

#include "mimemult.h"

/* The MimeMultipartParallel class implements the multipart/parallel MIME 
   container, which is currently no different from multipart/mixed, since
   it's not clear that there's anything useful it could do differently.
 */

typedef struct MimeMultipartParallelClass MimeMultipartParallelClass;
typedef struct MimeMultipartParallel      MimeMultipartParallel;

struct MimeMultipartParallelClass {
  MimeMultipartClass multipart;
};

extern MimeMultipartParallelClass mimeMultipartParallelClass;

struct MimeMultipartParallel {
  MimeMultipart multipart;
};

#endif /* _MIMEMPAR_H_ */
