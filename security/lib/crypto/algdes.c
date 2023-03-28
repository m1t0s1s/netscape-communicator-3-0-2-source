/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
/* 90/05/09 (BK)  Changed SBOX entries to 16-bit integers (combining
     adjacent entries A B in AB order).
 */
#include "crypto.h"

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

struct DESContextStr {
    int encrypt;

    /* Key material for each of up to three keys */
    UINT2 k0[64];
    UINT2 k1[64];
    UINT2 k2[64];

    /* Carry forward for CBC */
    unsigned char last[8];

    SECStatus (*enc)(DESContext *cx, unsigned char *out, unsigned *part,
		    unsigned maxOut, unsigned char *in, unsigned inLen);
    SECStatus (*dec)(DESContext *cx, unsigned char *out, unsigned *part,
		    unsigned maxOut, unsigned char *in, unsigned inLen);
};

/* S_PC1 = 64 to 56-bit permutation to apply to key */
static unsigned char S_PC1[] = {
      57, 49, 41, 33, 25, 17,  9,                             /*  BITS OF C */
       1, 58, 50, 42, 34, 26, 18, 
      10,  2, 59, 51, 43, 35, 27, 
      19, 11,  3, 60, 52, 44, 36, 
      63, 55, 47, 39, 31, 23, 15,                             /*  BITS OF D */
       7, 62, 54, 46, 38, 30, 22, 
      14,  6, 61, 53, 45, 37, 29, 
      21, 13,  5, 28, 20, 12,  4,   0};                /* guard byte on end */


/* S_KA_PC2 = 48 to 64 bit permutation = KA o PC2 */
static unsigned char S_KA_PC2[] = {
  0, 23, 19, 12,  4, 26,  8,  0,  0, 14, 17, 11, 24,  1,  5,  0, 
  0, 44, 49, 39, 56, 34, 53,  0,  0, 41, 52, 31, 37, 47, 55,  0, 
  0, 16,  7, 27, 20, 13,  2,  0,  0,  3, 28, 15,  6, 21, 10,  0, 
  0, 46, 42, 50, 36, 29, 32,  0,  0, 30, 40, 51, 45, 33, 48,  0, 
  0
};

/* DES_SHE, DES_SHD = tables of shifts for enc/decryption */
/* LSH FOR ENCRYPT */
static unsigned char DES_SHE [] =
  {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1} ;
/* RSH FOR DECRYPT */
static unsigned char DES_SHD [] =
  {0, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1} ;

/* 8 sbox tables in order 1, 3, 5, 7, 2, 4, 6, 8.
   1024 = 8*2*64.
 */
