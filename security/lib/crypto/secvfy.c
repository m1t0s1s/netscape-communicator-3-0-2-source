/*
 * Verification stuff.
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secvfy.c,v 1.30.2.1 1997/03/22 23:38:41 jwz Exp $
 */

#include <stdio.h>
#include "crypto.h"
#include "sechash.h"
#include "key.h"
#include "secasn1.h"
#include "secoid.h"
#include "pk11func.h"
#include "rsa.h"

extern int SEC_ERROR_INVALID_ALGORITHM;
extern int SEC_ERROR_INVALID_ARGS;
extern int SEC_ERROR_OUTPUT_LEN;
extern int SEC_ERROR_BAD_SIGNATURE;
extern int SEC_ERROR_IO;


/*
** Decrypt signature block using public key (in place)
** XXX this is assuming that the signature algorithm has WITH_RSA_ENCRYPTION
*/
static SECStatus
DecryptSigBlock(int *tagp, unsigned char *digest, SECKEYPublicKey *key,
		SECItem *sig, char *wincx)
{
    SECItem it;
    SGNDigestInfo *di = NULL;
    unsigned char *dsig;
    SECStatus rv;
    SECOidTag tag;
    unsigned char buf[MAX_RSA_MODULUS_LEN];
    
    dsig = NULL;
    it.data = buf;
    it.len = sizeof(buf);

    if (key == NULL) goto loser;

    /* Decrypt signature block */
    dsig = (unsigned char*) PORT_Alloc(sig->len);
    if (dsig == NULL) goto loser;

    /* decrypt the block */
    rv = PK11_VerifyRecover(key, sig, &it, wincx);
    if (rv != SECSuccess) goto loser;

    di = SGN_DecodeDigestInfo(&it);
    if (di == NULL) goto sigloser;

    /*
    ** Finally we have the digest info; now we can extract the algorithm
    ** ID and the signature block
    */
    tag = SECOID_GetAlgorithmTag(&di->digestAlgorithm);
    /* XXX Check that tag is an appropriate algorithm? */
    if (di->digest.len > 32) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	goto loser;
    }
    PORT_Memcpy(digest, di->digest.data, di->digest.len);
    *tagp = tag;
    goto done;

  sigloser:
    PORT_SetError(SEC_ERROR_BAD_SIGNATURE);

  loser:
    rv = SECFailure;

  done:
    if (di != NULL) SGN_DestroyDigestInfo(di);
    if (dsig != NULL) PORT_Free(dsig);
    
    return rv;
}

struct VFYContextStr {
    int alg;
    enum { VFY_RSA, VFY_DSA } type;
    SECKEYPublicKey *key;
    /* digest holds the full dsa signature... 40 bytes */
    unsigned char digest[40];
    void * wincx;
    void *hashcx;
    SECHashObject *hashobj;
};

VFYContext *
VFY_CreateContext(SECKEYPublicKey *key, SECItem *sig, void *wincx)
{
    VFYContext *cx;
    SECStatus rv;

    cx = (VFYContext*) PORT_ZAlloc(sizeof(VFYContext));
    cx->wincx = cx;
    if (cx) {
	switch (key->keyType) {
	  case rsaKey:
	    cx->type = VFY_RSA;
	    cx->key = NULL; /* paranoia */
	    rv = DecryptSigBlock(&cx->alg, &cx->digest[0], key, sig, wincx);
	    break;
	  case fortezzaKey:
	  case dsaKey:
	    cx->type = VFY_DSA;
	    cx->alg = SEC_OID_SHA1;
	    cx->key = SECKEY_CopyPublicKey(key);
	    PORT_Memcpy(&cx->digest[0],sig->data,sig->len);
	    rv = SECSuccess;
	    break;
	  default:
	    rv = SECFailure;
	    break;
	}
	if (rv) goto loser;
	switch (cx->alg) {
	  case SEC_OID_MD2:
	  case SEC_OID_MD5:
	  case SEC_OID_SHA1:
	    break;
	  default:
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    goto loser;
	}
    }
    return cx;

  loser:
    VFY_DestroyContext(cx, PR_TRUE);
    return 0;
}

void
VFY_DestroyContext(VFYContext *cx, PRBool freeit)
{
    if (cx) {
	if (cx->hashcx != NULL) {
	    (*cx->hashobj->destroy)(cx->hashcx, PR_TRUE);
	    cx->hashcx = NULL;
	}
	if (cx->key) {
	    SECKEY_DestroyPublicKey(cx->key);
	}
	if (freeit) {
	    PORT_ZFree(cx, sizeof(VFYContext));
	}
    }
}

