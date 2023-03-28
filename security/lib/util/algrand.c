/*
 * Random Number Generator (RNG) implementation.
 *
 * $Id: algrand.c,v 1.25.2.1 1997/04/05 04:10:17 jwz Exp $
 */

#include "secrng.h"
#include "sechash.h"

#ifndef NSPR20
#if defined(__sun)
# include "sunos4.h"
#endif /* __sun */
#endif /* NSPR20 */

/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

RNGContext* sec_base_rng = NULL;

struct RNGContextStr {
    unsigned char state[20];               /* 20 bytes of input to digest */
    unsigned char output[20];              /* current output of digest */
    unsigned int outputAvailable;
    SHA1Context *digestContext;
    unsigned char seed[20];
};

static RNGContext *
createContext(void)
{
    RNGContext *cx;
    SECStatus status;

    cx = (RNGContext*) PORT_ZAlloc(sizeof(RNGContext));
    if (cx) {
	cx->digestContext = SHA1_NewContext();
	if (!cx->digestContext)
	    goto loser;
	RNG_GetNoise(&cx->seed, sizeof(cx->seed));
	status = RNG_Update(cx, cx->seed, sizeof(cx->seed));
	if (status == SECFailure)
	    goto loser;
    }
    return cx;

  loser:
    RNG_DestroyContext(cx, PR_TRUE);
    return NULL;
}

/*
 * Stir the random number generator.  If input is non-NULL, include it in
 * the mix.
 */
static SECStatus
stir(RNGContext *cx, void *input, size_t inputLen)
{
    unsigned char digest[20];
    int i;
    unsigned int x, digestLen;
  
    if (input != NULL) {
	/* Digest input into 20 bytes of stuff */
	SHA1_Begin(cx->digestContext);
	SHA1_Update(cx->digestContext, input, inputLen);
	SHA1_End(cx->digestContext, digest, &digestLen, sizeof(digest));
    } else {
	PORT_Memset(digest, 0, sizeof (digest));	
    }

    /* output = SHA1(input + state) */
    x = 0;
    for (i = 19; i >= 0; i--) {
	x += cx->state[i] + digest[i];
	digest[i] = (unsigned char)x;
	x >>= 8;
    }
    SHA1_Begin(cx->digestContext);
    SHA1_Update(cx->digestContext, digest, sizeof(digest));
    SHA1_End(cx->digestContext, digest, &digestLen, sizeof(digest));

    if(PORT_Memcmp(digest, cx->output, sizeof(digest)) == 0)
	/* Fail FIPS continuous RNG test */
	return SECFailure;

    /* put output into output buffer */
    PORT_Memcpy(cx->output, digest, sizeof(digest));
  
    /* state = SHA1(1 + output + state) */
    x = 1;
    for (i = 19; i >= 0; i--) {
	x += cx->state[i] + digest[i];
	digest[i] = (unsigned char)x;
	x >>= 8;
    }
    SHA1_Begin(cx->digestContext);
    SHA1_Update(cx->digestContext, digest, sizeof(digest));
    SHA1_End(cx->digestContext, cx->state, &digestLen, sizeof(cx->state));
    cx->outputAvailable = 20;

    /* Zeroize sensitive information. */
    PORT_Memset(digest, 0, sizeof (digest));
    return SECSuccess;
}

/*
 * Functions for dealing with the root PRNG
 */

SECStatus
RNG_RandomUpdate(void *data, size_t bytes)
{
    SECStatus status;

    if (sec_base_rng == NULL) {
	status = RNG_RNGInit();
	if (status == SECFailure)
	    return SECFailure;
    }
    return RNG_Update(sec_base_rng, data, bytes);
}

SECStatus
RNG_GenerateGlobalRandomBytes(void *data, size_t bytes)
{
    return RNG_GenerateRandomBytes(sec_base_rng, data, bytes);
}

SECStatus
RNG_RNGInit(void)
{
    if (sec_base_rng != NULL)
	return SECSuccess;

    sec_base_rng = createContext();
	/*
	 * XXX
	 * Assume that there is room for a small node in the heap
	 * when doing initialization. The problem is that there is
	 * not much that can be done otherwise and there is no
	 * cross platform abort code.
	 */
    if (sec_base_rng == NULL)
	return SECFailure;

    return SECSuccess;
}

SECStatus
RNG_ResetRandom(void)
{
    SECStatus status;

    if (sec_base_rng == NULL) {
	status = RNG_RNGInit();
	if (status == SECFailure)
	    return SECFailure;
    }

    return RNG_Reseed(sec_base_rng);
}

/*
 * Exported RNG routines.
 */

RNGContext *
RNG_CreateContext(void)
{
    RNGContext *cx;
    SECStatus status;

    if (sec_base_rng == NULL) return NULL;

    cx = createContext();
    if (cx != NULL ) {
	status = RNG_GenerateRandomBytes(sec_base_rng, cx->seed,
					 sizeof(cx->seed));
	if (status == SECFailure)
	    goto loser;
	status = RNG_Update(cx, cx->seed, sizeof(cx->seed));
	if (status == SECFailure)
	    goto loser;
    }
    
    return cx;

loser:
    RNG_DestroyContext(cx, PR_TRUE);
    return NULL;
}

void
RNG_DestroyContext(RNGContext *cx, PRBool freeit)
{
    if (cx) {
	SHA1_DestroyContext(cx->digestContext, PR_TRUE);
	cx->digestContext = 0;
	if (freeit) {
	    PORT_Free(cx);
	}
    }
}

void
RNG_Init(RNGContext *cx)
{
    PORT_Memset(cx->state, 0, sizeof(cx->state));
    PORT_Memset(cx->output, 0, sizeof(cx->output));
    cx->outputAvailable = 0;
}

SECStatus
RNG_Update(RNGContext *cx, void *input, size_t inputLen)
{
    return stir(cx, input, inputLen);
}

SECStatus
RNG_GenerateRandomBytes(RNGContext *cx, void *output, size_t outputLen)
{
    unsigned int available;
    char *dest = output;	/* make sure to do byte arithmetic */
    SECStatus status;
  
    available = cx->outputAvailable;

    while (outputLen > available) {
	PORT_Memcpy(dest, &cx->output[20-available], available);
	dest += available;
	outputLen -= available;

	/* generate new output */
	status = stir(cx, NULL, 0);
	if (status == SECFailure)
	    return SECFailure;
	available = cx->outputAvailable;
    }

    PORT_Memcpy(dest, &cx->output[20-available], outputLen);
    cx->outputAvailable = available - outputLen;
    return SECSuccess;
}

SECStatus
RNG_Reseed(RNGContext *cx)
{
    (void) RNG_GetNoise(cx->seed, sizeof(cx->seed));
    return RNG_Update(cx, cx->seed, sizeof(cx->seed));
}