static UINT2 SB_TAB[1024] = {
  0x0020, 0x0000, 0x0000, 0x8020, 0x8020, 0x8000, 0x8000, 0x0000, 
  0x0000, 0x0020, 0x8020, 0x0000, 0x8020, 0x8020, 0x0020, 0x8000, 
  0x8000, 0x0020, 0x0020, 0x0000, 0x0000, 0x0020, 0x0020, 0x8020, 
  0x8000, 0x8020, 0x8020, 0x8000, 0x0000, 0x8000, 0x8000, 0x0020, 
  0x0000, 0x8020, 0x8000, 0x0020, 0x0020, 0x0020, 0x0020, 0x0000, 
  0x8020, 0x0000, 0x0000, 0x8020, 0x0000, 0x8000, 0x8020, 0x8000, 
  0x8020, 0x8000, 0x0020, 0x8020, 0x8020, 0x8000, 0x8000, 0x0020, 
  0x8000, 0x0020, 0x0020, 0x0000, 0x8000, 0x0000, 0x0000, 0x8020, 
  0x2080, 0x0000, 0x2000, 0x2080, 0x2000, 0x2080, 0x0000, 0x2000, 
  0x0080, 0x2080, 0x2080, 0x0080, 0x0080, 0x2000, 0x0000, 0x0000, 
  0x0080, 0x0080, 0x0080, 0x2080, 0x2080, 0x2000, 0x2000, 0x0080, 
  0x2000, 0x0000, 0x0000, 0x2000, 0x0000, 0x0080, 0x2080, 0x0000, 
  0x2000, 0x2080, 0x0000, 0x2000, 0x2080, 0x0000, 0x0000, 0x0080, 
  0x2000, 0x2000, 0x2080, 0x0000, 0x0080, 0x0000, 0x0080, 0x2080, 
  0x2080, 0x2000, 0x2000, 0x0080, 0x0000, 0x0080, 0x2080, 0x2080, 
  0x0080, 0x0080, 0x0080, 0x0000, 0x2000, 0x2080, 0x0000, 0x2000, 
  0x1002, 0x1000, 0x0000, 0x0002, 0x0002, 0x0000, 0x1002, 0x1000, 
  0x1000, 0x1002, 0x1002, 0x1000, 0x1000, 0x0002, 0x0000, 0x1002, 
  0x0002, 0x0002, 0x1000, 0x0000, 0x1000, 0x0000, 0x0002, 0x1002, 
  0x0002, 0x1000, 0x0000, 0x0002, 0x0000, 0x1002, 0x1002, 0x0000, 
  0x0000, 0x0002, 0x1002, 0x0002, 0x1000, 0x1002, 0x1002, 0x0000, 
  0x1002, 0x1000, 0x0000, 0x1002, 0x0002, 0x0000, 0x0000, 0x1000, 
  0x0000, 0x1002, 0x0002, 0x1000, 0x0002, 0x1000, 0x1000, 0x0002, 
  0x0002, 0x0000, 0x1000, 0x0000, 0x1000, 0x1002, 0x1002, 0x0002, 
  0x1004, 0x1000, 0x1000, 0x1004, 0x0000, 0x0004, 0x0004, 0x1004, 
  0x0004, 0x1004, 0x1000, 0x0000, 0x1000, 0x0000, 0x0004, 0x0004, 
  0x1000, 0x0004, 0x1004, 0x0000, 0x0000, 0x1000, 0x1004, 0x0000, 
  0x0004, 0x0004, 0x0000, 0x1000, 0x1004, 0x1000, 0x0000, 0x1004, 
  0x0000, 0x1004, 0x0004, 0x0000, 0x1004, 0x0000, 0x1000, 0x1000, 
  0x0000, 0x1000, 0x0004, 0x1004, 0x1004, 0x0004, 0x1000, 0x0000, 
  0x1004, 0x1000, 0x0000, 0x0004, 0x0004, 0x1004, 0x0004, 0x0004, 
  0x1000, 0x0000, 0x1000, 0x1004, 0x0000, 0x0004, 0x1004, 0x1000, 
  0x0000, 0x0100, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000, 0x0100, 
  0x0000, 0x0100, 0x0100, 0x0000, 0x0100, 0x0000, 0x0100, 0x0000, 
  0x0100, 0x0000, 0x0100, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 
  0x0100, 0x0000, 0x0000, 0x0100, 0x0000, 0x0100, 0x0000, 0x0100, 
  0x0100, 0x0100, 0x0000, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 
  0x0000, 0x0000, 0x0100, 0x0100, 0x0100, 0x0000, 0x0000, 0x0100, 
  0x0100, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000, 0x0000, 0x0100, 
  0x0100, 0x0100, 0x0000, 0x0100, 0x0000, 0x0000, 0x0100, 0x0000, 
  0x0041, 0x4040, 0x0000, 0x4001, 0x0040, 0x0000, 0x4041, 0x0040, 
  0x4001, 0x0001, 0x0001, 0x4000, 0x4041, 0x4001, 0x4000, 0x0041, 
  0x0000, 0x0001, 0x4040, 0x0040, 0x4040, 0x4000, 0x4001, 0x4041, 
  0x0041, 0x4040, 0x4000, 0x0041, 0x0001, 0x4041, 0x0040, 0x0000, 
  0x4040, 0x0000, 0x4001, 0x0041, 0x4000, 0x4040, 0x0040, 0x0000, 
  0x0040, 0x4001, 0x4041, 0x0040, 0x0001, 0x0040, 0x0000, 0x4001, 
  0x0041, 0x4000, 0x0000, 0x4041, 0x0001, 0x4041, 0x4040, 0x0001, 
  0x4000, 0x0041, 0x0041, 0x4000, 0x4041, 0x0001, 0x4001, 0x4040, 
  0x2010, 0x2000, 0x2000, 0x0000, 0x0010, 0x2010, 0x2010, 0x2000, 
  0x0000, 0x0010, 0x0010, 0x2010, 0x2000, 0x0000, 0x0010, 0x2010, 
  0x2000, 0x0000, 0x0010, 0x2010, 0x0000, 0x0010, 0x2000, 0x0000, 
  0x2010, 0x2000, 0x0000, 0x0010, 0x0000, 0x0010, 0x2010, 0x2000, 
  0x0010, 0x2010, 0x0010, 0x2010, 0x2000, 0x0000, 0x0000, 0x0010, 
  0x0000, 0x0010, 0x2010, 0x2000, 0x2010, 0x2000, 0x2000, 0x0000, 
  0x2010, 0x2000, 0x2000, 0x0000, 0x2010, 0x2000, 0x0010, 0x2010, 
  0x2000, 0x0000, 0x0010, 0x2010, 0x0000, 0x0010, 0x0000, 0x0010, 
  0x0400, 0x0410, 0x0410, 0x0010, 0x0410, 0x0010, 0x0000, 0x0400, 
  0x0000, 0x0400, 0x0400, 0x0410, 0x0010, 0x0000, 0x0010, 0x0000, 
  0x0000, 0x0400, 0x0000, 0x0400, 0x0010, 0x0000, 0x0400, 0x0410, 
  0x0010, 0x0000, 0x0410, 0x0010, 0x0400, 0x0410, 0x0410, 0x0010, 
  0x0010, 0x0000, 0x0400, 0x0410, 0x0010, 0x0000, 0x0000, 0x0400, 
  0x0410, 0x0010, 0x0010, 0x0000, 0x0400, 0x0410, 0x0410, 0x0010, 
  0x0410, 0x0010, 0x0000, 0x0400, 0x0000, 0x0400, 0x0410, 0x0010, 
  0x0400, 0x0410, 0x0000, 0x0400, 0x0010, 0x0000, 0x0400, 0x0410, 
  0x0000, 0x0041, 0x0041, 0x0840, 0x0001, 0x0000, 0x0800, 0x0041, 
  0x0801, 0x0001, 0x0040, 0x0801, 0x0840, 0x0841, 0x0001, 0x0800, 
  0x0040, 0x0801, 0x0801, 0x0000, 0x0800, 0x0841, 0x0841, 0x0040, 
  0x0841, 0x0800, 0x0000, 0x0840, 0x0041, 0x0040, 0x0840, 0x0001, 
  0x0001, 0x0840, 0x0000, 0x0040, 0x0800, 0x0041, 0x0840, 0x0801, 
  0x0040, 0x0800, 0x0841, 0x0041, 0x0801, 0x0000, 0x0040, 0x0841, 
  0x0841, 0x0001, 0x0840, 0x0841, 0x0041, 0x0000, 0x0801, 0x0840, 
  0x0001, 0x0040, 0x0800, 0x0001, 0x0000, 0x0801, 0x0041, 0x0800, 
  0x0020, 0x0020, 0x0000, 0x0020, 0x0000, 0x0020, 0x0000, 0x0000, 
  0x0020, 0x0000, 0x0020, 0x0020, 0x0020, 0x0000, 0x0020, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0020, 0x0020, 0x0020, 
  0x0000, 0x0020, 0x0000, 0x0000, 0x0020, 0x0000, 0x0000, 0x0020, 
  0x0000, 0x0020, 0x0020, 0x0000, 0x0000, 0x0000, 0x0020, 0x0020, 
  0x0020, 0x0000, 0x0000, 0x0020, 0x0020, 0x0020, 0x0000, 0x0000, 
  0x0020, 0x0020, 0x0000, 0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0020, 0x0020, 0x0020, 0x0000, 0x0000, 0x0000, 0x0020, 0x0020, 
  0x0400, 0x0408, 0x0000, 0x0408, 0x0408, 0x0000, 0x0408, 0x0008, 
  0x0400, 0x0008, 0x0008, 0x0400, 0x0008, 0x0400, 0x0400, 0x0000, 
  0x0000, 0x0008, 0x0400, 0x0000, 0x0008, 0x0400, 0x0000, 0x0408, 
  0x0408, 0x0000, 0x0008, 0x0408, 0x0000, 0x0008, 0x0408, 0x0400, 
  0x0400, 0x0000, 0x0408, 0x0008, 0x0408, 0x0008, 0x0000, 0x0400, 
  0x0008, 0x0400, 0x0400, 0x0000, 0x0400, 0x0408, 0x0008, 0x0408, 
  0x0008, 0x0408, 0x0000, 0x0408, 0x0000, 0x0000, 0x0408, 0x0008, 
  0x0000, 0x0008, 0x0400, 0x0000, 0x0408, 0x0400, 0x0008, 0x0400, 
  0x0002, 0x0000, 0x0800, 0x0802, 0x0000, 0x0002, 0x0802, 0x0000, 
  0x0800, 0x0802, 0x0000, 0x0002, 0x0002, 0x0800, 0x0000, 0x0802, 
  0x0000, 0x0002, 0x0802, 0x0800, 0x0800, 0x0802, 0x0002, 0x0002, 
  0x0002, 0x0000, 0x0802, 0x0800, 0x0802, 0x0800, 0x0800, 0x0000, 
  0x0800, 0x0002, 0x0002, 0x0800, 0x0802, 0x0000, 0x0802, 0x0002, 
  0x0000, 0x0800, 0x0000, 0x0802, 0x0002, 0x0802, 0x0800, 0x0000, 
  0x0802, 0x0800, 0x0000, 0x0002, 0x0002, 0x0800, 0x0000, 0x0802, 
  0x0800, 0x0002, 0x0802, 0x0000, 0x0800, 0x0000, 0x0002, 0x0802, 
  0x0004, 0x4084, 0x4080, 0x0000, 0x0000, 0x4080, 0x4004, 0x0084, 
  0x4084, 0x0004, 0x0000, 0x4080, 0x4000, 0x0080, 0x4084, 0x4000, 
  0x0080, 0x4004, 0x4004, 0x0080, 0x4080, 0x0084, 0x0084, 0x4004, 
  0x0084, 0x0000, 0x4000, 0x4084, 0x0004, 0x4000, 0x0080, 0x0004, 
  0x0080, 0x0004, 0x0004, 0x4080, 0x4080, 0x4084, 0x4084, 0x4000, 
  0x4004, 0x0080, 0x0080, 0x0004, 0x0084, 0x4000, 0x4004, 0x0084, 
  0x4000, 0x4080, 0x4084, 0x0084, 0x0004, 0x0000, 0x4000, 0x4084, 
  0x0000, 0x4004, 0x0084, 0x0000, 0x4080, 0x0080, 0x0000, 0x4004, 
  0x0000, 0x0000, 0x0100, 0x0000, 0x0100, 0x0100, 0x0100, 0x0100, 
  0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 
  0x0100, 0x0100, 0x0000, 0x0100, 0x0000, 0x0000, 0x0100, 0x0000, 
  0x0000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000, 0x0000, 0x0100, 
  0x0000, 0x0100, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0100, 0x0000, 0x0100, 0x0100, 0x0100, 0x0100, 
  0x0100, 0x0000, 0x0100, 0x0000, 0x0100, 0x0000, 0x0000, 0x0100, 
  0x0000, 0x0100, 0x0000, 0x0100, 0x0000, 0x0100, 0x0100, 0x0000, 
  0x0200, 0x0000, 0x0000, 0x0200, 0x0200, 0x0200, 0x0000, 0x0200, 
  0x0000, 0x0200, 0x0200, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000, 
  0x0200, 0x0200, 0x0200, 0x0000, 0x0000, 0x0000, 0x0200, 0x0200, 
  0x0000, 0x0000, 0x0000, 0x0200, 0x0200, 0x0200, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 
  0x0200, 0x0000, 0x0200, 0x0200, 0x0200, 0x0200, 0x0000, 0x0200, 
  0x0000, 0x0200, 0x0000, 0x0200, 0x0200, 0x0200, 0x0200, 0x0000, 
  0x0200, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0200, 0x0200, 
  0x0208, 0x0200, 0x8000, 0x8208, 0x0000, 0x0208, 0x0008, 0x0000, 
  0x8008, 0x8000, 0x8208, 0x8200, 0x8200, 0x8208, 0x0200, 0x0008, 
  0x8000, 0x0008, 0x0200, 0x0208, 0x8200, 0x8008, 0x8008, 0x8200, 
  0x0208, 0x0000, 0x0000, 0x8008, 0x0008, 0x0200, 0x8208, 0x8000, 
  0x8208, 0x8000, 0x8200, 0x0200, 0x0008, 0x8008, 0x0200, 0x8208, 
  0x0200, 0x0008, 0x0008, 0x8000, 0x8008, 0x0000, 0x8000, 0x0208, 
  0x0000, 0x8208, 0x8008, 0x0008, 0x8000, 0x0200, 0x0208, 0x0000, 
  0x8208, 0x8200, 0x8200, 0x0208, 0x0208, 0x8008, 0x0000, 0x8200
};