SECStatus
VFY_Begin(VFYContext *cx)
{
    if (cx->hashcx != NULL) {
	(*cx->hashobj->destroy)(cx->hashcx, PR_TRUE);
	cx->hashcx = NULL;
    }

    switch (cx->alg) {
      case SEC_OID_MD2:
	cx->hashobj = &SECHashObjects[HASH_AlgMD2];
	break;
      case SEC_OID_MD5:
	cx->hashobj = &SECHashObjects[HASH_AlgMD5];
	break;
      case SEC_OID_SHA1:
	cx->hashobj = &SECHashObjects[HASH_AlgSHA1];
	break;
      default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }

    cx->hashcx = (*cx->hashobj->create)();
    if (cx->hashcx == NULL)
	return SECFailure;

    (*cx->hashobj->begin)(cx->hashcx);
    return SECSuccess;
}

SECStatus
VFY_Update(VFYContext *cx, unsigned char *input, unsigned inputLen)
{
    if (cx->hashcx == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    (*cx->hashobj->update)(cx->hashcx, input, inputLen);
    return SECSuccess;
}

SECStatus
VFY_End(VFYContext *cx)
{
    unsigned char final[32];
    unsigned part;
    SECItem hash,sig;

    if (cx->hashcx == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    (*cx->hashobj->end)(cx->hashcx, final, &part, sizeof(final));
    switch (cx->type) {
      case VFY_DSA:
	sig.data = cx->digest;
	sig.len = 40; /* magic size of dsa signature */
	hash.data = final;
	hash.len = part;
	if (PK11_Verify(cx->key,&sig,&hash,cx->wincx) != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		return SECFailure;
	}
	break;
      case VFY_RSA:
	if (PORT_Memcmp(final, cx->digest, part)) {
	    PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    return SECFailure;
	}
	break;
      default:
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	return SECFailure; /* shouldn't happen */
    }
    return SECSuccess;
}

/************************************************************************/
/*
 * Verify that a previously-computed digest matches a signature.
 * XXX This should take a parameter that specifies the digest algorithm,
 * and we should compare that the algorithm found in the DigestInfo
 * matches it!
 */
SECStatus
VFY_VerifyDigest(SECItem *digest, SECKEYPublicKey *key,
						 SECItem *sig, void *wincx)
{
    SECStatus rv;
    VFYContext *cx;

    rv = SECFailure;

    cx = VFY_CreateContext(key, sig, wincx);
    if (cx != NULL) {
	switch (key->keyType) {
	case rsaKey:
	    if (PORT_Memcmp(digest->data, cx->digest, digest->len)) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    } else {
		rv = SECSuccess;
	    }
	    break;
	case fortezzaKey:
	case dsaKey:
	     if (PK11_Verify(cx->key,sig,digest,wincx) != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    } else {
		rv = SECSuccess;
	    }
	    break;
	default:
	    break;
	}

	VFY_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
VFY_VerifyData(unsigned char *buf, int len, SECKEYPublicKey *key,
	       SECItem *sig, void *wincx)
{
    SECStatus rv;
    VFYContext *cx;

    cx = VFY_CreateContext(key, sig, wincx);
    if (cx == NULL)
	return SECFailure;

    rv = VFY_Begin(cx);
    if (rv == SECSuccess) {
	rv = VFY_Update(cx, buf, len);
	if (rv == SECSuccess)
	    rv = VFY_End(cx);
    }

    VFY_DestroyContext(cx, PR_TRUE);
    return rv;
}

SECStatus
SEC_VerifyFile(FILE *input, SECKEYPublicKey *key, SECItem *sig, void *wincx)
{
    unsigned char buf[1024];
    SECStatus rv;
    int nb;
    VFYContext *cx;

    cx = VFY_CreateContext(key, sig, wincx);
    if (cx == NULL)
	rv = SECFailure;

    rv = VFY_Begin(cx);
    if (rv == SECSuccess) {
	/*
	 * Now feed the contents of the input file into the digest algorithm,
	 * one chunk at a time, until we have exhausted the input.
	 */
	for (;;) {
	    if (feof(input))
		break;
	    nb = fread(buf, 1, sizeof(buf), input);
	    if (nb == 0) {
		if (ferror(input)) {
		    PORT_SetError(SEC_ERROR_IO);
		    VFY_DestroyContext(cx, PR_TRUE);
		    return SECFailure;
		}
		break;
	    }
	    rv = VFY_Update(cx, buf, nb);
	    if (rv != SECSuccess)
		goto loser;
	}
    }

    /* Verify the digest */
    rv = VFY_End(cx);
    /* FALL THROUGH */

  loser:
    VFY_DestroyContext(cx, PR_TRUE);
    return rv;
}
