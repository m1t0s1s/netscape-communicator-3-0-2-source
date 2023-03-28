/* -*- Mode: C; tab-width: 4 -*-
   mimecont.h --- definition of the MimeContainer class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMECONT_H_
#define _MIMECONT_H_

#include "mimeobj.h"

/* MimeContainer is the class for the objects representing all MIME 
   types which can contain other MIME objects within them.  In addition 
   to the methods inherited from MimeObject, it provides one method:

   int add_child (MimeObject *parent, MimeObject *child)

     Given a parent (a subclass of MimeContainer) this method adds the
     child (any MIME object) to the parent's list of children.

     The MimeContainer `finalize' method will finalize the children as well.
 */

typedef struct MimeContainerClass MimeContainerClass;
typedef struct MimeContainer      MimeContainer;

struct MimeContainerClass {
  MimeObjectClass object;
  int (*add_child) (MimeObject *parent, MimeObject *child);
};

extern MimeContainerClass mimeContainerClass;

struct MimeContainer {
  MimeObject object;		/* superclass variables */

  MimeObject **children;	/* list of contained objects */
  int32 nchildren;			/* how many */
};

#endif /* _MIMECONT_H_ */
