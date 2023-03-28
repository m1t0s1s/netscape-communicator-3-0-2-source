/* -*- Mode: C; tab-width: 8 -*-
 *
 * Header file for routines specific to S/MIME.  Keep things that are pure
 * pkcs7 out of here; this is for S/MIME policy, S/MIME interoperability, etc.
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secmime.h,v 1.2.2.1 1997/04/19 05:12:09 repka Exp $
 */

#ifndef _SECMIME_H_
#define _SECMIME_H_ 1

#include "secpkcs7.h"


/************************************************************************/
SEC_BEGIN_PROTOS

/*
 * Initialize the local recording of the user S/MIME preference.
 */
extern SECStatus SECMIME_EnableCipher(long which, int on);

/*
 * Initialize the local recording of the S/MIME policy.
 */
extern SECStatus SECMIME_SetPolicy(   long which, int on);

/*
 * Does the current policy allow S/MIME decryption of this particular
 * algorithm and keysize?
 */
extern PRBool SECMIME_DecryptionAllowed(SECAlgorithmID *algid, PK11SymKey *key);

/*
 * Start an S/MIME encrypting context.
 *
 * "scert" is the cert for the sender.  It will be checked for validity.
 * "rcerts" are the certs for the recipients.  They will also be checked.
 *
 * "certdb" is the cert database to use for verifying the certs.
 * It can be NULL if a default database is available (like in the client).
 *
 * This function already does all of the stuff specific to S/MIME protocol
 * and local policy; the return value just needs to be passed to
 * SEC_PKCS7Encode() or to SEC_PKCS7EncoderStart() to create the encoded data,
 * and finally to SEC_PKCS7DestroyContentInfo().
 *
 * An error results in a return value of NULL and an error set.
 * (Retrieve specific errors via PORT_GetError()/XP_GetError().)
 */
extern SEC_PKCS7ContentInfo *SECMIME_CreateEncrypted(CERTCertificate *scert,
						     CERTCertificate **rcerts,
						     CERTCertDBHandle *certdb,
						     SECKEYGetPasswordKey pwfn,
						     void *pwfn_arg);

/*
 * Start an S/MIME signing context.
 *
 * "scert" is the cert that will be used to sign the data.  It will be
 * checked for validity.
 *
 * "certdb" is the cert database to use for verifying the cert.
 * It can be NULL if a default database is available (like in the client).
 * 
 * "digestalg" names the digest algorithm.  (It should be SEC_OID_SHA1;
 * XXX There should be SECMIME functions for hashing, or the hashing should
 * be built into this interface, which we would like because we would
 * support more smartcards that way, and then this argument should go away.)
 *
 * "digest" is the actual digest of the data.  It must be provided in
 * the case of detached data or NULL if the content will be included.
 *
 * This function already does all of the stuff specific to S/MIME protocol
 * and local policy; the return value just needs to be passed to
 * SEC_PKCS7Encode() or to SEC_PKCS7EncoderStart() to create the encoded data,
 * and finally to SEC_PKCS7DestroyContentInfo().
 *
 * An error results in a return value of NULL and an error set.
 * (Retrieve specific errors via PORT_GetError()/XP_GetError().)
 */
extern SEC_PKCS7ContentInfo *SECMIME_CreateSigned(CERTCertificate *scert,
						  CERTCertDBHandle *certdb,
						  SECOidTag digestalg,
						  SECItem *digest,
						  SECKEYGetPasswordKey pwfn,
						  void *pwfn_arg);

/************************************************************************/
SEC_END_PROTOS

#endif /* _SECMIME_H_ */
