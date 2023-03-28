/* Copyright (C) RSA Data Security, Inc. created 1994.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef __rsakeygn_h_
#define __rsakeygn_h_ 1

#include "cmp.h"

/* Need randomBlock to hold bytes for two prime numbers, each of
     length RSA_PRIME_BITS (modulusBits). Previous BSAFE versions
     relied on 16 bit words. In order to remain compatible, the
     following definition looks funny. */
#define RSA_KEY_GEN_RANDOM_BLOCK_LEN(modulusBits) \
	(4 * (((RSA_PRIME_BITS (modulusBits)) >> 4) + 1))

#define RSA_KEYGEN_MAX_TRIES 10

typedef struct RSAKeyGenContextStr RSAKeyGenContext;

struct RSAKeyGenContextStr {
  unsigned int modulusBits;
  CMPInt cmpModulus;
  CMPInt cmpPublicExponent;
  CMPInt cmpPrivateExponent;
  CMPInt cmpPrime1;
  CMPInt cmpPrime2;
  CMPInt cmpExponentP;
  CMPInt cmpExponentQ;
  CMPInt cmpCoefficient;
  SECKEYLowPrivateKey result;
  unsigned char *resultBuffer;           
  int bufSize;
};

typedef struct RSAKeyGenParamsStr RSAKeyGenParams;

struct RSAKeyGenParamsStr {
  unsigned int modulusBits;
  SECItem publicExponent;
};

RSAKeyGenContext *RSA_CreateKeyGenContext(RSAKeyGenParams *params);
void RSA_DestroyKeyGenContext(RSAKeyGenContext *context);
SECKEYLowPrivateKey *RSA_KeyGen(RSAKeyGenContext *context,
			  unsigned char *randomBlock);

#endif /* __rsakeygn_h_ */