/* Do an 8 byte DES encryption.
 */
static void DES(UINT2 *keyTable, unsigned char *out, unsigned char *in)
{
  UINT2 cx, dx, old_cx, new_cx, old_dx, new_dx, keyWord, keyIndex, ax, bx,
    new_ax, new_bx, tempword, *sboxPtr1, *sboxPtr2, mask = 1;
  int i, roundCount;
  unsigned char al = 0, ah = 0, bl = 0, bh = 0, cl = 0, ch = 0, dl = 0,
    dh = 0,                 /* zeroize even though all bits are shifted out */
    tempChar;

  /*
   * initial permutation code
   * result goes into ah al bh bl ch cl dh dl.
   *                   2  4  6  8  1  3  5  7    <-- right most bits (LSB's)
   *                  10 12 14 16  9 11 13 15
   * input            18 20 22 24 17 19 21 23
   *                  26 28 30 32 25 27 29 31
   * bits             34 36 38 40 33 35 37 39
   *                  42 44 46 48 41 43 45 47
   *                  50 52 54 56 49 51 53 55
   *                  58 60 62 64 57 59 61 63    <-- MSB's
   */
  for (i = 8; i > 0; i--) {
    tempChar = *in++;

    bl >>= 1;
    bl &= 0x7f;
    bl |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    dl >>= 1;
    dl &= 0x7f;
    dl |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    bh >>= 1;
    bh &= 0x7f;
    bh |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    dh >>= 1;
    dh &= 0x7f;
    dh |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    al >>= 1;
    al &= 0x7f;
    al |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    cl >>= 1;
    cl &= 0x7f;
    cl |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    ah >>= 1;
    ah &= 0x7f;
    ah |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;

    ch >>= 1;
    ch &= 0x7f;
    ch |= (tempChar & 1 ? 0x80 : 0);
    tempChar >>= 1;
  }

  /* left */
  new_ax = (UINT2)((ah << 8) & 0xff00) + (UINT2)(al & 0xff);
  new_bx = (UINT2)((bh << 8) & 0xff00) + (UINT2)(bl & 0xff);
  /* right */
  new_cx = (UINT2)((ch << 8) & 0xff00) + (UINT2)(cl & 0xff);
  new_dx = (UINT2)((dh << 8) & 0xff00) + (UINT2)(dl & 0xff);

  /* rotate cx, dx right by 2 */
  cx = ((new_cx >> 2) & 0x3fff) | ((new_dx << 14) & 0xc000);
  dx = ((new_dx >> 2) & 0x3fff) | ((new_cx << 14) & 0xc000);
  
  /* rotate ax, bx right by 2 */
  ax = ((new_ax >> 2) & 0x3fff) | ((new_bx << 14) & 0xc000);
  bx = ((new_bx >> 2) & 0x3fff) | ((new_ax << 14) & 0xc000);

  /* temporary segment registers */
  sboxPtr1 = SB_TAB;         /* beginning of sbox table */
  sboxPtr2 = sboxPtr1 + 64;  /* 64 entries later */
  
  for (roundCount = 16; roundCount; roundCount--) {
    /*   assume left  half in [ax, bx] */
    /*   assume right half in [cx, dx] */
    old_cx = cx;             /* save cx and dx */
    old_dx = dx;
  
    /* prepare for sboxes 1, 3 */
    keyWord = *keyTable++; 
    /* 0111111001111110 */
    keyWord = (keyWord ^ cx) & 0x7e7e;        

    /* sbox 1 */
    keyIndex = 0x0000 + ((keyWord >> 8) & 0xff); 
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];

    /* sbox 3 */
    keyIndex = 0x200 + (keyWord & 0xff);        
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];

    /* prepare for sboxes 5, 7 */
    keyWord = *keyTable++; 
    /* 0111111001111110 */
    keyWord = (keyWord ^ dx) & 0x7e7e;        

    /* sbox 5 */
    keyIndex = 0x400 + ((keyWord >> 8) & 0xff); 
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];
  
    /* sbox 7 */
    keyIndex = 0x600 + (keyWord & 0xff);      
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];

    /* rotate cx, dx left by 4 bits */
    new_cx = ((cx << 4) & 0xfff0) | ((dx >> 12) & 0xf);
    new_dx = ((dx << 4) & 0xfff0) | ((cx >> 12) & 0xf);
    cx = new_cx;
    dx = new_dx;

    /* prepare for sboxes 2, 4 */
    keyWord = *keyTable++; 
    /* 0111111001111110 */
    keyWord = (keyWord ^ cx) & 0x7e7e;        

    /* sbox 2 */
    keyIndex = 0x100 + ((keyWord >> 8) & 0xff); 
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];
  
    /* sbox 4 */
    keyIndex = 0x300 + (keyWord & 0xff);      
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];

    /* prepare for sboxes 6, 8 */
    keyWord = *keyTable++; 
    /* 0111111001111110 */
    keyWord = (keyWord ^ dx) & 0x7e7e;        

    /* sbox 6 */
    keyIndex = 0x500 + ((keyWord >> 8) & 0xff); 
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];

    /* sbox 8 */
    keyIndex = 0x700 + (keyWord & 0xff);       
    ax ^= sboxPtr1[keyIndex >> 1];
    bx ^= sboxPtr2[keyIndex >> 1];

    /* restore cx and dx */
    cx = old_cx;             
    dx = old_dx;
    
    /* swap halves */
    tempword = ax; ax = cx; cx = tempword;       
    tempword = bx; bx = dx; dx = tempword;
  }
  /* end of round */

  /* rotate cx, dx left by 2 bits */
  new_cx = ((cx << 2) & 0xfffc) | ((dx >> 14) & 0x3);
  new_dx = ((dx << 2) & 0xfffc) | ((cx >> 14) & 0x3);

  /* rotate ax, bx left by 2 bits */
  new_ax = ((ax << 2) & 0xfffc) | ((bx >> 14) & 0x3);
  new_bx = ((bx << 2) & 0xfffc) | ((ax >> 14) & 0x3);
  
  /* split back into bytes */
  al = (unsigned char)(new_ax & 0xff);         
  ah = (unsigned char)((new_ax >> 8) & 0xff);
  bl = (unsigned char)(new_bx & 0xff);
  bh = (unsigned char)((new_bx >> 8) & 0xff);
  cl = (unsigned char)(new_cx & 0xff);
  ch = (unsigned char)((new_cx >> 8) & 0xff);
  dl = (unsigned char)(new_dx & 0xff);
  dh = (unsigned char)((new_dx >> 8) & 0xff);

  /*
   * final permutation code -- inverse initial permutation
   * includes ax-cx and bx-dx swap to undo final swap
   * assumes input in cl ch dl dh al ah bl bh = bits 1...64
   * output is of form:
   *             40  8 48 16 56 24 64 32
   *             39  7 47 15 55 23 63 31
   *             38  6 46 14 54 22 62 30
   *             37  5 45 13 53 21 61 29
   *             36  4 44 12 52 20 60 28
   *             35  3 43 11 51 19 59 27
   *             34  2 42 10 50 18 58 26
   *             33  1 41  9 49 17 57 25
   */
  for (i = 8; i; i--) {
    *out = 0;
    *out |= (dl & mask ? 1 : 0);
    *out |= (bl & mask ? 2 : 0);
    *out |= (dh & mask ? 4 : 0);
    *out |= (bh & mask ? 8 : 0);
    *out |= (cl & mask ? 16 : 0);
    *out |= (al & mask ? 32 : 0);
    *out |= (ch & mask ? 64 : 0);
    *out |= (ah & mask ? 128 : 0);
    out++;
    mask *= 2;
  }
}

