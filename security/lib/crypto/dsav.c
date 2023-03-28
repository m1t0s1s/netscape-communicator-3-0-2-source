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

/* Initialize the DSA signature verification process, constructing and
 *   setting CMPInt elements of context.
 */
DSAVerifyContext *
DSA_CreateVerifyContext(SECKEYLowPublicKey *key)
{
    DSAVerifyContext *context;
    int status, bitLength;

    PORT_Assert(key->keyType = dsaKey);

    context = (DSAVerifyContext *) PORT_ZAlloc(sizeof(DSAVerifyContext));
    if (context == NULL)
	goto loser;

    CMP_Constructor (&context->p);
    CMP_Constructor (&context->q);
    CMP_Constructor (&context->g);
    CMP_Constructor (&context->publicValue);

    /* Convert key components to CMP format, and make sure
     * A_MIN_DSA_PRIME_BITS <= ||p|| <= A_MAX_DSA_PRIME_BITS
     * ||q|| = A_DSA_SUBPRIME_BITS
     * g < p
     * y < p
     */
    status = CMP_OctetStringToCMPInt(key->u.dsa.params.prime.data,
				     key->u.dsa.params.prime.len,
				     &context->p);
    if (status != CMP_SUCCESS)
	goto loser;

    bitLength = CMP_BitLengthOfCMPInt(&context->p);
    if ((bitLength < MIN_DSA_PRIME_BITS) || (bitLength > MAX_DSA_PRIME_BITS))
	goto loser;

    status = CMP_OctetStringToCMPInt(key->u.dsa.params.subPrime.data,
				     key->u.dsa.params.subPrime.len,
				     &context->q);
    if (status != CMP_SUCCESS)
	goto loser;

    bitLength = CMP_BitLengthOfCMPInt(&context->q);
    if (bitLength != DSA_SUBPRIME_BITS)
	goto loser;

    status = CMP_OctetStringToCMPInt(key->u.dsa.params.base.data,
				     key->u.dsa.params.base.len,
				     &context->g);
    if (status != CMP_SUCCESS)
	goto loser;

    /* base must be less than prime. */
    if (CMP_Compare(&context->g, &context->p) >= 0)
	goto loser;

    status = CMP_OctetStringToCMPInt(key->u.dsa.publicValue.data,
				     key->u.dsa.publicValue.len,
				     &context->publicValue);
    if (status != CMP_SUCCESS)
	goto loser;

    /* y must be less than prime. */
    if (CMP_Compare(&context->publicValue, &context->p) >= 0)
	goto loser;

    /* Mark as initialized. */
    context->initialized = 1;

    return context;
loser:
    if (context != NULL) DSA_DestroyVerifyContext(context,PR_TRUE);
    return NULL;
}

/* Given the digest, which is subPrime bits long, and the signature, which
     is 2 * subPrime bits long, verify the signature.
 */
SECStatus
DSA_Verify(DSAVerifyContext *context, unsigned char *signature,
	unsigned int signLength, unsigned char *digest,
	unsigned int hashLength)
{
    int status;
    CMPInt digestCMP, rPortion, sigPortion, sigInverse, u1, u2, a, b, ab, v;
    PRBool valid;

    CMP_Constructor (&digestCMP);
    CMP_Constructor (&rPortion);
    CMP_Constructor (&sigPortion);
    CMP_Constructor (&sigInverse);
    CMP_Constructor (&u1);
    CMP_Constructor (&u2);
    CMP_Constructor (&a);
    CMP_Constructor (&b);
    CMP_Constructor (&ab);
    CMP_Constructor (&v);

    /* Make sure the context has been initialized. */
    if (context->initialized != 1)
	goto loser;

    if (hashLength != DSA_SUBPRIME_LEN)
	goto loser;

    if (signLength != DSA_SUBPRIME_LEN*2) 
	goto loser;

    /* Convert digest and signature, in the order (r,s), to CMP format
     * and make sure
     * r < q
     * s < p
     */
    status = CMP_OctetStringToCMPInt(digest, DSA_SUBPRIME_LEN, &digestCMP);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_OctetStringToCMPInt(signature, DSA_SUBPRIME_LEN, &rPortion);
    if (status != CMP_SUCCESS)
	goto loser;

    if (CMP_Compare(&rPortion, &context->q) >= 0)
	goto loser;

    status = CMP_OctetStringToCMPInt(signature + DSA_SUBPRIME_LEN,
				     DSA_SUBPRIME_LEN, &sigPortion);
    if (status != CMP_SUCCESS)
	goto loser;

    if (CMP_Compare(&sigPortion, &context->q) >= 0)
	goto loser;

    /* Verify signature, according to the equations
     * sigInverse = sigPortion^-1 mod q
     * u1 = (digest * sigInverse) mod q
     * u2 = (rPortion * sigInverse) mod q
     * a  = g^(u1) mod p
     * b  = y^(u2) mod p
     * v  = (ab mod p) mod q
     * The resulting v should match the rPortion of the signature.
     * Error if sigPortion is not invertible.
     */
    status = CMP_ModInvert(&sigPortion, &context->q, &sigInverse);
    if (status != CMP_SUCCESS) {
	/*if (status == CMP_INVERSE)
	    status = AE_INPUT_DATA ;*/
	goto loser;
    }

    /* u1 = (digest * sigInverse) mod subPrime. */
    status = CMP_ModMultiply(&digestCMP, &sigInverse, &context->q, &u1);
    if (status != CMP_SUCCESS)
	goto loser;

    /* u2 = (rPortion * sigInverse) mod subPrime. */
    status = CMP_ModMultiply(&rPortion, &sigInverse, &context->q, &u2);
    if (status != CMP_SUCCESS)
	goto loser;

    /* a = base ^ u1 mod prime. */
    status = CMP_ModExp(&context->g, &u1, &context->p, &a);
    if (status != CMP_SUCCESS)
	goto loser;

    /* b = y ^ u2 mod prime. */
    status = CMP_ModExp(&context->publicValue, &u2, &context->p, &b);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Find (a * b) mod prime. */
    status = CMP_ModMultiply(&a, &b, &context->p, &ab);
    if (status != CMP_SUCCESS)
	goto loser;

    /* v = [ (a * b) mod prime ] mod subPrime. */
    status = CMP_ModularReduce(&ab, &context->q, &v);
    if (status != CMP_SUCCESS)
	goto loser;

    /* If v = rPortion, the signature verifies. */
    valid = (CMP_Compare (&v, &rPortion) == 0);

    CMP_Destructor (&digestCMP);
    CMP_Destructor (&rPortion);
    CMP_Destructor (&sigPortion);
    CMP_Destructor (&sigInverse);
    CMP_Destructor (&u1);
    CMP_Destructor (&u2);
    CMP_Destructor (&a);
    CMP_Destructor (&b);
    CMP_Destructor (&ab);
    CMP_Destructor (&v);

    return valid ? SECSuccess : SECFailure;

loser:
    return SECFailure;
}

/* Destroy the CMPInt's of the signature varification context. */
void
DSA_DestroyVerifyContext(DSAVerifyContext *context, PRBool freeit)
{
    PORT_Assert(context != NULL);

    CMP_Destructor (&context->p);
    CMP_Destructor (&context->q);
    CMP_Destructor (&context->g);
    CMP_Destructor (&context->publicValue);
    if (freeit) PORT_Free(context);
    return;
}
