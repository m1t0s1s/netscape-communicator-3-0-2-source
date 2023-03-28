#ifndef __sec_rsa_h_
#define __sec_rsa_h_

/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

#define MAX_RSA_MODULUS_BITS 2048

#define RSA_MODULUS_LEN(modulusBits) (((modulusBits) + 7) / 8)
#define RSA_PRIME_BITS(modulusBits) (((modulusBits) + 1) / 2)
#define RSA_PRIME_LEN(modulusBits) ((RSA_PRIME_BITS (modulusBits) + 7) / 8)
#define BITS_TO_WORDS(bits) ((bits) / 16 + 1)

#undef  BITS_PER_BYTE
#define BITS_PER_BYTE                8
#define BITS_TO_LEN(modulusBits) \
    (((modulusBits) + BITS_PER_BYTE - 1) / BITS_PER_BYTE)

/* MAX_RSA_PRIME_BITS -- length in bits of the maximum allowed RSA prime
   MAX_RSA_MODULUS_LEN -- length in bytes of the maximum allowed RSA modulus,
                          in canonical format (no sign bit)
   MAX_RSA_PRIME_LEN -- length in bytes of the maximum allowed RSA prime, in
                        canonical format (no sign bit)
 */
#define MAX_RSA_PRIME_BITS RSA_PRIME_BITS (MAX_RSA_MODULUS_BITS)
#define MAX_RSA_PRIME_LEN RSA_PRIME_LEN (MAX_RSA_MODULUS_BITS)
#define MAX_RSA_MODULUS_LEN RSA_MODULUS_LEN (MAX_RSA_MODULUS_BITS)

/* MAX_RSA_MODULUS_WORDS -- length in 16-bit words of the maximum allowed RSA
                            modulus, in bignum format (including sign bit)
   MAX_RSA_PRIME_WORDS -- length in 16-bit words of the maximum allowed RSA
                          prime, in bignum format (including sign bit)
 */
#define MAX_RSA_MODULUS_WORDS BITS_TO_WORDS (MAX_RSA_MODULUS_BITS)
#define MAX_RSA_PRIME_WORDS BITS_TO_WORDS (MAX_RSA_PRIME_BITS)

/* Possible modes for RSA_CreatePrivateContext */
#define NO_BLIND_MODE   0
#define BLIND_BBT_MODE  1

#endif /* __sec_rsa_h_ */