/* In an effort to do away with one-origin indexing it is assumed that
     selectors still refer to one-origin arrays but the translation
     to zero-origin arrays is done within the Select routine.
 */
static void Select (unsigned char *dest, unsigned char *selector,
		    unsigned char *source, int count)
{
  int i;
  
  for (i=0; i<count; i++)
    if (selector[i] == 0)
      dest[i] = 0;
    else
      dest[i] = source[selector[i]-1];
}

/* In-place left-shift 56-bit field by halves.
 */
static void LShift1 (unsigned char *reg)
{
  unsigned char x;
  
  x = reg[0];
  PORT_Memmove(&reg[0], &reg[1], 27);
  reg[27] = x;
  x = reg[28];
  PORT_Memmove(&reg[28], &reg[29], 27);
  reg[55] = x;
}

/* In-place left-shift by 2 in 56-bit field by halves.
 */
static void LShift2 (unsigned char *reg)
{
  unsigned char x, y;
  
  x = reg[0];
  y = reg[1];
  PORT_Memmove (&reg[0], &reg[2], 26);
  reg[26] = x;
  reg[27] = y;
  x = reg[28];
  y = reg[29];
  PORT_Memmove (&reg[28], &reg[30], 26);
  reg[54] = x;
  reg[55] = y;
}

