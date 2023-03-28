/*
 * SHA-1 implementation
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: algsha.c,v 1.11 1997/02/13 03:02:28 nelsonb Exp $
 *
 * Reference implementation of NIST FIPS PUB 180-1.
 *   (Secure Hash Standard, revised version).
 *   Copyright 1994 by Paul C. Kocher.  All rights reserved.
 *   
 * Comments:  This implementation is written to be as close as 
 *   possible to the NIST specification.  No performance or size 
 *   optimization has been attempted.
 *
 * Disclaimer:  This code is provided without warranty of any kind.
 */

#include "sechash.h"

struct SHA1ContextStr {
    uint32 h[5];
    uint32 w[80];
    int lenW;
    uint32 sizeHi, sizeLo;
};

#define SHS_ROTL(X,n) (((X) << (n)) | ((X) >> (32-(n))))

static void
shsHashBlock(SHA1Context *ctx)
{
    int t;
    uint32 a, b, c, d, e, temp;

    for (t = 16; t <= 79; t++)
	ctx->w[t] = SHS_ROTL(
	    ctx->w[t-3] ^ ctx->w[t-8] ^ ctx->w[t-14] ^ ctx->w[t-16], 1);

    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];

    for (t = 0; t <= 19; t++) {
	temp = SHS_ROTL(a,5) + (((c^d)&b)^d) + e + ctx->w[t] + 0x5a827999l;
	e = d;
	d = c;
	c = SHS_ROTL(b, 30);
	b = a;
	a = temp;
    }
    for (t = 20; t <= 39; t++) {
	temp = SHS_ROTL(a,5) + (b^c^d) + e + ctx->w[t] + 0x6ed9eba1l;
	e = d;
	d = c;
	c = SHS_ROTL(b, 30);
	b = a;
	a = temp;
    }
    for (t = 40; t <= 59; t++) {
	temp = SHS_ROTL(a,5) + ((b&c)|(d&(b|c))) + e + ctx->w[t] + 0x8f1bbcdcl;
	e = d;
	d = c;
	c = SHS_ROTL(b, 30);
	b = a;
	a = temp;
    }
    for (t = 60; t <= 79; t++) {
	temp = SHS_ROTL(a,5) + (b^c^d) + e + ctx->w[t] + 0xca62c1d6l;
	e = d;
	d = c;
	c = SHS_ROTL(b, 30);
	b = a;
	a = temp;
    }

    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

void
SHA1_Begin(SHA1Context *ctx)
{
    ctx->lenW = 0;
    ctx->sizeHi = ctx->sizeLo = 0;

    /* initialize h with the magic constants (see fips180 for constants)
     */
    ctx->h[0] = 0x67452301l;
    ctx->h[1] = 0xefcdab89l;
    ctx->h[2] = 0x98badcfel;
    ctx->h[3] = 0x10325476l;
    ctx->h[4] = 0xc3d2e1f0l;
}


SHA1Context *
SHA1_NewContext(void)
{
    SHA1Context *cx;

    cx = (SHA1Context*) PORT_ZAlloc(sizeof(SHA1Context));
    return cx;
}

SHA1Context *
SHA1_CloneContext(SHA1Context *cx)
{
    SHA1Context *clone = SHA1_NewContext();
    if (clone)
	*clone = *cx;
    return clone;
}

void
SHA1_DestroyContext(SHA1Context *cx, PRBool freeit)
{
    if (freeit) {
        PORT_ZFree(cx, sizeof(SHA1Context));
    }
}

void
SHA1_Update(SHA1Context *ctx, const unsigned char *dataIn, unsigned len)
{
    unsigned int i;

    /* Read the data into W and process blocks as they get full
     */
    for (i = 0; i < len; i++) {
	ctx->w[ctx->lenW / 4] <<= 8;
	ctx->w[ctx->lenW / 4] |= (uint32)dataIn[i];
	if ((++ctx->lenW) % 64 == 0) {
	    shsHashBlock(ctx);
	    ctx->lenW = 0;
	}
	ctx->sizeLo += 8;
	ctx->sizeHi += (ctx->sizeLo < 8);
    }
}


void
SHA1_End(SHA1Context *ctx, unsigned char *hashout,
	 unsigned int *pDigestLen, unsigned int maxDigestLen)
{
    unsigned char pad0x80 = 0x80;
    unsigned char pad0x00 = 0x00;
    unsigned char padlen[8];
    int i;

    PORT_Assert (maxDigestLen >= SHA1_LENGTH);

    /* 
     * Pad with a binary 1 (e.g. 0x80), then zeroes, then length
     */
    padlen[0] = (unsigned char)((ctx->sizeHi >> 24) & 255);
    padlen[1] = (unsigned char)((ctx->sizeHi >> 16) & 255);
    padlen[2] = (unsigned char)((ctx->sizeHi >> 8) & 255);
    padlen[3] = (unsigned char)((ctx->sizeHi >> 0) & 255);
    padlen[4] = (unsigned char)((ctx->sizeLo >> 24) & 255);
    padlen[5] = (unsigned char)((ctx->sizeLo >> 16) & 255);
    padlen[6] = (unsigned char)((ctx->sizeLo >> 8) & 255);
    padlen[7] = (unsigned char)((ctx->sizeLo >> 0) & 255);

    SHA1_Update(ctx, &pad0x80, 1);
    while (ctx->lenW != 56)
	SHA1_Update(ctx, &pad0x00, 1);
    SHA1_Update(ctx, padlen, 8);

    /* Output hash
     */
    for (i = 0; i < SHA1_LENGTH; i++) {
	hashout[i] = (unsigned char)(ctx->h[i / 4] >> 24);
	ctx->h[i / 4] <<= 8;
    }
    *pDigestLen = SHA1_LENGTH;
    SHA1_Begin(ctx);
}


SECStatus
SHA1_HashBuf(unsigned char *dest, const unsigned char *src, uint32 src_length)
{
    SHA1Context ctx;		/* XXX shouldn't this be allocated? */
    unsigned int outLen;

    SHA1_Begin(&ctx);
    SHA1_Update(&ctx, src, src_length);
    SHA1_End(&ctx, dest, &outLen, SHA1_LENGTH);

    return SECSuccess;
}

SECStatus
SHA1_Hash(unsigned char *dest, const char *src)
{
    return SHA1_HashBuf(dest, (const unsigned char *)src, PORT_Strlen (src));
}
