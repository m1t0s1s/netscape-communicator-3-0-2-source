/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
 * unpublished work protected as such under copyright law.  This work
 * contains proprietary, confidential, and trade secret information of
 * RSA Data Security, Inc.  Use, disclosure or reproduction without the
 * express written authorization of RSA Data Security, Inc. is
 * prohibited.
 */
#include "crypto.h"
#include "cmp.h"
#include "dsa.h"
#include "secitem.h"
#include "secrng.h"

extern int SEC_ERROR_NO_MEMORY;


/* This routine initializes the DSA key generation process, constructing
 *   the CMPInt's of context, allocating space where needed and placing
 *   the p, q and g values into the proper context elements.
 */
DSAKeyGenContext *
DSA_CreateKeyGenContext(PQGParams *params)
{
    DSAKeyGenContext *context;
    int bitLength;
    CMPStatus status;

    context = (DSAKeyGenContext *) PORT_ZAlloc(sizeof(DSAKeyGenContext));
    if (context == NULL)
	goto loser;

    CMP_Constructor (&context->p);
    CMP_Constructor (&context->q);
    CMP_Constructor (&context->g);

    /* Initialize to NULL so that if there is an error before allocation,
     *   when this is destroyed, we won't try to free up something we did not
     *   allocate.
     */
    context->prime.data = NULL;
    context->subPrime.data = NULL;
    context->base.data = NULL;
    context->privateValue.data = NULL;
    context->publicValue.data = NULL;

    /* Convert key components to CMP format, and make sure
	 *    MIN_DSA_PRIME_BITS <= ||p|| <= MAX_DSA_PRIME_BITS
	 *    ||q|| = DSA_SUBPRIME_BITS
	 *    g < p
	 */
    status = CMP_OctetStringToCMPInt(params->prime.data, params->prime.len,
				     &context->p);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_OctetStringToCMPInt(params->subPrime.data,
				     params->subPrime.len, &context->q);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_OctetStringToCMPInt(params->base.data, params->base.len,
				     &context->g);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Is the prime too big or too small? */
    bitLength = CMP_BitLengthOfCMPInt (&context->p);
    if ( (bitLength > MAX_DSA_PRIME_BITS) ||
	 (bitLength < MIN_DSA_PRIME_BITS) ) {
	goto loser;
    }

    /* Is the sub prime the wrong size? */
    bitLength = CMP_BitLengthOfCMPInt (&context->q);
    if (bitLength != DSA_SUBPRIME_BITS) {
	goto loser;
    }

    /* Is the base bigger than or equal to the prime? */
    if (CMP_Compare (&context->g, &context->p) >= 0) {
	goto loser;
    }

    /* Allocate space for prime, subPrime, and base */
    context->prime.data = PORT_Alloc(params->prime.len);
    if (context->prime.data == NULL)
	goto loser;

    context->subPrime.data = PORT_Alloc(params->subPrime.len);
    if (context->subPrime.data == NULL)
	goto loser;

    context->base.data = PORT_Alloc(params->base.len);
    if (context->base.data == NULL)
	goto loser;

    /* Copy parameters to context. */
    PORT_Memcpy(context->prime.data, params->prime.data, params->prime.len);
    context->prime.len = params->prime.len;

    PORT_Memcpy(context->subPrime.data, params->subPrime.data,
		params->subPrime.len);
    context->subPrime.len = params->subPrime.len;

    PORT_Memcpy(context->base.data, params->base.data, params->base.len);
    context->base.len = params->base.len;

    /* Initialize key pair pointers */
    context->privateKey = NULL;
    context->publicKey = NULL;

    /* Mark as initialized. */
    context->initialized = 1;

    return context;
loser:
    if (context != NULL) DSA_DestroyKeyGenContext(context);
    return NULL;
}

