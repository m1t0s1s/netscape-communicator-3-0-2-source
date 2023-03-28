/* -*- Mode: C; tab-width: 4 -*-
   mimemsg.h --- definition of the MimeMessage class (see mimei.h)
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEMSG_H_
#define _MIMEMSG_H_

#include "mimecont.h"

/* The MimeMessage class implements the message/rfc822 and message/news
   MIME containers, which is to say, mail and news messages.
 */

typedef struct MimeMessageClass MimeMessageClass;
typedef struct MimeMessage      MimeMessage;

struct MimeMessageClass {
  MimeContainerClass container;
};

extern MimeMessageClass mimeMessageClass;

struct MimeMessage {
  MimeContainer container;		/* superclass variables */
  MimeHeaders *hdrs;			/* headers of this message */
  XP_Bool newline_p;			/* whether the last line ended in a newline */
  XP_Bool crypto_stamped_p;		/* whether the header of this message has been
								   emitted expecting its child to emit HTML
								   which says that it is encrypted. */

  XP_Bool crypto_msg_signed_p;	/* What the emitted crypto-stamp *says*. */
  XP_Bool crypto_msg_encrypted_p;
};

#endif /* _MIMEMSG_H_ */