/* Right shift 56-bit field in place by halves.
 */
static void RShift1 (unsigned char *reg)
{
  unsigned char x;
  
  x = reg[27];
  PORT_Memmove (&reg[1], &reg[0], 27);
  reg[0] = x;
  x = reg[55];
  PORT_Memmove (&reg[29], &reg[28], 27);
  reg[28] = x;
}

/* Right shift 56-bit field by two in halves.
 */
static void RShift2 (unsigned char *reg)
{
  unsigned char x, y;
  
  x=reg[26];
  y=reg[27];
  PORT_Memmove (&reg[2], &reg[0], 26);
  reg[0]=x;
  reg[1]=y;
  x=reg[54];
  y=reg[55];
  PORT_Memmove (&reg[30], &reg[28], 26);
  reg[28]=x;
  reg[29]=y;
}


/* Set the 8 byte key to odd parity.
 */
static void SetDESParity (unsigned char *dstkey, unsigned char *srckey)
{
  unsigned char mask;
  unsigned int i, bitCount;
  
  for (i = 0; i < 8; i++) {
    bitCount = 0;
    for (mask = 0x80; mask > 0x01; mask >>= 1) {
      if (mask & srckey[i])
        bitCount ++;
    }
    
    if (bitCount & 0x01)
      /* there are already an odd number of 1 bits in the top 7 bits */
      dstkey[i] = (unsigned char)(srckey[i] & 0xfe);
    else
      dstkey[i] = (unsigned char)(srckey[i] | 1);
  }
}