/* This routine performs the DSA key generation. */
SECStatus
DSA_KeyGen(DSAKeyGenContext *context, SECKEYLowPublicKey **publicKey,
	   SECKEYLowPrivateKey **privateKey, unsigned char *randomBlock)
{
    CMPStatus status;
    SECStatus secstatus;
    unsigned int xBufferSize, yBufferSize;
    CMPInt x, y;
    PRArenaPool *arena;

    CMP_Constructor (&x);
    CMP_Constructor (&y);

    /* Check to see if the process has been initialized. */
    if (context->initialized != 1)
	goto loser;

	/* Convert random block to CMP, use y as a temp */
    status = CMP_OctetStringToCMPInt(randomBlock, DSA_SUBPRIME_LEN, &y);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Reduce mod q to get private key x. */
    status = CMP_ModularReduce(&y, &context->q, &x);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Compute DSA public key according to the equation
	 * y = g^x mod p.
	 */
    status = CMP_ModExp(&context->g, &x, &context->p, &y);
    if (status != CMP_SUCCESS)
	goto loser;

	/* Compute buffer sizes of x and y ITEM's */
    xBufferSize = CMP_BITS_TO_LEN(CMP_BitLengthOfCMPInt(&x)) + 1;
    yBufferSize = CMP_BITS_TO_LEN(CMP_BitLengthOfCMPInt(&y)) + 1;

    context->privateValue.data = PORT_Alloc(xBufferSize);
    if (context->privateValue.data == NULL)
	goto loser;

    context->publicValue.data = PORT_Alloc(yBufferSize);
    if (context->publicValue.data == NULL)
	goto loser;

    /* Convert results to octet strings */
    status = CMP_CMPIntToSignedOctetString(&x, xBufferSize,
					   &(context->privateValue.len),
					   context->privateValue.data);
    if (status != CMP_SUCCESS)
	goto loser;

    status = CMP_CMPIntToSignedOctetString(&y, yBufferSize,
					   &(context->publicValue.len),
					   context->publicValue.data);
    if (status != CMP_SUCCESS)
	goto loser;

    /* Create key pair structures */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    context->publicKey = PORT_ArenaZAlloc(arena, sizeof(SECKEYLowPublicKey));
    if (context->publicKey == NULL) {
	PORT_FreeArena(arena, PR_FALSE);/* free arena.  we won't when we lose */
	goto loser;
    }

    context->publicKey->arena = arena;
    context->publicKey->keyType = dsaKey;
    secstatus = SECITEM_CopyItem(arena, &context->publicKey->u.dsa.params.prime,
				 &context->prime);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena,
				 &context->publicKey->u.dsa.params.subPrime,
				 &context->subPrime);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena, &context->publicKey->u.dsa.params.base,
				 &context->base);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena, &context->publicKey->u.dsa.publicValue,
				 &context->publicValue);
    if (secstatus == SECFailure)
	goto loser;


    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    context->privateKey = PORT_ArenaZAlloc(arena, sizeof(SECKEYLowPrivateKey));
    if (context->privateKey == NULL) {
	PORT_FreeArena(arena, PR_TRUE); /* free arena.  we won't when we lose */
	goto loser;
    }

    context->privateKey->arena = arena;
    context->privateKey->keyType = dsaKey;
    secstatus = SECITEM_CopyItem(arena,
				 &context->privateKey->u.dsa.params.prime,
				 &context->prime);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena,
				 &context->privateKey->u.dsa.params.subPrime,
				 &context->subPrime);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena, &context->privateKey->u.dsa.params.base,
				 &context->base);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena, &context->privateKey->u.dsa.publicValue,
				 &context->publicValue);
    if (secstatus == SECFailure)
	goto loser;
    secstatus = SECITEM_CopyItem(arena,
				 &context->privateKey->u.dsa.privateValue,
				 &context->privateValue);
    if (secstatus == SECFailure)
	goto loser;

    CMP_Destructor (&x);
    CMP_Destructor (&y);

    *privateKey = context->privateKey;
    *publicKey = context->publicKey;
    context->privateKey = NULL;
    context->publicKey = NULL;

    return SECSuccess;
loser:

    if (context->publicKey != NULL)
	PORT_FreeArena(context->publicKey->arena, PR_FALSE);
    if (context->privateKey != NULL)
	PORT_FreeArena(context->privateKey->arena, PR_TRUE);

    return SECFailure;
}

/* Destroy the CMPInt's and free up any allocated space in the context. */
void
DSA_DestroyKeyGenContext(DSAKeyGenContext *context)
{
    PORT_Assert(context != NULL);

    CMP_Destructor (&context->p);
    CMP_Destructor (&context->q);
    CMP_Destructor (&context->g);

    if (context->prime.data != NULL) {
	PORT_Memset(context->prime.data, 0, context->prime.len);
	PORT_Free(context->prime.data);
	context->prime.data = NULL;
    }

    if (context->subPrime.data != NULL) {
	PORT_Memset(context->subPrime.data, 0, context->subPrime.len);
	PORT_Free(context->subPrime.data);
	context->subPrime.data = NULL;
    }

    if (context->base.data != NULL) {
	PORT_Memset(context->base.data, 0, context->base.len);
	PORT_Free(context->base.data);
	context->base.data = NULL;
    }

    if (context->privateValue.data != NULL) {
	PORT_Memset(context->privateValue.data, 0, context->privateValue.len);
	PORT_Free(context->privateValue.data);
	context->privateValue.data = NULL;
    }

    if (context->publicValue.data != NULL) {
	PORT_Memset(context->publicValue.data, 0, context->publicValue.len);
	PORT_Free(context->publicValue.data);
	context->publicValue.data = NULL;
    }

    PORT_Memset(context, 0, sizeof(DSAKeyGenContext));
    PORT_Free(context);
    return;
}

SECStatus
DSA_NewKey(PQGParams *params, SECKEYLowPublicKey **pubKey,
	   SECKEYLowPrivateKey **privKey)
{
    DSAKeyGenContext *cx;
    unsigned char randomBlock[20];
    SECStatus status;
    
    cx = DSA_CreateKeyGenContext(params);
    if (cx == NULL)
	return SECFailure;

    RNG_GenerateGlobalRandomBytes(randomBlock, sizeof(randomBlock));
    status = DSA_KeyGen(cx, pubKey, privKey, randomBlock);
    DSA_DestroyKeyGenContext(cx);
    return status;
}
