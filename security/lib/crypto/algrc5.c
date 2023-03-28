/* Copyright (C) 1994 RSA Data Security, Inc. Created 1994
   This is an unpublished work protected as such under copyright law.
   This work contains proprietary, confidential, and trade secret information
   of RSA Data Security, Inc.  Use, disclosure, or reproduction without the
   express written authorization of RSA Data Security, Inc., is prohibited.

   Implementation of RC5 encryption algorithm in C.
   Ronald L. Rivest
   September 24, 1994
 */
#include "crypto.h"

typedef unsigned long UINT4;
typedef UINT4 RC5_WORD;

extern int SEC_ERROR_INPUT_LEN;
extern int SEC_ERROR_OUTPUT_LEN;
extern int SEC_ERROR_INVALID_ARGS;

#define RC5_WORDSIZE   32
#define RC5_ROTMASK    (RC5_WORDSIZE - 1)

#define MAX_RC5_ROUNDS        255
#define MIN_RC5_ROUNDS        0
#define MAX_RC5_KEY_BYTES     255
#define MIN_RC5_KEY_BYTES     0
#define MAX_RC5_VERSION       0x10
#define RC5_VALID_WORDSIZE_1  16
#define RC5_VALID_WORDSIZE_2  32
#define RC5_VALID_WORDSIZE_3  64

/* Define auxiliary table sizes. */
#define AUX_TABLE_WORDS  ((MAX_RC5_KEY_BYTES + 3) / 4)
#define AUX_TABLE_BYTES  (AUX_TABLE_WORDS * 4)

/* Define magic constants. */
#define P32 0xb7e15163l
#define Q32 0x9e3779b9l

/* The rotation operator assumes a right shift is logical, not arithmetic.
 *   It also assumes that operands A and B are side-effect free, since they
 *   are evaluated twice.
 *
 *   Both the rotate-left and rotate-right operators
 *   assume A is unsigned
 */
#define RC5_ROTL(A, B) (((A) << (int)((B) & RC5_ROTMASK)) | \
     ((A) >> (int)(RC5_WORDSIZE - ((B) & RC5_ROTMASK))))

#define RC5_ROTR(A, B) (((A) >> (int)((B) & RC5_ROTMASK)) | \
     ((A) << (int)(RC5_WORDSIZE - ((B) & RC5_ROTMASK))))

/* Macros for loading bytes into words and words into bytes for 4 byte words */
/* just do it the slow way for now */
#if 1 /* BIG_ENDIAN > 0 */
#define RC5_LOAD_UINT4(N,blk) \
  N = ((UINT4) (blk)[3] << 24) + ((UINT4) (blk)[2] << 16) + \
     ((UINT4) (blk)[1] << 8) + (UINT4) (blk)[0];
  
#define RC5_SAVE_UINT4(N,blk) \
  *(blk+0) = (unsigned char)(N);       \
  *(blk+1) = (unsigned char)(N >> 8);  \
  *(blk+2) = (unsigned char)(N >> 16); \
  *(blk+3) = (unsigned char)(N >> 24);
#else
#define RC5_LOAD_UINT4(N,blk) (N) = (*((UINT4 *)(blk)));
#define RC5_SAVE_UINT4(N,blk) (*((UINT4 *)(blk))) = (N);
#endif /* BIG_ENDIAN */

struct RC5ContextStr {
    unsigned int rounds;
    UINT4 *keyTable;

    /* Carry forward for CBC mode */
    unsigned char last[8];

    SECStatus (*enc)(RC5Context *cx, unsigned char *out, unsigned *part,
		     unsigned maxOut, unsigned char *in, unsigned inLen);
    SECStatus (*dec)(RC5Context *cx, unsigned char *out, unsigned *part,
		     unsigned maxOut, unsigned char *in, unsigned inLen);
};

static void rc5_encrypt(RC5Context *cx, unsigned char *output,
			unsigned char *input);
static void rc5_decrypt(RC5Context *cx, unsigned char *output,
			unsigned char *input);
static SECStatus rc5_enc(RC5Context *cx, unsigned char *out,
			 unsigned *part, unsigned maxOut,
			 unsigned char *in, unsigned inLen);
static SECStatus rc5_dec(RC5Context *cx, unsigned char *out,
			 unsigned *part, unsigned maxOut,
			 unsigned char *in, unsigned inLen);
