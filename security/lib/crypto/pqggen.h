/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _PQGGEN_H_
#define _PQGGEN_H_ 1

/* Maximum, minimum lengths of prime and subprime.
     Requires A_MAX_PQG_PRIME_BITS <= the larger of
     BSAFE_MAX_DSA_KEY_SIZE_BITS and BSAFE_MAX_DH_KEY_SIZE_BITS
     and A_MIN_PQG_PRIME_BITS >= the smaller of
     BSAFE_MIN_DSA_KEY_SIZE_BITS and BSAFE_MIN_DH_KEY_SIZE_BITS
     See keysize.h
 */
#define MAX_DH_KEY_SIZE_BITS MAX_DSA_KEY_SIZE_BITS
#define MIN_DH_KEY_SIZE_BITS MIN_DSA_KEY_SIZE_BITS

#if MAX_DSA_KEY_SIZE_BITS > MAX_DH_KEY_SIZE_BITS
#define MAX_PQG_PRIME_BITS     MAX_DSA_KEY_SIZE_BITS
#else
#define MAX_PQG_PRIME_BITS     MAX_DH_KEY_SIZE_BITS
#endif

#if MIN_DSA_KEY_SIZE_BITS < MIN_DH_KEY_SIZE_BITS
#define MIN_PQG_PRIME_BITS     MIN_DSA_KEY_SIZE_BITS
#else
#define MIN_PQG_PRIME_BITS     MIN_DH_KEY_SIZE_BITS
#endif

#define MAX_PQG_SUBPRIME_BITS  (MAX_PQG_PRIME_BITS - 1)
#define MIN_PQG_SUBPRIME_BITS  128

#define MAX_PQG_PRIME_LEN      BITS_TO_LEN(MAX_PQG_PRIME_BITS)
#define MAX_PQG_SUBPRIME_LEN   BITS_TO_LEN(MAX_PQG_SUBPRIME_BITS)

/* Supply an unsigned char * random block of this length: */
#define PQG_GEN_RANDOM_BLOCK_LEN(primeBits) (2 * BITS_TO_LEN(primeBits))

typedef struct {
  int initialized;
  unsigned int primeBits;
  unsigned int subPrimeBits;
} PQGParamGenContext;

PQGParamGenContext *PQG_CreateParamGenContext(unsigned int primeBits,
					      unsigned int subPrimeBits);
SECStatus PQG_ParamGen(PQGParamGenContext *context, PQGParams **params,
		       unsigned char *randomBlock);
void PQG_DestroyParamGenContext(PQGParamGenContext *context);

#endif
