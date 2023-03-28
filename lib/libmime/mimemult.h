/* -*- Mode: C; tab-width: 4 -*-
   mimemult.h --- definition of the MimeMultipart class (see mimei.h)
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMULT_H_
#define _MIMEMULT_H_

#include "mimecont.h"

/* The MimeMultipart class class implements the objects representing all of
   the "multipart/" MIME types.  In addition to the methods inherited from
   MimeContainer, it provides the following methods and class variables:

   int create_child (MimeObject *obj)

     When it has been determined that a new sub-part should be created,
     this method is called to do that.  The default value for this method
     does it in the usual multipart/mixed way.  The headers of the object-
     to-be-created may be found in the `hdrs' slot of the `MimeMultipart'
     object.

   XP_Bool output_child_p (MimeObject *parent, MimeObject *child)

     Whether this child should be output.  Default method always says `yes'.

   int parse_child_line (MimeObject *obj, char *line, int32 length,
						 XP_Bool first_line_p)

     When we have a line which should be handed off to the currently-active
     child object, this method is called to do that.  The `first_line_p'
     variable will be true only for the very first line handed off to this
     sub-part.  The default method simply passes the line to the most-
	 recently-added child object.

   int close_child (MimeObject *self)

     When we reach the end of a sub-part (a separator line) this method is
	 called to shut down the currently-active child.  The default method
	 simply calls `parse_eof' on the most-recently-added child object.

   MimeMultipartBoundaryType check_boundary (MimeObject *obj, 
											const char *line, int32 length)

     This method is used to examine a line and determine whether it is a
     part boundary, and if so, what kind.  It should return a member of 
     the MimeMultipartBoundaryType describing the line.

   const char *default_part_type

     This is the type which should be assumed for sub-parts which have
     no explicit type specified.  The default is "text/plain", but the
     "multipart/digest" subclass overrides this to "message/rfc822".
 */

typedef struct MimeMultipartClass MimeMultipartClass;
typedef struct MimeMultipart      MimeMultipart;

typedef enum {
  MimeMultipartPreamble,
  MimeMultipartHeaders,
  MimeMultipartPartFirstLine,
  MimeMultipartPartLine,
  MimeMultipartEpilogue
} MimeMultipartParseState;

typedef enum {
  MimeMultipartBoundaryTypeNone,
  MimeMultipartBoundaryTypeSeparator,
  MimeMultipartBoundaryTypeTerminator
} MimeMultipartBoundaryType;


struct MimeMultipartClass {
  MimeContainerClass container;
  const char *default_part_type;

  int (*create_child) (MimeObject *);
  XP_Bool (*output_child_p) (MimeObject *self, MimeObject *child);
  int (*close_child) (MimeObject *);
  int (*parse_child_line) (MimeObject *, char *line, int32 length,
						   XP_Bool first_line_p);
  MimeMultipartBoundaryType (*check_boundary) (MimeObject *, const char *line,
											   int32 length);
};

extern MimeMultipartClass mimeMultipartClass;

struct MimeMultipart {
  MimeContainer container;			/* superclass variables */
  char *boundary;					/* Inter-part delimiter string */
  MimeHeaders *hdrs;				/* headers of the part currently
									   being parsed, if any */
  MimeMultipartParseState state;	/* State of parser */
};

#endif /* _MIMEMULT_H_ */