static SECStatus rc5cbc_enc(RC5Context *cx, unsigned char *out,
			    unsigned *part, unsigned maxOut,
			    unsigned char *in, unsigned inLen);
static SECStatus rc5cbc_dec(RC5Context *cx, unsigned char *out,
			    unsigned *part, unsigned maxOut,
			    unsigned char *in, unsigned inLen);

/* RC5 DEFINITION CODE -- RC5-32 only */

/* RC5 performs a single-block "electronic code book" mode encryption */
static void
rc5_encrypt(RC5Context *cx, unsigned char *output, unsigned char *input)
{
    int i;
    unsigned int rounds;
    RC5_WORD *Sp;
    register RC5_WORD A, B;

    RC5_LOAD_UINT4(A, input);
    RC5_LOAD_UINT4(B, input + 4);
  
    rounds = cx->rounds;
    Sp     = cx->keyTable;
    A     += *Sp++;
    B     += *Sp++;

    for (i = 0; i < rounds; ++i) {
	A = A ^ B;
	A = RC5_ROTL(A,B) + *Sp++;
	B = B ^ A;
	B = RC5_ROTL(B,A) + *Sp++;
    }
    RC5_SAVE_UINT4(A, output);
    RC5_SAVE_UINT4(B, output + 4);
}

/* RC5 performs a single-block "electronic code book" mode decryption */
static void
rc5_decrypt(RC5Context *cx, unsigned char *output, unsigned char *input)
{
    int i;
    RC5_WORD *Sp;
    RC5_WORD B, A;
    unsigned int rounds;

    rounds = cx->rounds;
    Sp = cx->keyTable + 2 * (rounds + 1);
    RC5_LOAD_UINT4(A, input);
    RC5_LOAD_UINT4(B, input + 4);

    for (i = 0; i < rounds; ++i) {
	B = B - *--Sp;
	B = RC5_ROTR(B,A);
	B = B ^ A;
	A = A - *--Sp;
	A = RC5_ROTR(A,B);
	A = A ^ B;
    }
    B -= *--Sp;
    A -= *--Sp;
    RC5_SAVE_UINT4(A, output);
    RC5_SAVE_UINT4(B, output + 4);
}

