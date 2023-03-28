/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
extern int SEC_ERROR_OUTPUT_LEN;
extern int SEC_ERROR_INPUT_LEN;
extern int SEC_ERROR_INVALID_ARGS;

#include "crypto.h"

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

struct RC2ContextStr {
    UINT2 keyTable[64];
    int encrypt;

    /* Carry forward for CBC mode */
    unsigned char last[8];

    SECStatus (*enc)(RC2Context *cx, unsigned char *out, unsigned *part,
		    unsigned maxOut, unsigned char *in, unsigned inLen);
    SECStatus (*dec)(RC2Context *cx, unsigned char *out, unsigned *part,
		    unsigned maxOut, unsigned char *in, unsigned inLen);
};

#ifndef MAX_EFFECTIVE_KEY_BITS
#define MAX_EFFECTIVE_KEY_BITS 1024
#endif

#ifndef MIN_EFFECTIVE_KEY_BITS
#define MIN_EFFECTIVE_KEY_BITS 1
#endif

/* PI_SUBST is pseudorandom permutation for expanding key */
static unsigned char PI_SUBST[256] = { 
  217,120,249,196, 25,221,181,237, 40,233,253,121, 74,160,216,157,
  198,126, 55,131, 43,118, 83,142, 98, 76,100,136, 68,139,251,162,
   23,154, 89,245,135,179, 79, 19, 97, 69,109,141,  9,129,125, 50,
  189,143, 64,235,134,183,123, 11,240,149, 33, 34, 92,107, 78,130,
   84,214,101,147,206, 96,178, 28,115, 86,192, 20,167,140,241,220,
   18,117,202, 31, 59,190,228,209, 66, 61,212, 48,163, 60,182, 38,
  111,191, 14,218, 70,105,  7, 87, 39,242, 29,155,188,148, 67,  3,
  248, 17,199,246,144,239, 62,231,  6,195,213, 47,200,102, 30,215,
    8,232,234,222,128, 82,238,247,132,170,114,172, 53, 77,106, 42,
  150, 26,210,113, 90, 21, 73,116, 75,159,208, 94,  4, 24,164,236,
  194,224, 65,110, 15, 81,203,204, 36,145,175, 80,161,244,112, 57,
  153,124, 58,133, 35,184,180,122,252,  2, 54, 91, 37, 85,151, 49,
   45, 93,250,152,227,138,146,174,  5,223, 41, 16,103,108,186,201,
  211,  0,230,207,225,158,168, 44, 99, 22,  1, 63, 88,226,137,169,
   13, 56, 52, 27,171, 51,255,176,187, 72, 12, 95,185,177,205, 46,
  197,243,219, 71,229,165,156,119, 10,166, 32,104,254,127,193,173
};

/* LEFT[i], RIGHT[i] and ACROSS[i] are clockwise with respect to i */
static unsigned int LEFT[] = { 3, 0, 1, 2 };
static unsigned int RIGHT[] = { 1, 2, 3, 0 };
static unsigned int ACROSS[] = { 2, 3, 0, 1 };

/* ROTATION[i] is rotation amount for i-th word in mixing operation */
static unsigned int ROTATION[] = { 1, 2, 3, 5 };

/* ROTATE_LEFT rotates x left n bits, ROTATE_RIGHT rotates x right n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (16-(n))))
#define ROTATE_RIGHT(x, n) (((x) >> (n)) | ((x) << (16-(n))))

/* Forward mixing round.
 */
static void ForwardMixRound (UINT2 working[4], UINT2 keyTable[64])
{
  int i;
  
  for (i = 0; i < 4; i++) {
    working[i] += keyTable[i];
    working[i] += working[LEFT[i]] & working[ACROSS[i]];
    working[i] += (~working[LEFT[i]]) & working[RIGHT[i]];
    working[i] = ROTATE_LEFT (working[i], ROTATION[i]);
  }
}

/* Reverse mixing round.
 */
static void ReverseMixRound (UINT2 working[4], UINT2 keyTable[64])
{
  int i;

  for (i = 3; i >= 0; i--) {
    working[i] = ROTATE_RIGHT (working[i], ROTATION[i]);
    working[i] -= (~working[LEFT[i]]) & working[RIGHT[i]];
    working[i] -= working[LEFT[i]] & working[ACROSS[i]];
    working[i] -= keyTable[i];
  }
}

/* Forward mashing round.
 */
static void ForwardMashRound (UINT2 working[4], UINT2 keyTable[64])
{ 
  int i;
  
  for (i = 0; i < 4; i++)
    working[i] += keyTable[working[LEFT[i]] & 0x3F];
}

/* Reverse mashing round.
 */
static void ReverseMashRound (UINT2 working[4], UINT2 keyTable[64])
{ 
  int i;

  for (i = 3; i >= 0; i--)
    working[i] -= keyTable[working[LEFT[i]] & 0x3F];
}

/* RC2 performs a single-block "electronic code book" mode encryption or
     decryption.
 */
