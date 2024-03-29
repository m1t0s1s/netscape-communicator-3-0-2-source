/* -*- Mode: C; tab-width: 4 -*-
   mimeunty.h --- definition of the MimeUntypedText class (see mimei.h)
   Copyright � 1996 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEUNTY_H_
#define _MIMEUNTY_H_

#include "mimecont.h"

/* The MimeUntypedText class is used for untyped message contents, that is, 
   it is the class used for the body of a message/rfc822 object which had
   *no* Content-Type header, as opposed to an unrecognized content-type.
   Such a message, technically, does not contain MIME data (it follows only
   RFC 822, not RFC 1521.)

   This is a container class, and the reason for that is that it loosely
   parses the body of the message looking for ``sub-parts'' and then
   creates appropriate containers for them.

   More specifically, it looks for uuencoded data.  It may do more than that
   some day.

   Basically, the algorithm followed is:

     if line is "begin 644 foo.gif"
       if there is an open sub-part, close it
	   add a sub-part with type: image/gif; encoding: x-uue
	   hand this line to it
	   and hand subsequent lines to that subpart
	 else if there is an open uuencoded sub-part, and line is "end"
	   hand this line to it
	   close off the uuencoded sub-part
     else if there is an open sub-part
	   hand this line to it
     else
       open a text/plain subpart
	   hand this line to it

   Adding other types than uuencode to this (for example, PGP) would be 
   pretty straightforward.
 */

typedef struct MimeUntypedTextClass MimeUntypedTextClass;
typedef struct MimeUntypedText      MimeUntypedText;

struct MimeUntypedTextClass {
  MimeContainerClass container;
};

extern MimeUntypedTextClass mimeUntypedTextClass;

typedef enum {
  MimeUntypedTextSubpartTypeText,	/* text/plain */
  MimeUntypedTextSubpartTypeUUE,	/* uuencoded data */
  MimeUntypedTextSubpartTypeBinhex	/* Mac BinHex data */
} MimeUntypedTextSubpartType;

struct MimeUntypedText {
  MimeContainer container;			/* superclass variables */
  MimeObject *open_subpart;			/* The part still-being-parsed */
  MimeUntypedTextSubpartType type;	/* What kind of type it is */
  MimeHeaders *open_hdrs;			/* The faked-up headers describing it */
};

#endif /* _MIMEUNTY_H_ */
