/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _DSA_H_
#define _DSA_H_ 1

#include "cmppriv.h"

/* Maximum, minimum lengths of DSA prime, and length of DSA subprime.
 */
#define MAX_DSA_PRIME_BITS   2048
#define MAX_DSA_PRIME_LEN    CMP_BITS_TO_LEN(MAX_DSA_PRIME_BITS)

#define MIN_DSA_PRIME_BITS   256

#define DSA_SUBPRIME_BITS    160
#define DSA_SUBPRIME_LEN     CMP_BITS_TO_LEN(DSA_SUBPRIME_BITS)

#define MAX_DSA_KEY_SIZE_BITS MAX_DSA_PRIME_BITS
#define MIN_DSA_KEY_SIZE_BITS MIN_DSA_PRIME_BITS

struct DSAKeyGenContextStr {
  int initialized;
  SECItem prime;
  SECItem subPrime;
  SECItem base;
  SECItem privateValue;
  SECItem publicValue;
  CMPInt p;
  CMPInt q;
  CMPInt g;
  SECKEYLowPublicKey *publicKey;
  SECKEYLowPrivateKey *privateKey;
};

struct DSASignContextStr {
  int state;
  CMPInt p;
  CMPInt q;
  CMPInt g;
  CMPInt privateValue;
  CMPInt r;
  CMPInt xr;
  CMPInt kInv;
};

struct DSAVerifyContextStr {
  int initialized;
  CMPInt p;
  CMPInt q;
  CMPInt g;
  CMPInt publicValue;
};

#endif