static void RC2 (UINT2 keyTable[64], unsigned char *out,
		 unsigned char *in, int encryptFlag)
{ 
  UINT2 working[4];                                   /* working data block */
  unsigned int i, ii, j, jIncrement;
  void (*MashRound) (UINT2 [4], UINT2 [64]);
  void (*MixRound) (UINT2 [4], UINT2 [64]);

  /* load input into working data block */
  for (i = 0, ii = 0; i < 4; i++, ii += 2)
    working[i] = (((UINT2)in[ii + 1]) << 8) | ((UINT2)in[ii]);
  
  /* parameterize for encryption or decryption */
  j = encryptFlag ? 0 : 60;
  jIncrement = encryptFlag ? 4 : -4;
  MixRound = encryptFlag ? ForwardMixRound : ReverseMixRound;
  MashRound = encryptFlag ? ForwardMashRound : ReverseMashRound;
  
  /* Encryption or decryption consists of five mixing rounds, a mashing round,
       six more mixing rounds, another mashing round, and five more mixing
       rounds.
   */
  for (i = 0; i < 5; i++, j += jIncrement)
    (*MixRound) (working, &keyTable[j]);
  
  (*MashRound) (working, keyTable);
  
  for (i = 0; i < 6; i++, j += jIncrement)
    (*MixRound) (working, &keyTable[j]);

  (*MashRound) (working, keyTable);
  
  for (i = 0; i < 5; i++, j += jIncrement)
    (*MixRound) (working, &keyTable[j]);
  
  /* store working data block in output */
  for (i = 0, ii = 0; i < 4; i++, ii += 2) {
    out[ii] = (unsigned char)(working[i] & 0xFF);
    out[ii+1] = (unsigned char)((working[i] >> 8) & 0xFF);
  }
}

/************************************************************************/

static SECStatus RC2_Enc(RC2Context *cx, unsigned char *out,
			unsigned *part, unsigned maxOut,
			unsigned char *in, unsigned inLen)
{
    *part = inLen;
    while (inLen) {
	RC2 (cx->keyTable, out, in, 1);
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

static SECStatus RC2_Dec(RC2Context *cx, unsigned char *out,
			unsigned *part, unsigned maxOut,
			unsigned char *in, unsigned inLen)
{
    *part = inLen;
    while (inLen) {
	RC2 (cx->keyTable, out, in, 0);
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

static SECStatus RC2CBC_Enc(RC2Context *cx, unsigned char *out,
			   unsigned *part, unsigned maxOut,
			   unsigned char *in, unsigned inLen)
{
    int i;

    *part = inLen;
    while (inLen) {
	for (i = 0; i < 8; i++) {
	    out[i] = in[i] ^ cx->last[i];
	}
	RC2 (cx->keyTable, out, out, 1);
	for (i = 0; i < 8; i++) {
	    cx->last[i] = out[i];
	}
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

static SECStatus RC2CBC_Dec(RC2Context *cx, unsigned char *out,
			   unsigned *part, unsigned maxOut,
			   unsigned char *in, unsigned inLen)
{
    unsigned char tmp[8];
    int i;

    *part = inLen;
    while (inLen) {
	/* Copy 8 input bytes into temp buf incase out is overwriting in */
	tmp[0] = in[0]; tmp[1] = in[1]; tmp[2] = in[2]; tmp[3] = in[3];
	tmp[4] = in[4]; tmp[5] = in[5]; tmp[6] = in[6]; tmp[7] = in[7];
	RC2 (cx->keyTable, out, in, 0);
	for (i = 0; i < 8; i++) {
	    out[i] = out[i] ^ cx->last[i];
	    cx->last[i] = tmp[i];
	}
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

/************************************************************************/

/* RC2 expands a key into a key table which is 64 UINT2s in size.
 */
RC2Context *RC2_CreateContext(unsigned char *key, unsigned keyLen,
		  unsigned char *iv, int mode, unsigned effectiveKeyLen)
{
  int i, j;
  unsigned char preTable[128];
  RC2Context *cx;

  cx = (RC2Context*) PORT_ZAlloc(sizeof(RC2Context));
  if (!cx) return cx;

  switch (mode) {
    case SEC_RC2:
      cx->enc = RC2_Enc;
      cx->dec = RC2_Dec;
      break;

    case SEC_RC2_CBC:
      PORT_Memcpy(cx->last, iv, 8);
      cx->enc = RC2CBC_Enc;
      cx->dec = RC2CBC_Dec;
      break;

    default:
      PORT_SetError(SEC_ERROR_INVALID_ARGS);
      PORT_Free(cx);
      return 0;
  }

  /* Initialize pre-table with key */
  PORT_Memcpy(preTable, key, keyLen);

  /* Expand key forward using pseudorandom permutation */
  for (i = keyLen; i < 128; i++)
    preTable[i] = PI_SUBST[(preTable[i - 1] + preTable[i - keyLen]) & 0xff];
  
  /* Establish effective key length by masking and expanding back.
   */
  preTable[128 - effectiveKeyLen] = PI_SUBST[preTable[128 - effectiveKeyLen]];
  for (i = 127 - effectiveKeyLen; i >= 0; i--) 
    preTable[i] =
      PI_SUBST[(preTable[i + 1] ^ preTable[i + effectiveKeyLen]) & 0xff];

  /* Store byte pre-table in key table */
  for (i = 0, j = 0; i < 64; i++, j += 2)
    cx->keyTable[i] = (((UINT2)preTable[j + 1]) << 8) | ((UINT2)preTable[j]);

  PORT_Memset (preTable, 0, sizeof (preTable));

  return cx;
}

void RC2_DestroyContext(RC2Context *cx, PRBool freeit)
{
    if (freeit) {
	PORT_ZFree(cx, sizeof(RC2Context));
    }
}

SECStatus RC2_Encrypt(RC2Context *cx, unsigned char *out, unsigned *part,
		     unsigned maxOut, unsigned char *in, unsigned inLen)
{
    if (inLen & 7) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    if (maxOut < inLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    return (*cx->enc)(cx, out, part, maxOut, in, inLen);
}

SECStatus RC2_Decrypt(RC2Context *cx, unsigned char *out, unsigned *part,
		     unsigned maxOut, unsigned char *in, unsigned inLen)
{
    if (inLen & 7) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    if (maxOut < inLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    return (*cx->dec)(cx, out, part, maxOut, in, inLen);
}
