/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
#include "sechash.h"

struct MD2ContextStr {
  unsigned char state[16];                                 /* state */
  unsigned char checksum[16];                           /* checksum */
  unsigned int count;                 /* number of bytes, modulo 16 */
  unsigned char buffer[16];                         /* input buffer */
};

/* Permutation of 0..255 constructed from the digits of pi. It gives a
   "random" nonlinear byte substitution operation.
 */
static unsigned char PI_SUBST[256] = {
  41, 46, 67, 201, 162, 216, 124, 1, 61, 54, 84, 161, 236, 240, 6,
  19, 98, 167, 5, 243, 192, 199, 115, 140, 152, 147, 43, 217, 188,
  76, 130, 202, 30, 155, 87, 60, 253, 212, 224, 22, 103, 66, 111, 24,
  138, 23, 229, 18, 190, 78, 196, 214, 218, 158, 222, 73, 160, 251,
  245, 142, 187, 47, 238, 122, 169, 104, 121, 145, 21, 178, 7, 63,
  148, 194, 16, 137, 11, 34, 95, 33, 128, 127, 93, 154, 90, 144, 50,
  39, 53, 62, 204, 231, 191, 247, 151, 3, 255, 25, 48, 179, 72, 165,
  181, 209, 215, 94, 146, 42, 172, 86, 170, 198, 79, 184, 56, 210,
  150, 164, 125, 182, 118, 252, 107, 226, 156, 116, 4, 241, 69, 157,
  112, 89, 100, 113, 135, 32, 134, 91, 207, 101, 230, 45, 168, 2, 27,
  96, 37, 173, 174, 176, 185, 246, 28, 70, 97, 105, 52, 64, 126, 15,
  85, 71, 163, 35, 221, 81, 175, 58, 195, 92, 249, 206, 186, 197,
  234, 38, 44, 83, 13, 110, 133, 40, 132, 9, 211, 223, 205, 244, 65,
  129, 77, 82, 106, 220, 55, 200, 108, 193, 171, 250, 36, 225, 123,
  8, 12, 189, 177, 74, 120, 136, 149, 139, 227, 99, 232, 109, 233,
  203, 213, 254, 59, 0, 29, 57, 242, 239, 183, 14, 102, 88, 208, 228,
  166, 119, 114, 248, 235, 117, 75, 10, 49, 68, 80, 180, 143, 237,
  31, 26, 219, 153, 141, 51, 159, 17, 131, 20
};

