/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
#include "crypto.h"
#include "cmp.h"
#include "dsa.h"

extern int SEC_ERROR_NEED_RANDOM;

/* Initialize the DSA signing process, constructing and setting CMPInt
     elements of context.
 */
DSASignContext *DSA_CreateSignContext(SECKEYLowPrivateKey *key)
{
    CMPStatus status;
    int bitLength;
    DSASignContext *context;

    PORT_Assert(key->keyType = dsaKey);

    context = (DSASignContext *) PORT_ZAlloc(sizeof(DSASignContext));
    if (context == NULL)
	goto loser;

    CMP_Constructor (&context->p);
    CMP_Constructor (&context->q);
    CMP_Constructor (&context->g);
    CMP_Constructor (&context->privateValue);
    CMP_Constructor (&context->r);
    CMP_Constructor (&context->xr);
    CMP_Constructor (&context->kInv);

    /* Convert key components to CMP format, and check
     *   MIN_DSA_PRIME_BITS <= ||p|| <= MAX_DSA_PRIME_BITS
     *   ||q|| = DSA_SUBPRIME_BITS
     *   g < p
     *   x < q
     */
    status = CMP_OctetStringToCMPInt(key->u.dsa.params.prime.data,
				     key->u.dsa.params.prime.len, &context->p);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Is the prime within the legal bounds? */
    bitLength = CMP_BitLengthOfCMPInt (&context->p);
    if ((bitLength < MIN_DSA_PRIME_BITS) ||
	(bitLength > MAX_DSA_PRIME_BITS)) {
	goto loser;
    }

    status = CMP_OctetStringToCMPInt(key->u.dsa.params.subPrime.data,
				     key->u.dsa.params.subPrime.len,
				     &context->q);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Is the subPrime the proper length? */
    bitLength = CMP_BitLengthOfCMPInt(&context->q);
    if (bitLength != DSA_SUBPRIME_BITS)
	goto loser;

    status = CMP_OctetStringToCMPInt(key->u.dsa.params.base.data,
				     key->u.dsa.params.base.len,
				     &context->g);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Is g >= p? */
    if (CMP_Compare(&context->g, &context->p) >= 0)
	goto loser;

    status = CMP_OctetStringToCMPInt(key->u.dsa.privateValue.data,
				     key->u.dsa.privateValue.len,
				     &context->privateValue);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Is x >= the prime? */
    if (CMP_Compare(&context->privateValue, &context->q) >= 0)
	goto loser;

    /* Mark as initialized. */
    context->state = 1;

    return context;

loser:
    if (context != NULL) DSA_DestroySignContext(context,PR_TRUE);
    return NULL;
}

/* Given a random block the same length as the subPrime, build elements
     of a DSA signature.
 */
SECStatus
DSA_Presign(DSASignContext *context, unsigned char *randomBlock)
{
    CMPStatus status;
    CMPInt k, tempCMP;

    /* Check that the context is initialized. */
    if (context->state != 1 && context->state != 2)
	return (SECFailure);

    CMP_Constructor (&k);
    CMP_Constructor (&tempCMP);

    /* Convert randomBlock to CMP format. */
    status = CMP_OctetStringToCMPInt(randomBlock, DSA_SUBPRIME_LEN, &k);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Compute kInv = k^-1 mod q. If the value is invertible, return
	 * "Need Random". k must be less than q, if it is not, reduce.
	 */
    if (CMP_Compare(&k, &context->q) > 0) {
	status = CMP_ModularReduce (&k, &context->q, &tempCMP);
	if (status != CMP_SUCCESS)
	    goto loser;
	status = CMP_ModInvert(&tempCMP, &context->q, &context->kInv);
	if (status != CMP_SUCCESS) {
	    if (status == CMP_INVERSE)
		PORT_SetError(SEC_ERROR_NEED_RANDOM);
	    goto loser;
	}
    } else {
	status = CMP_ModInvert (&k, &context->q, &context->kInv);
	if (status != CMP_SUCCESS) {
	    if (status == CMP_INVERSE)
		PORT_SetError(SEC_ERROR_NEED_RANDOM);
	    goto loser;
	}
    }

    /* Compute r = (g^k mod p) mod q and xr = (x * r) mod q */
    status = CMP_ModExp(&context->g, &k, &context->p, &tempCMP);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_ModularReduce(&tempCMP, &context->q, &context->r);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_ModMultiply(&context->privateValue, &context->r, &context->q,
			     &context->xr);
    if (status != CMP_SUCCESS)
	goto loser;

    CMP_Destructor (&k);
    CMP_Destructor (&tempCMP);

    /* If successful, mark as pre-signed. */
    context->state = 2;
    return SECSuccess;

loser:
    CMP_Destructor (&k);
    CMP_Destructor (&tempCMP);
    return SECFailure;
}

