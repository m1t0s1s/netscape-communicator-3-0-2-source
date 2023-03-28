/* -*- Mode: C; tab-width: 4 -*-
   mimepkcs.h --- definition of the MimeEncryptedPKCS7 class (see mimei.h)
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 11-Aug-96.
 */

#ifndef _MIMEPKCS_H_
#define _MIMEPKCS_H_

#include "mimecryp.h"

/* The MimeEncryptedPKCS7 class implements a type of MIME object where the
   object is passed through a PKCS7 decryption engine to decrypt or verify
   signatures.  That module returns a new MIME object, which is then presented
   to the user.  See mimecryp.h for details of the general mechanism on which
   this is built.
 */

typedef struct MimeEncryptedPKCS7Class MimeEncryptedPKCS7Class;
typedef struct MimeEncryptedPKCS7      MimeEncryptedPKCS7;

struct MimeEncryptedPKCS7Class {
  MimeEncryptedClass encrypted;

  /* Callback used to access the SEC_PKCS7ContentInfo of this object. */
  void (*get_content_info) (MimeObject *self,
							SEC_PKCS7ContentInfo **content_info_ret,
							char **sender_email_addr_return,
							int32 *decode_error_ret,
							int32 *verify_error_ret);
};

extern MimeEncryptedPKCS7Class mimeEncryptedPKCS7Class;

struct MimeEncryptedPKCS7 {
  MimeEncrypted encrypted;		/* superclass variables */
};

#endif /* _MIMEPKCS_H_ */