static unsigned char PADDING1[] = {1};
static unsigned char PADDING2[] = {2, 2};
static unsigned char PADDING3[] = {3, 3, 3};
static unsigned char PADDING4[] = {4, 4, 4, 4};
static unsigned char PADDING5[] = {5, 5, 5, 5, 5};
static unsigned char PADDING6[] = {6, 6, 6, 6, 6, 6};
static unsigned char PADDING7[] = {7, 7, 7, 7, 7, 7, 7};
static unsigned char PADDING8[] = {8, 8, 8, 8, 8, 8, 8, 8};
static unsigned char PADDING9[] = {9, 9, 9, 9, 9, 9, 9, 9, 9};
static unsigned char PADDING10[] =
  {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
static unsigned char PADDING11[] =
  {11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11};
static unsigned char PADDING12[] =
  {12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
static unsigned char PADDING13[] =
  {13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
static unsigned char PADDING14[] =
  {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14};
static unsigned char PADDING15[] =
  {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static unsigned char PADDING16[] =
  {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};

static unsigned char *PADDING[] = {
  (unsigned char *)0, PADDING1, PADDING2, PADDING3, PADDING4, PADDING5,
  PADDING6, PADDING7, PADDING8, PADDING9, PADDING10, PADDING11, PADDING12,
  PADDING13, PADDING14, PADDING15, PADDING16
};

/* MD2 basic transformation. Transforms state and updates checksum
     based on block.
 */
static void
MD2Transform(unsigned char state[16], unsigned char checksum[16],
	     const unsigned char block[16], unsigned char x[48])
{
  unsigned int i, j, t;
  
  /* Form encryption block from state, block, state ^ block.
   */
  PORT_Memcpy (x, state, 16);
  PORT_Memcpy (x+16, block, 16);
  for (i = 0; i < 16; i++)
    x[i+32] = state[i] ^ block[i];
  
  /* Encrypt block (18 rounds).
   */
  t = 0;
  for (i = 0; i < 18; i++) {
    for (j = 0; j < 48; j++)
      t = x[j] ^= PI_SUBST[t];
    t = (t + i) & 0xff;
  }

  /* Save new state */
  PORT_Memcpy (state, x, 16);

  /* Update checksum.
   */
  t = checksum[15];
  for (i = 0; i < 16; i++)
    t = checksum[i] ^= PI_SUBST[block[i] ^ t];
}

MD2Context *
MD2_NewContext(void)
{
    MD2Context *cx;
    cx = (MD2Context*) PORT_ZAlloc(sizeof(MD2Context));
    return cx;
}

MD2Context *
MD2_CloneContext(MD2Context *cx)
{
    MD2Context *clone;
    clone = MD2_NewContext();
    if (clone)
	*clone = *cx;
    return clone;
}

void
MD2_DestroyContext(MD2Context *cx, PRBool freeit)
{
    if (freeit) {
	PORT_ZFree(cx, sizeof(MD2Context));
    }
}

/*
 * MD2 initialization. Begins an MD2 operation, writing a new context.
 */
void
MD2_Begin(MD2Context *cx)
{
  PORT_Memset(cx, 0, sizeof(MD2Context));
}

/*
 * MD2 block update operation. Continues an MD2 message-digest
 *   operation, processing another message block, and updating the
 *   context.
 */
void
MD2_Update(MD2Context *cx, const unsigned char *input, unsigned inputLen)
{
  unsigned int i, index, partLen;
  unsigned char x[48];

  /* Update number of bytes mod 16 */
  index = cx->count;
  cx->count = (index + inputLen) & 0xf;
  
  partLen = 16 - index;

  /* Transform as many times as possible.
    */
  if (inputLen >= partLen) {
    PORT_Memcpy(&cx->buffer[index], input, partLen);
    MD2Transform(cx->state, cx->checksum, cx->buffer, x);

    for (i = partLen; i + 15 < inputLen; i += 16)
      MD2Transform (cx->state, cx->checksum, &input[i], x);

    index = 0;
  }
  else
    i = 0;
  
  /* Buffer remaining input */
  PORT_Memcpy(&cx->buffer[index], &input[i], inputLen-i);

  /* Clear sensitive information.  */
  PORT_Memset (x, 0, sizeof (x));
}

/*
 * MD2 finalization.
 * Ends an MD2 message-digest operation, writing the message digest.
 */
void
MD2_End(MD2Context *cx, unsigned char *digest,
	unsigned int *pDigestLen, unsigned int maxDigestLen)
{
  unsigned int index, padLen;

  PORT_Assert (maxDigestLen >= MD2_LENGTH);

  /*
   * Pad out to multiple of MD2_LENGTH.
   */
  index = cx->count;
  padLen = MD2_LENGTH - index;
  MD2_Update (cx, PADDING[padLen], padLen);

  /* Extend with checksum */
  MD2_Update (cx, cx->checksum, MD2_LENGTH);
  
  /* Store state in digest */
  PORT_Memcpy (digest, cx->state, MD2_LENGTH);

  *pDigestLen = MD2_LENGTH;
}

SECStatus
MD2_Hash(unsigned char *dest, const char *src)
{
    MD2Context *cx;
    unsigned part;

    cx = MD2_NewContext();
    if (cx == NULL)
	return SECFailure;

    MD2_Begin(cx);
    MD2_Update(cx, (const unsigned char *)src, PORT_Strlen(src));
    MD2_End(cx, dest, &part, MD2_LENGTH);
    MD2_DestroyContext(cx, PR_TRUE);

    return SECSuccess;
}
