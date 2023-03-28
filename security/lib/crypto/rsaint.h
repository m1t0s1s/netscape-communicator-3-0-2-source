#ifndef __sec_rsat_h_
#define __sec_rsat_h_

/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

struct RSAPublicContextStr {
  unsigned int blockLen;          /* total size for the block to be computed */
  unsigned char *input;
  unsigned int inputLen;
  CMPInt modulus;
  CMPInt publicExponent;
};

struct RSAPrivateContextStr {
  int mode;
  unsigned int blockLen;           /* total size of the block to be computed */
  unsigned char *input;
  unsigned int inputLen;
  CMPInt modulus;
  CMPInt publicExponent;
  CMPInt privateExponent;
  CMPInt prime[2];
  CMPInt primeExponent[2];
  CMPInt coefficient;
};

#endif /* __sec_rsat_h_ */