/* A DSA signature consists of subPrime length bytes of
     r = base^k mod prime concatenate sig = kInv (digest + xr) mod q.
     The signature argument is a buffer of length 2 * subPrime length,
     the digest is of length subPrime length.
 */
SECStatus
DSA_Sign(DSASignContext *context, unsigned char *signature,
	unsigned int *signLength, unsigned int maxSignLength,
	 unsigned char *digest, unsigned int digestLength)
{
    CMPStatus status;
    unsigned int octetLen;
    CMPInt digestCMP, sigCMP, tempCMP;

    *signLength = 0;

    /* Make sure the context has gone through pre-signing. */
    if (context->state != 2)
	return SECFailure;

    if (maxSignLength < DSA_SUBPRIME_LEN*2)
	return SECFailure;

    if (digestLength != DSA_SUBPRIME_LEN)
	return SECFailure;

    CMP_Constructor(&sigCMP);
    CMP_Constructor(&digestCMP);
    CMP_Constructor(&tempCMP);

    /* Convert digest to CMP format. */
    status = CMP_OctetStringToCMPInt(digest, DSA_SUBPRIME_LEN, &digestCMP);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Compute "sig" part of DSA signature, according to the equation
	 * sig = kInv * (digest + xr) mod q
	 * where xr and kInv have been precomputed.
	 * "Need random" error if sig = 0.
	 */
    status = CMP_Add(&context->xr, &digestCMP, &tempCMP);
    if (status != CMP_SUCCESS)
	goto loser;
    status = CMP_ModMultiply(&context->kInv, &tempCMP,
			     &context->q, &sigCMP);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Check for sig == 0. */
    status = CMP_CMPWordToCMPInt((CMPWord)0, &tempCMP);
    if (status != CMP_SUCCESS)
	goto loser;
    if (CMP_Compare(&tempCMP, &sigCMP) == 0) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	goto loser;
    }

    /* Convert signature to canonical format, in the order (r, sig). At
	 * this point, the result is a cryptographic data unit, not a math
	 * unit. The buffer to return must be the same size as the subPrime,
	 * so if the actual value of the CMPInt is smaller, pad with leading
	 * zeros.
	 */
    status = CMP_CMPIntToFixedLenOctetStr(&context->r, DSA_SUBPRIME_LEN,
					  DSA_SUBPRIME_LEN, &octetLen,
					  signature);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_CMPIntToFixedLenOctetStr(&sigCMP, DSA_SUBPRIME_LEN,
					  DSA_SUBPRIME_LEN, &octetLen,
					  signature + DSA_SUBPRIME_LEN);
    if (status != CMP_SUCCESS)
	goto loser;

    CMP_Destructor (&digestCMP);
    CMP_Destructor (&sigCMP);
    CMP_Destructor (&tempCMP);

    /* Mark as initialized again. */
    context->state = 1;
    *signLength = DSA_SUBPRIME_LEN*2;

    return SECSuccess;
loser:
    CMP_Destructor (&digestCMP);
    CMP_Destructor (&sigCMP);
    CMP_Destructor (&tempCMP);
    return SECFailure;
}

/* Destroy the CMPInt elements of the context. */
void
DSA_DestroySignContext(DSASignContext *context,PRBool freeit)
{
    PORT_Assert(context != NULL);

    CMP_Destructor (&context->p);
    CMP_Destructor (&context->q);
    CMP_Destructor (&context->g);
    CMP_Destructor (&context->privateValue);
    CMP_Destructor (&context->r);
    CMP_Destructor (&context->xr);
    CMP_Destructor (&context->kInv);

    PORT_Memset(context, 0, sizeof(DSASignContext));
    if (freeit) PORT_Free(context);
    return;
}