/* This routine will preprocess and `setup' a key.
   KeyTable is a pointer to a 128 byte block.
   Key is a pointer to an eight-byte key variable
   Encrypt_flag is non-zero iff we are encrypting
 */
static void DESKey(UINT2 *keyTable, unsigned char *rawkey, PRBool enc)
{
  int roundCount,i,j;
  unsigned char *charPtr,
    uchar1 = 0, uchar2 = 0, /* zeroize even though all bits are shifted out */
    u_key[64],                                          /* unpacked des key */
    u_cidi[56],                             /* unpacked ci-di at each round */
    u_ki[64],                                 /* unpacked ki to xor with ri */
    pkey[8], *key;

  SetDESParity(key = pkey, rawkey);

  /* UnPack key: For each bit in key, write a byte in u_key with the value
       of the bit.
   */
  charPtr = u_key;
  for (i=8; i; i--) {
    for (j=7; j>=0; j--)
      *charPtr++ = (unsigned char)((*key >> j) & 1);
    key++;
  }

  /* Move unpacked into u_cidi, selecting bytes from S_PC1.
   */
  Select  (u_cidi, S_PC1, u_key, 56);

  for (roundCount = 0;roundCount < 16;roundCount++) {
    /* Shift key right or left by one or two.
     */
    if (enc) {
      /* shifting left (for encryption) */
      if (DES_SHE[roundCount] == 1)
        LShift1 (u_cidi);
      else if (DES_SHE[roundCount] == 2)
        LShift2 (u_cidi);
    }
    else {
      /* shifting right (for decryption) */
      if (DES_SHD[roundCount] == 1)
        RShift1 (u_cidi);
      else if (DES_SHD[roundCount] == 2)
        RShift2 (u_cidi);
    }
    
    /* permute key bits for this round for xoring later */
    Select (u_ki, S_KA_PC2, u_cidi, 64);

    /* Read LSBits in source bit-array (u_ki) and pack them 16/word into
         the destination array (keyTable). 1st 8 bits go into LSByte, 2nd
         8 bits go into MSByte.
     */
    charPtr = u_ki;

    for (i=4; i; i--) {
      for (j=8; j; j--)
        uchar1 = (unsigned char) ((uchar1 << 1) | (*charPtr++ & 1));
      for (j=8; j; j--)
        uchar2 = (unsigned char) ((uchar2 << 1) | (*charPtr++ & 1));
      
      *keyTable++ = (UINT2)(uchar1 + ((UINT2)uchar2 << 8));
    }
  }
}