static SECStatus
rc5_enc(RC5Context *cx, unsigned char *out, unsigned *part, unsigned maxOut,
	unsigned char *in, unsigned inLen)
{
    *part = inLen;
    while (inLen) {
	rc5_encrypt(cx, out, in);
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

static SECStatus
rc5_dec(RC5Context *cx, unsigned char *out, unsigned *part, unsigned maxOut,
	unsigned char *in, unsigned inLen)
{
    *part = inLen;
    while (inLen) {
	rc5_decrypt(cx, out, in);
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

static SECStatus
rc5cbc_enc(RC5Context *cx, unsigned char *out, unsigned *part, unsigned maxOut,
	   unsigned char *in, unsigned inLen)
{
    int i;

    *part = inLen;
    while (inLen) {
	for (i = 0; i < 8; i++) {
	    out[i] = in[i] ^ cx->last[i];
	}
	rc5_encrypt(cx, out, out);
	for (i = 0; i < 8; i++) {
	    cx->last[i] = out[i];
	}
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

static SECStatus
rc5cbc_dec(RC5Context *cx, unsigned char *out, unsigned *part, unsigned maxOut,
	   unsigned char *in, unsigned inLen)
{
    int i;

    *part = inLen;
    while (inLen) {
	for (i = 0; i < 8; i++) {
	    out[i] = in[i] ^ cx->last[i];
	}
	rc5_decrypt(cx, out, out);
	for (i = 0; i < 8; i++) {
	    cx->last[i] = out[i];
	}
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

/* RC5_CreateContext creates context and expands rc5Key into key table */
RC5Context *
RC5_CreateContext(SECItem *key, unsigned int rounds, unsigned char *iv,
		  int mode)
{
    RC5Context *cx;
    int i, j, k, shift;     
    unsigned int bytesPerWord;
    unsigned int expKeySize;            /* Expanded key size = 2 * rounds + 2 */
    unsigned int keyLenInWords;
    unsigned int keyBytes;              /* number of key bytes supplied */
    unsigned char *rc5Key;              /* points at RC5 key bytes      */
    RC5_WORD *keyTable;                 /* points at expansion table    */
    RC5_WORD A, B, T;                   /* temporary variables */
    RC5_WORD auxTable[AUX_TABLE_WORDS]; /* Auxiliary table for key expansion */

    keyBytes       = key->len;
    rc5Key         = key->data;
    expKeySize     = 2 * (rounds + 1);
    bytesPerWord   = (unsigned int)(RC5_WORDSIZE / 8);

    /* keyBytes must be in the range [0, 255]. Since keyBytes is
       unsigned, only need to check the upper limit. */
    if (keyBytes > MAX_RC5_KEY_BYTES)
	return NULL;

    /* rounds must be in the range [0, 255]. Since rounds is
       unsigned, only need to check the upper limit. */
    if (rounds > MAX_RC5_ROUNDS)
	return NULL;

    /* CBC mode requires an IV */
    if (mode == SEC_RC5_CBC && (iv == NULL))
	return NULL;

    cx = PORT_ZAlloc(sizeof(RC5Context));
    if (cx == NULL)
	goto loser;

    keyTable = cx->keyTable = PORT_ZAlloc(expKeySize * sizeof(UINT4));
    if (cx->keyTable == NULL)
	goto loser;

    cx->rounds = rounds;

    switch(mode) {
      case SEC_RC5:
	cx->enc = rc5_enc;
	cx->dec = rc5_dec;
	break;
      case SEC_RC5_CBC:
	cx->enc = rc5cbc_enc;
	cx->dec = rc5cbc_dec;
	PORT_Memcpy(cx->last, iv, sizeof(cx->last));
	break;
      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	goto loser;
    }

    /* Initialize auxiliary table with RC5 key Data */
    /* Calculate ceiling[keyBytes / bytesPerWord] */
    keyLenInWords = (keyBytes + bytesPerWord - 1) / bytesPerWord;

    shift = 0;
    j = 0;
    auxTable[0] = 0;
    for (i = 0; i < keyBytes; ++i) { 
	auxTable[j] += ((RC5_WORD) rc5Key[i]) << shift;
	shift += 8;
	if (shift == RC5_WORDSIZE) {
	    shift = 0;
	    j++;
	    auxTable[j] = 0;
	}
    }

    /* Initialize keyTable with congruential generator */
    keyTable[0] = P32;
    for (i = 1; i < expKeySize; ++i)
	keyTable[i] = keyTable[i-1] + Q32;

    /* Mix in key */
    i = j = 0;
    A = B = 0;
    k = (keyLenInWords > expKeySize) ?  3 * keyLenInWords : 3 * expKeySize;
    for ( ; k > 0; --k) {
	A = A + B + keyTable[i];
	A = RC5_ROTL(A,3);
	keyTable[i] = A;

	T = A + B;
	B = auxTable[j] + T;
	B = RC5_ROTL(B,T); 
	auxTable[j] = B;

	/* Increment i modulo expKeySize */
	if ((++i) >= expKeySize)
	    i = 0;
	/* Increment j modulo keyLenInWords */
	if ((++j) >= keyLenInWords)
	    j = 0;
    }

    /* Zero out ancilliary data structures to prevent key leakage */
    T = A = B = 0;
    PORT_Memset(auxTable, 0, AUX_TABLE_BYTES);
    return cx;

loser:
    RC5_DestroyContext(cx, PR_TRUE);
    return NULL;
}

void
RC5_DestroyContext(RC5Context *cx, PRBool freeit)
{
    if (cx == NULL)
	return;
    if (cx->keyTable != NULL)
	PORT_Free(cx->keyTable);
    if (freeit == PR_TRUE)
	PORT_Free(cx);
    return;
}

SECStatus
RC5_Encrypt(RC5Context *cx, unsigned char *output, unsigned int *outputLen,
	    unsigned int maxOutputLen, unsigned char *input,
	    unsigned int inputLen)
{
    if (inputLen & 7) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    if (maxOutputLen < inputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    return (*cx->enc)(cx, output, outputLen, maxOutputLen, input, inputLen);
}

SECStatus
RC5_Decrypt(RC5Context *cx, unsigned char *output, unsigned int *outputLen,
	    unsigned int maxOutputLen, unsigned char *input,
	    unsigned int inputLen)
{
    if (inputLen & 7) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    if (maxOutputLen < inputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    return (*cx->dec)(cx, output, outputLen, maxOutputLen, input, inputLen);
}