/************************************************************************/

/*
** Perform basic DES encryption/decryption
*/
static SECStatus DES_Crypt(DESContext *cx, unsigned char *out,
			  unsigned *part, unsigned maxOut,
			  unsigned char *in, unsigned inLen)
{
    *part = inLen;
    while (inLen) {
	DES (cx->k0, out, in);
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

/************************************************************************/

/*
** Perform DES-CBC encryption
*/
static SECStatus DESCBC_Encrypt(DESContext *cx, unsigned char *out,
			       unsigned *part, unsigned maxOut,
			       unsigned char *in, unsigned inLen)
{
    int i;

    *part = inLen;
    while (inLen) {
	for (i = 0; i < 8; i++) {
	    out[i] = in[i] ^ cx->last[i];
	}
	DES (cx->k0, out, out);
	for (i = 0; i < 8; i++) {
	    cx->last[i] = out[i];
	}
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

/*
** Perform DES-CBC decryption
*/
static SECStatus DESCBC_Decrypt(DESContext *cx, unsigned char *out,
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
	DES (cx->k0, out, in);
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

/*
** Perform basic DES-EDE3 encryption/decryption
*/
static SECStatus DESEDE3_Crypt(DESContext *cx, unsigned char *out,
			      unsigned *part, unsigned maxOut,
			      unsigned char *in, unsigned inLen)
{
    *part = inLen;
    while (inLen) {
	DES(cx->k0, out, in);
	DES(cx->k1, out, out);
	DES(cx->k2, out, out);
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

/************************************************************************/

/*
** Perform DES-EDE3-CBC encryption
*/
static SECStatus DESEDE3CBC_Encrypt(DESContext *cx, unsigned char *out,
				   unsigned *part, unsigned maxOut,
				   unsigned char *in, unsigned inLen)
{
    int i;

    *part = inLen;
    while (inLen) {
	for (i = 0; i < 8; i++) {
	    out[i] = in[i] ^ cx->last[i];
	}
	DES (cx->k0, out, out);
	DES (cx->k1, out, out);
	DES (cx->k2, out, out);
	for (i = 0; i < 8; i++) {
	    cx->last[i] = out[i];
	}
	out += 8;
	in += 8;
	inLen -= 8;
    }
    return SECSuccess;
}

/*
** Perform DES-EDE3-CBC decryption
*/
static SECStatus DESEDE3CBC_Decrypt(DESContext *cx, unsigned char *out,
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
	DES (cx->k0, out, in);
	DES (cx->k1, out, out);
	DES (cx->k2, out, out);
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

DESContext *DES_CreateContext(unsigned char *key, unsigned char *iv,
			      int mode, PRBool encrypt)
{
    DESContext *cx;

    cx = (DESContext*) PORT_ZAlloc(sizeof(DESContext));
    if (cx) {
	switch (mode) {
	  case SEC_DES:
	    cx->dec = cx->enc = DES_Crypt;
	    DESKey(cx->k0, key, encrypt);
	    break;

	  case SEC_DES_CBC:
	    cx->dec = DESCBC_Decrypt;
	    cx->enc = DESCBC_Encrypt;
	    PORT_Memcpy(cx->last, iv, 8);
	    DESKey(cx->k0, key, encrypt);
	    break;

	  case SEC_DES_EDE3:
	    cx->dec = cx->enc = DESEDE3_Crypt;

	  produce_ede3_keys:
	    if (encrypt) {
		DESKey(cx->k0, key, PR_TRUE);
		DESKey(cx->k1, key+8, PR_FALSE);
		DESKey(cx->k2, key+16, PR_TRUE);
	    } else {
		/* Reverse order and sense of keys so we can use the same
		   code */
		DESKey(cx->k0, key+16, PR_FALSE);
		DESKey(cx->k1, key+8, PR_TRUE);
		DESKey(cx->k2, key, PR_FALSE);
	    }
	    break;

	  case SEC_DES_EDE3_CBC:
	    cx->dec = DESEDE3CBC_Decrypt;
	    cx->enc = DESEDE3CBC_Encrypt;
	    PORT_Memcpy(cx->last, iv, 8);
	    goto produce_ede3_keys;
	    break;

	  default:
	    PORT_Assert(0);	/* invalid/unsupported/unknown mode */
	    PORT_ZFree(cx, sizeof(DESContext));
	    return 0;
	}
	cx->encrypt = encrypt;
    }
    return cx;
}

void DES_DestroyContext(DESContext *cx, PRBool freeit)
{
    if (freeit) {
	PORT_ZFree(cx, sizeof(DESContext));
    }
}

SECStatus DES_Encrypt(DESContext *cx, unsigned char *out,
		     unsigned int *part, unsigned int maxOut,
		     unsigned char *in, unsigned int inLen)
{
    PORT_Assert((inLen & 7) == 0);	/* must be multiple of 8 */
    PORT_Assert(maxOut >= inLen);		/* check for enough room */

    return (*cx->enc)(cx, out, part, maxOut, in, inLen);
}

SECStatus DES_Decrypt(DESContext *cx, unsigned char *out,
		     unsigned int *part, unsigned int maxOut,
		     unsigned char *in, unsigned int inLen)
{
    PORT_Assert((inLen & 7) == 0);	/* must be multiple of 8 */
    PORT_Assert(maxOut >= inLen);		/* check for enough room */

    return (*cx->dec)(cx, out, part, maxOut, in, inLen);
}

extern int SEC_ERROR_NO_MEMORY;

/*
 * Prepare a buffer for DES encryption, growing to the appropriate boundary,
 * filling with the appropriate padding.
 *
 * NOTE: If arena is non-NULL, we re-allocate from there, otherwise
 * we assume (and use) XP memory (re)allocation.
 */
unsigned char *
DES_PadBuffer(PRArenaPool *arena, unsigned char *inbuf, unsigned int inlen,
	      unsigned int *outlen)
{
    unsigned int des_len;
    unsigned char des_pad_len;
    unsigned char *outbuf;
    int i;

    /*
     * We need from 1 to DES_KEY_LENGTH bytes -- we *always* grow.
     * The extra bytes contain the value of the length of the padding:
     * if we have 2 bytes of padding, then the padding is "0x02, 0x02".
     */
    des_len = (inlen + DES_KEY_LENGTH) & ~(DES_KEY_LENGTH - 1);

    if (arena != NULL) {
	outbuf = PORT_ArenaGrow (arena, inbuf, inlen, des_len);
    } else {
	outbuf = PORT_Realloc (inbuf, des_len);
    }

    if (outbuf == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    des_pad_len = des_len - inlen;
    for (i = inlen; i < des_len; i++)
	outbuf[i] = des_pad_len;

    *outlen = des_len;
    return outbuf;
}
