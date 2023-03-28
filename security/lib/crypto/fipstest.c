/*
 * PKCS #11 FIPS Power-Up Self Test.
 *
 * Copyright (C) 1997 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: fipstest.c,v 1.1.2.1 1997/05/24 00:23:14 jwz Exp $
 */

#include "crypto.h"     /* Required for RC2-ECB, RC2-CBC, RC4, DES-ECB,  */
                        /*              DES-CBC, DES3-ECB, DES3-CBC, RSA */
                        /*              and DSA.                         */
#include "sechash.h"    /* Required for MD2, MD5, and SHA-1. */
#include "seccomon.h"   /* Required for RSA and DSA. */
#include "key.h"        /* Required for RSA and DSA. */
#include "cmp.h"        /* Required for RSA and DSA. */
#include "rsa.h"        /* Required for RSA. */
#include "dsa.h"        /* Required for DSA. */
#include "pkcs11.h"     /* Required for PKCS #11. */

extern int SEC_ERROR_NO_MEMORY;

/* FIPS preprocessor directives for RC2-ECB and RC2-CBC.        */
#define FIPS_RC2_KEY_LENGTH                      5  /*  40-bits */
#define FIPS_RC2_ENCRYPT_LENGTH                  8  /*  64-bits */
#define FIPS_RC2_DECRYPT_LENGTH                  8  /*  64-bits */


/* FIPS preprocessor directives for RC4.                        */
#define FIPS_RC4_KEY_LENGTH                      5  /*  40-bits */
#define FIPS_RC4_ENCRYPT_LENGTH                  8  /*  64-bits */
#define FIPS_RC4_DECRYPT_LENGTH                  8  /*  64-bits */


/* FIPS preprocessor directives for DES-ECB and DES-CBC.        */
#define FIPS_DES_ENCRYPT_LENGTH                  8  /*  64-bits */
#define FIPS_DES_DECRYPT_LENGTH                  8  /*  64-bits */


/* FIPS preprocessor directives for DES3-CBC and DES3-ECB.      */
#define FIPS_DES3_ENCRYPT_LENGTH                 8  /*  64-bits */
#define FIPS_DES3_DECRYPT_LENGTH                 8  /*  64-bits */


/* FIPS preprocessor directives for MD2.                        */
#define FIPS_MD2_HASH_MESSAGE_LENGTH            64  /* 512-bits */


/* FIPS preprocessor directives for MD5.                        */
#define FIPS_MD5_HASH_MESSAGE_LENGTH            64  /* 512-bits */


/* FIPS preprocessor directives for SHA-1.                      */
#define FIPS_SHA1_HASH_MESSAGE_LENGTH           64  /* 512-bits */


/* FIPS preprocessor directives for RSA.                        */
#define FIPS_RSA_TYPE                           siBuffer
#define FIPS_RSA_PUBLIC_EXPONENT_LENGTH          1  /*   8-bits */
#define FIPS_RSA_PRIVATE_VERSION_LENGTH          1  /*   8-bits */
#define FIPS_RSA_MESSAGE_LENGTH                 16  /* 128-bits */
#define FIPS_RSA_PRIVATE_COEFFICIENT_LENGTH     32  /* 256-bits */
#define FIPS_RSA_PRIVATE_PRIME0_LENGTH          33  /* 264-bits */
#define FIPS_RSA_PRIVATE_PRIME1_LENGTH          33  /* 264-bits */
#define FIPS_RSA_PRIVATE_PRIME_EXPONENT0_LENGTH 33  /* 264-bits */
#define FIPS_RSA_PRIVATE_PRIME_EXPONENT1_LENGTH 33  /* 264-bits */
#define FIPS_RSA_PRIVATE_EXPONENT_LENGTH        64  /* 512-bits */
#define FIPS_RSA_ENCRYPT_LENGTH                 64  /* 512-bits */
#define FIPS_RSA_DECRYPT_LENGTH                 64  /* 512-bits */
#define FIPS_RSA_CRYPTO_LENGTH                  64  /* 512-bits */
#define FIPS_RSA_SIGNATURE_LENGTH               64  /* 512-bits */
#define FIPS_RSA_MODULUS_LENGTH                 65  /* 520-bits */


/* FIPS preprocessor directives for DSA.                        */
#define FIPS_DSA_TYPE                           siBuffer
#define FIPS_DSA_DIGEST_LENGTH                  20  /* 160-bits */
#define FIPS_DSA_SUBPRIME_LENGTH                20  /* 160-bits */
#define FIPS_DSA_SIGNATURE_LENGTH               40  /* 320-bits */
#define FIPS_DSA_PRIME_LENGTH                   64  /* 512-bits */
#define FIPS_DSA_BASE_LENGTH                    64  /* 512-bits */


static CK_RV
pk11_fips_RC2_PowerUpSelfTest( void )
{
    /* RC2 Known Key (40-bits). */
    unsigned char *rc2_known_key = (unsigned char *)"RSARC";

    /* RC2-CBC Known Initialization Vector (64-bits). */
    unsigned char *rc2_cbc_known_initialization_vector = (unsigned char *)
                                                         "Security";

    /* RC2 Known Plaintext (64-bits). */
    unsigned char *rc2_ecb_known_plaintext = (unsigned char *)"Netscape";
    unsigned char *rc2_cbc_known_plaintext = (unsigned char *)"Netscape";

    /* RC2 Known Ciphertext (64-bits). */
    unsigned char *rc2_ecb_known_ciphertext = (unsigned char *)
                                              "\x1a\x71\x33\x54"
                                              "\x8d\x5c\xd2\x30";
    unsigned char *rc2_cbc_known_ciphertext = (unsigned char *)
                                              "\xff\x41\xdb\x94"
                                              "\x8a\x4c\x33\xb3";

    /* RC2 variables. */
    unsigned char  rc2_computed_ciphertext[FIPS_RC2_ENCRYPT_LENGTH];
    unsigned char  rc2_computed_plaintext[FIPS_RC2_DECRYPT_LENGTH];
    RC2Context *   rc2_context;
    unsigned int   rc2_bytes_encrypted;
    unsigned int   rc2_bytes_decrypted;
    SECStatus      rc2_status;


    /******************************************************/
    /* RC2-ECB Single-Round Known Answer Encryption Test: */
    /******************************************************/

    rc2_context = RC2_CreateContext( rc2_known_key, FIPS_RC2_KEY_LENGTH,
                                     NULL, SEC_RC2,
                                     FIPS_RC2_KEY_LENGTH );

    if( rc2_context == NULL )
        return( CKR_HOST_MEMORY );

    rc2_status = RC2_Encrypt( rc2_context, rc2_computed_ciphertext,
                              &rc2_bytes_encrypted, FIPS_RC2_ENCRYPT_LENGTH,
                              rc2_ecb_known_plaintext,
                              FIPS_RC2_DECRYPT_LENGTH );

    RC2_DestroyContext( rc2_context, PR_TRUE );

    if( ( rc2_status != SECSuccess ) ||
        ( rc2_bytes_encrypted != FIPS_RC2_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( rc2_computed_ciphertext, rc2_ecb_known_ciphertext,
                       FIPS_RC2_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /******************************************************/
    /* RC2-ECB Single-Round Known Answer Decryption Test: */
    /******************************************************/

    rc2_context = RC2_CreateContext( rc2_known_key, FIPS_RC2_KEY_LENGTH,
                                     NULL, SEC_RC2,
                                     FIPS_RC2_KEY_LENGTH );

    if( rc2_context == NULL )
        return( CKR_HOST_MEMORY );

    rc2_status = RC2_Decrypt( rc2_context, rc2_computed_plaintext,
                              &rc2_bytes_decrypted, FIPS_RC2_DECRYPT_LENGTH,
                              rc2_ecb_known_ciphertext,
                              FIPS_RC2_ENCRYPT_LENGTH );

    RC2_DestroyContext( rc2_context, PR_TRUE );

    if( ( rc2_status != SECSuccess ) ||
        ( rc2_bytes_decrypted != FIPS_RC2_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( rc2_computed_plaintext, rc2_ecb_known_plaintext,
                       FIPS_RC2_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /******************************************************/
    /* RC2-CBC Single-Round Known Answer Encryption Test: */
    /******************************************************/

    rc2_context = RC2_CreateContext( rc2_known_key, FIPS_RC2_KEY_LENGTH,
                                     rc2_cbc_known_initialization_vector,
                                     SEC_RC2_CBC, FIPS_RC2_KEY_LENGTH );

    if( rc2_context == NULL )
        return( CKR_HOST_MEMORY );

    rc2_status = RC2_Encrypt( rc2_context, rc2_computed_ciphertext,
                              &rc2_bytes_encrypted, FIPS_RC2_ENCRYPT_LENGTH,
                              rc2_cbc_known_plaintext,
                              FIPS_RC2_DECRYPT_LENGTH );

    RC2_DestroyContext( rc2_context, PR_TRUE );

    if( ( rc2_status != SECSuccess ) ||
        ( rc2_bytes_encrypted != FIPS_RC2_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( rc2_computed_ciphertext, rc2_cbc_known_ciphertext,
                       FIPS_RC2_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /******************************************************/
    /* RC2-CBC Single-Round Known Answer Decryption Test: */
    /******************************************************/

    rc2_context = RC2_CreateContext( rc2_known_key, FIPS_RC2_KEY_LENGTH,
                                     rc2_cbc_known_initialization_vector,
                                     SEC_RC2_CBC, FIPS_RC2_KEY_LENGTH );

    if( rc2_context == NULL )
        return( CKR_HOST_MEMORY );

    rc2_status = RC2_Decrypt( rc2_context, rc2_computed_plaintext,
                              &rc2_bytes_decrypted, FIPS_RC2_DECRYPT_LENGTH,
                              rc2_cbc_known_ciphertext,
                              FIPS_RC2_ENCRYPT_LENGTH );

    RC2_DestroyContext( rc2_context, PR_TRUE );

    if( ( rc2_status != SECSuccess ) ||
        ( rc2_bytes_decrypted != FIPS_RC2_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( rc2_computed_plaintext, rc2_ecb_known_plaintext,
                       FIPS_RC2_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_RC4_PowerUpSelfTest( void )
{
    /* RC4 Known Key (40-bits). */
    unsigned char *rc4_known_key = (unsigned char *)"RSARC";

    /* RC4 Known Plaintext (64-bits). */
    unsigned char *rc4_known_plaintext = (unsigned char *)"Netscape";

    /* RC4 Known Ciphertext (64-bits). */
    unsigned char *rc4_known_ciphertext = (unsigned char *)"\x29\x33\xc7\x9a"
                                                           "\x9d\x6c\x09\xdd";

    /* RC4 variables. */
    unsigned char  rc4_computed_ciphertext[FIPS_RC4_ENCRYPT_LENGTH];
    unsigned char  rc4_computed_plaintext[FIPS_RC4_DECRYPT_LENGTH];
    RC4Context *   rc4_context;
    unsigned int   rc4_bytes_encrypted;
    unsigned int   rc4_bytes_decrypted;
    SECStatus      rc4_status;


    /**************************************************/
    /* RC4 Single-Round Known Answer Encryption Test: */
    /**************************************************/

    rc4_context = RC4_CreateContext( rc4_known_key, FIPS_RC4_KEY_LENGTH );

    if( rc4_context == NULL )
        return( CKR_HOST_MEMORY );

    rc4_status = RC4_Encrypt( rc4_context, rc4_computed_ciphertext,
                              &rc4_bytes_encrypted, FIPS_RC4_ENCRYPT_LENGTH,
                              rc4_known_plaintext, FIPS_RC4_DECRYPT_LENGTH );

    RC4_DestroyContext( rc4_context, PR_TRUE );

    if( ( rc4_status != SECSuccess ) ||
        ( rc4_bytes_encrypted != FIPS_RC4_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( rc4_computed_ciphertext, rc4_known_ciphertext,
                       FIPS_RC4_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /**************************************************/
    /* RC4 Single-Round Known Answer Decryption Test: */
    /**************************************************/

    rc4_context = RC4_CreateContext( rc4_known_key, FIPS_RC4_KEY_LENGTH );

    if( rc4_context == NULL )
        return( CKR_HOST_MEMORY );

    rc4_status = RC4_Decrypt( rc4_context, rc4_computed_plaintext,
                              &rc4_bytes_decrypted, FIPS_RC4_DECRYPT_LENGTH,
                              rc4_known_ciphertext, FIPS_RC4_ENCRYPT_LENGTH );

    RC4_DestroyContext( rc4_context, PR_TRUE );

    if( ( rc4_status != SECSuccess ) ||
        ( rc4_bytes_decrypted != FIPS_RC4_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( rc4_computed_plaintext, rc4_known_plaintext,
                       FIPS_RC4_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_DES_PowerUpSelfTest( void )
{
    /* DES Known Key (56-bits). */
    unsigned char *des_known_key = (unsigned char *)"ANSI DES";

    /* DES-CBC Known Initialization Vector (64-bits). */
    unsigned char *des_cbc_known_initialization_vector = (unsigned char *)
                                                         "Security";

    /* DES Known Plaintext (64-bits). */
    unsigned char *des_ecb_known_plaintext = (unsigned char *)"Netscape";
    unsigned char *des_cbc_known_plaintext = (unsigned char *)"Netscape";

    /* DES Known Ciphertext (64-bits). */
    unsigned char *des_ecb_known_ciphertext  = (unsigned char *)
                                               "\x26\x14\xe9\xc3"
                                               "\x28\x80\x50\xb0";
    unsigned char *des_cbc_known_ciphertext  = (unsigned char *)
                                               "\x5e\x95\x94\x5d"
                                               "\x76\xa2\xd3\x7d";

    /* DES variables. */
    unsigned char  des_computed_ciphertext[FIPS_DES_ENCRYPT_LENGTH];
    unsigned char  des_computed_plaintext[FIPS_DES_DECRYPT_LENGTH];
    DESContext *   des_context;
    unsigned int   des_bytes_encrypted;
    unsigned int   des_bytes_decrypted;
    SECStatus      des_status;


    /******************************************************/
    /* DES-ECB Single-Round Known Answer Encryption Test: */
    /******************************************************/

    des_context = DES_CreateContext( des_known_key, NULL, SEC_DES, PR_TRUE );

    if( des_context == NULL )
        return( CKR_HOST_MEMORY );

    des_status = DES_Encrypt( des_context, des_computed_ciphertext,
                              &des_bytes_encrypted, FIPS_DES_ENCRYPT_LENGTH,
                              des_ecb_known_plaintext,
                              FIPS_DES_DECRYPT_LENGTH );

    DES_DestroyContext( des_context, PR_TRUE );

    if( ( des_status != SECSuccess ) ||
        ( des_bytes_encrypted != FIPS_DES_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( des_computed_ciphertext, des_ecb_known_ciphertext,
                       FIPS_DES_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /******************************************************/
    /* DES-ECB Single-Round Known Answer Decryption Test: */
    /******************************************************/

    des_context = DES_CreateContext( des_known_key, NULL, SEC_DES, PR_FALSE );

    if( des_context == NULL )
        return( CKR_HOST_MEMORY );

    des_status = DES_Decrypt( des_context, des_computed_plaintext,
                              &des_bytes_decrypted, FIPS_DES_DECRYPT_LENGTH,
                              des_ecb_known_ciphertext,
                              FIPS_DES_ENCRYPT_LENGTH );

    DES_DestroyContext( des_context, PR_TRUE );

    if( ( des_status != SECSuccess ) ||
        ( des_bytes_decrypted != FIPS_DES_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( des_computed_plaintext, des_ecb_known_plaintext,
                       FIPS_DES_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /******************************************************/
    /* DES-CBC Single-Round Known Answer Encryption Test. */
    /******************************************************/

    des_context = DES_CreateContext( des_known_key,
                                     des_cbc_known_initialization_vector,
                                     SEC_DES_CBC, PR_TRUE );

    if( des_context == NULL )
        return( CKR_HOST_MEMORY );

    des_status = DES_Encrypt( des_context, des_computed_ciphertext,
                              &des_bytes_encrypted, FIPS_DES_ENCRYPT_LENGTH,
                              des_cbc_known_plaintext,
                              FIPS_DES_DECRYPT_LENGTH );

    DES_DestroyContext( des_context, PR_TRUE );

    if( ( des_status != SECSuccess ) ||
        ( des_bytes_encrypted != FIPS_DES_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( des_computed_ciphertext, des_cbc_known_ciphertext,
                       FIPS_DES_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /******************************************************/
    /* DES-CBC Single-Round Known Answer Decryption Test. */
    /******************************************************/

    des_context = DES_CreateContext( des_known_key,
                                     des_cbc_known_initialization_vector,
                                     SEC_DES_CBC, PR_FALSE );

    if( des_context == NULL )
        return( CKR_HOST_MEMORY );

    des_status = DES_Decrypt( des_context, des_computed_plaintext,
                              &des_bytes_decrypted, FIPS_DES_DECRYPT_LENGTH,
                              des_cbc_known_ciphertext,
                              FIPS_DES_ENCRYPT_LENGTH );

    DES_DestroyContext( des_context, PR_TRUE );

    if( ( des_status != SECSuccess ) ||
        ( des_bytes_decrypted != FIPS_DES_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( des_computed_plaintext, des_cbc_known_plaintext,
                       FIPS_DES_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_DES3_PowerUpSelfTest( void )
{
    /* DES3 Known Key (56-bits). */
    unsigned char *des3_known_key = (unsigned char *)
                                    "ANSI Triple-DES Key Data";

    /* DES3-CBC Known Initialization Vector (64-bits). */
    unsigned char *des3_cbc_known_initialization_vector = (unsigned char *)
                                                          "Security";

    /* DES3 Known Plaintext (64-bits). */
    unsigned char *des3_ecb_known_plaintext = (unsigned char *)"Netscape";
    unsigned char *des3_cbc_known_plaintext = (unsigned char *)"Netscape";

    /* DES3 Known Ciphertext (64-bits). */
    unsigned char *des3_ecb_known_ciphertext = (unsigned char *)
                                               "\x55\x8e\xad\x3c"
                                               "\xee\x49\x69\xbe";
    unsigned char *des3_cbc_known_ciphertext = (unsigned char *)
                                               "\x43\xdc\x6a\xc1"
                                               "\xaf\xa6\x32\xf5";

    /* DES3 variables. */
    unsigned char  des3_computed_ciphertext[FIPS_DES3_ENCRYPT_LENGTH];
    unsigned char  des3_computed_plaintext[FIPS_DES3_DECRYPT_LENGTH];
    DESContext *   des3_context;
    unsigned int   des3_bytes_encrypted;
    unsigned int   des3_bytes_decrypted;
    SECStatus      des3_status;


    /*******************************************************/
    /* DES3-ECB Single-Round Known Answer Encryption Test. */
    /*******************************************************/

    des3_context = DES_CreateContext( des3_known_key, NULL,
                                     SEC_DES_EDE3, PR_TRUE );

    if( des3_context == NULL )
        return( CKR_HOST_MEMORY );

    des3_status = DES_Encrypt( des3_context, des3_computed_ciphertext,
                               &des3_bytes_encrypted, FIPS_DES3_ENCRYPT_LENGTH,
                               des3_ecb_known_plaintext,
                               FIPS_DES3_DECRYPT_LENGTH );

    DES_DestroyContext( des3_context, PR_TRUE );

    if( ( des3_status != SECSuccess ) ||
        ( des3_bytes_encrypted != FIPS_DES3_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( des3_computed_ciphertext, des3_ecb_known_ciphertext,
                       FIPS_DES3_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /*******************************************************/
    /* DES3-ECB Single-Round Known Answer Decryption Test. */
    /*******************************************************/

    des3_context = DES_CreateContext( des3_known_key, NULL,
                                     SEC_DES_EDE3, PR_FALSE );

    if( des3_context == NULL )
        return( CKR_HOST_MEMORY );

    des3_status = DES_Decrypt( des3_context, des3_computed_plaintext,
                               &des3_bytes_decrypted, FIPS_DES3_DECRYPT_LENGTH,
                               des3_ecb_known_ciphertext,
                               FIPS_DES3_ENCRYPT_LENGTH );

    DES_DestroyContext( des3_context, PR_TRUE );

    if( ( des3_status != SECSuccess ) ||
        ( des3_bytes_decrypted != FIPS_DES3_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( des3_computed_plaintext, des3_ecb_known_plaintext,
                       FIPS_DES3_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /*******************************************************/
    /* DES3-CBC Single-Round Known Answer Encryption Test. */
    /*******************************************************/

    des3_context = DES_CreateContext( des3_known_key,
                                      des3_cbc_known_initialization_vector,
                                      SEC_DES_EDE3_CBC, PR_TRUE );

    if( des3_context == NULL )
        return( CKR_HOST_MEMORY );

    des3_status = DES_Encrypt( des3_context, des3_computed_ciphertext,
                               &des3_bytes_encrypted, FIPS_DES3_ENCRYPT_LENGTH,
                               des3_cbc_known_plaintext,
                               FIPS_DES3_DECRYPT_LENGTH );

    DES_DestroyContext( des3_context, PR_TRUE );

    if( ( des3_status != SECSuccess ) ||
        ( des3_bytes_encrypted != FIPS_DES3_ENCRYPT_LENGTH ) ||
        ( PORT_Memcmp( des3_computed_ciphertext, des3_cbc_known_ciphertext,
                       FIPS_DES3_ENCRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /*******************************************************/
    /* DES3-CBC Single-Round Known Answer Decryption Test. */
    /*******************************************************/

    des3_context = DES_CreateContext( des3_known_key,
                                      des3_cbc_known_initialization_vector,
                                      SEC_DES_EDE3_CBC, PR_FALSE );

    if( des3_context == NULL )
        return( CKR_HOST_MEMORY );

    des3_status = DES_Decrypt( des3_context, des3_computed_plaintext,
                               &des3_bytes_decrypted, FIPS_DES3_DECRYPT_LENGTH,
                               des3_cbc_known_ciphertext,
                               FIPS_DES3_ENCRYPT_LENGTH );

    DES_DestroyContext( des3_context, PR_TRUE );

    if( ( des3_status != SECSuccess ) ||
        ( des3_bytes_decrypted != FIPS_DES3_DECRYPT_LENGTH ) ||
        ( PORT_Memcmp( des3_computed_plaintext, des3_cbc_known_plaintext,
                       FIPS_DES3_DECRYPT_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_MD2_PowerUpSelfTest( void )
{
    /* MD2 Known Hash Message (512-bits). */
    unsigned char *md2_known_hash_message = (unsigned char *)
                                            "The test message for the MD2, MD"
                                            "5, and SHA-1 hashing algorithms.";

    /* MD2 Known Digest Message (128-bits). */
    unsigned char *md2_known_digest  = (unsigned char *)
                                       "\x41\x5a\x12\xb2\x3f\x28\x97\x17"
                                       "\x0c\x71\x4e\xcc\x40\xc8\x1d\x1b";

    /* MD2 variables. */
    MD2Context *   md2_context;
    unsigned int md2_bytes_hashed;
    unsigned char md2_computed_digest[MD2_LENGTH];


    /***********************************************/
    /* MD2 Single-Round Known Answer Hashing Test. */
    /***********************************************/

    md2_context = MD2_NewContext();

    if( md2_context == NULL )
        return( CKR_HOST_MEMORY );

    MD2_Begin( md2_context );

    MD2_Update( md2_context, md2_known_hash_message,
                FIPS_MD2_HASH_MESSAGE_LENGTH );

    MD2_End( md2_context, md2_computed_digest, &md2_bytes_hashed, MD2_LENGTH );

    MD2_DestroyContext( md2_context , PR_TRUE );

    if( ( md2_bytes_hashed != MD2_LENGTH ) ||
        ( PORT_Memcmp( md2_computed_digest, md2_known_digest,
                       MD2_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_MD5_PowerUpSelfTest( void )
{
    /* MD5 Known Hash Message (512-bits). */
    unsigned char *md5_known_hash_message = (unsigned char *)
                                            "The test message for the MD2, MD"
                                            "5, and SHA-1 hashing algorithms.";

    /* MD5 Known Digest Message (128-bits). */
    unsigned char *md5_known_digest  = (unsigned char *)
                                       "\x25\xc8\xc0\x10\xc5\x6e\x68\x28"
                                       "\x28\xa4\xa5\xd2\x98\x9a\xea\x2d";

    /* MD5 variables. */
    unsigned char  md5_computed_digest[MD5_LENGTH];
    SECStatus      md5_status;


    /***********************************************/
    /* MD5 Single-Round Known Answer Hashing Test. */
    /***********************************************/

    md5_status = MD5_HashBuf( md5_computed_digest, md5_known_hash_message,
                              FIPS_MD5_HASH_MESSAGE_LENGTH );

    if( ( md5_status != SECSuccess ) ||
        ( PORT_Memcmp( md5_computed_digest, md5_known_digest,
                       MD5_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_SHA1_PowerUpSelfTest( void )
{
    /* SHA-1 Known Hash Message (512-bits). */
    unsigned char *sha1_known_hash_message = (unsigned char *)
                                             "The test message for the MD2, MD"
                                             "5, and SHA-1 hashing algorithms.";

    /* SHA-1 Known Digest Message (160-bits). */
    unsigned char *sha1_known_digest = (unsigned char *)
                                       "\x0a\x6d\x07\xba\x1e\xbd\x8a\x1b"
                                       "\x72\xf6\xc7\x22\xf1\x27\x9f\xf0"
                                       "\xe0\x68\x47\x7a";

    /* SHA-1 variables. */
    unsigned char  sha1_computed_digest[SHA1_LENGTH];
    SECStatus      sha1_status;


    /*************************************************/
    /* SHA-1 Single-Round Known Answer Hashing Test. */
    /*************************************************/

    sha1_status = SHA1_HashBuf( sha1_computed_digest, sha1_known_hash_message,
                                FIPS_SHA1_HASH_MESSAGE_LENGTH );

    if( ( sha1_status != SECSuccess ) ||
        ( PORT_Memcmp( sha1_computed_digest, sha1_known_digest,
                       SHA1_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );
}


static CK_RV
pk11_fips_RSA_PowerUpSelfTest( void )
{
    /* RSA Known Modulus used in both Public/Private Key Values (520-bits). */
    unsigned char *rsa_modulus = (unsigned char *)
                                 "\x00\xa1\xe9\x5e\x66\x88\xe2\xf2"
                                 "\x2b\xe7\x70\x36\x33\xbc\xeb\x55"
                                 "\x55\xf1\x60\x18\x3c\xfb\xd2\x79"
                                 "\xf6\xc4\xb8\x09\xe3\x12\xf6\x63"
                                 "\x6d\xc7\x8e\x19\xc0\x0e\x10\x78"
                                 "\xc1\xfe\x2a\x41\x74\x2d\xf7\xc4"
                                 "\x69\xa7\x3c\xbc\x8a\xc8\x31\x2b"
                                 "\x4f\x60\xf0\xf1\xec\x5a\x29\xec"
                                 "\x6b";

    /* RSA Known Public Key Values (8-bits). */
    unsigned char *rsa_public_exponent = (unsigned char *)"\x03";

    /* RSA Known Private Key Values (version                 is   8-bits), */
    /*                              (private exponent        is 512-bits), */
    /*                              (private prime0          is 264-bits), */
    /*                              (private prime1          is 264-bits), */
    /*                              (private prime exponent0 is 264-bits), */
    /*                              (private prime exponent1 is 264-bits), */
    /*                          and (private coefficient     is 256-bits). */
    unsigned char *rsa_version          = (unsigned char *)"\x00";
    unsigned char *rsa_private_exponent = (unsigned char *)
                                          "\x6b\xf0\xe9\x99\xb0\x97\x4c\x1d"
                                          "\x44\xf5\x79\x77\xd3\x47\x8e\x39"
                                          "\x4b\x95\x65\x7d\xfd\x36\xfb\xf9"
                                          "\xd8\x7a\xb1\x42\x0c\xa4\x42\x48"
                                          "\x20\x1c\x6b\x7d\x5d\xa3\x58\xd6"
                                          "\x95\xd6\x41\xe3\xd6\x73\xad\xdb"
                                          "\x3b\x89\x00\x8a\xcd\x1d\xb9\x06"
                                          "\xac\xac\x0e\x02\x72\x1c\xf8\xab";
    unsigned char *rsa_private_prime0   = (unsigned char *)
                                          "\x00\xd2\x2c\x9d\xef\x7c\x8f\x58"
                                          "\x93\x19\xa1\x77\x0e\x38\x3e\x85"
                                          "\xb4\xaf\xcc\x99\xa5\x43\xbf\x97"
                                          "\xdc\x46\xb8\x3f\x6e\x85\x18\x00"
                                          "\x81";
    unsigned char *rsa_private_prime1   = (unsigned char *)
                                          "\x00\xc5\x36\xda\x94\x85\x0c\x1a"
                                          "\xed\x03\xc7\x67\x90\x34\x0b\xb9"
                                          "\xec\x1e\x22\xa2\x15\x50\xc4\xfd"
                                          "\xe9\x17\x36\x9d\x7a\x29\xe6\x76"
                                          "\xeb";
    unsigned char *rsa_private_prime_exponent0 = (unsigned char *)
                                                 "\x00\x8c\x1d\xbe\x9f\xa8"
                                                 "\x5f\x90\x62\x11\x16\x4f"
                                                 "\x5e\xd0\x29\xae\x78\x75"
                                                 "\x33\x11\x18\xd7\xd5\x0f"
                                                 "\xe8\x2f\x25\x7f\x9f\x03"
                                                 "\x65\x55\xab";
    unsigned char *rsa_private_prime_exponent1 = (unsigned char *)
                                                 "\x00\x83\x79\xe7\x0d\xae"
                                                 "\x08\x11\xf3\x57\xda\x45"
                                                 "\x0a\xcd\x5d\x26\x9d\x69"
                                                 "\x6c\x6c\x0e\x35\xd8\xa9"
                                                 "\x46\x0f\x79\xbe\x51\x71"
                                                 "\x44\x4f\x47";
    unsigned char *rsa_private_coefficient = (unsigned char *)
                                             "\x54\x8d\xb8\xdc\x8b\xde\xbb"
                                             "\x08\xc9\x67\xb7\xa9\x5f\xa5"
                                             "\xc4\x5e\x67\xaa\xfe\x1a\x08"
                                             "\xeb\x48\x43\xcb\xb0\xb9\x38"
                                             "\x3a\x31\x39\xde";


    /* RSA Known Plaintext (512-bits). */
    unsigned char *rsa_known_plaintext = (unsigned char *)
                                         "Known plaintext utilized for RSA"
                                         " Encryption and Decryption test.";

    /* RSA Known Ciphertext (512-bits). */
    unsigned char *rsa_known_ciphertext = (unsigned char *)
                                          "\x12\x80\x3a\x53\xee\x93\x81\xa5"
                                          "\xf7\x40\xc5\xb1\xef\xd9\x27\xaf"
                                          "\xef\x4b\x87\x44\x00\xd0\xda\xcf"
                                          "\x10\x57\x4c\xd5\xc3\xed\x84\xdc"
                                          "\x74\x03\x19\x69\x2c\xd6\x54\x3e"
                                          "\xd2\xe3\x90\xb6\x67\x91\x2f\x1f"
                                          "\x54\x13\x99\x00\x0b\xfd\x52\x7f"
                                          "\xd8\xc6\xdb\x8a\xfe\x06\xf3\xb1";

    /* RSA Known Message (128-bits). */
    unsigned char *rsa_known_message  = (unsigned char *)"Netscape Forever";

    /* RSA Known Signed Hash (512-bits). */
    unsigned char *rsa_known_signature = (unsigned char *)
                                         "\x27\x23\xa6\x71\x57\xc8\x70\x5f"
                                         "\x70\x0e\x06\x7b\x96\x6a\xaa\x41"
                                         "\x6e\xab\x67\x4b\x5f\x76\xc4\x53"
                                         "\x23\xd7\x57\x7a\x3a\xbc\x4c\x27"
                                         "\x65\xca\xde\x9f\xd3\x1d\xa4\x5a"
                                         "\xf9\x8f\xb2\x05\xa3\x86\xf9\x66"
                                         "\x55\x4c\x68\x50\x66\xa4\xe9\x17"
                                         "\x45\x11\xb8\x1a\xfc\xbc\x79\x3b";


    /* RSA variables. */
    unsigned char         rsa_computed_ciphertext[FIPS_RSA_ENCRYPT_LENGTH];
    unsigned char         rsa_computed_plaintext[FIPS_RSA_DECRYPT_LENGTH];
    unsigned char         rsa_computed_signature[FIPS_RSA_SIGNATURE_LENGTH];
    PRArenaPool *         rsa_public_arena;
    PRArenaPool *         rsa_private_arena;
    SECKEYLowPublicKey *  rsa_public_key;
    SECKEYLowPrivateKey * rsa_private_key;
    unsigned int          rsa_bytes_signed;
    SECStatus             rsa_status;


    /****************************************/
    /* Compose RSA Public/Private Key Pair. */
    /****************************************/

    /* Create some space for the RSA public key. */
    rsa_public_arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if( rsa_public_arena == NULL ) {
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }

    /* Create some space for the RSA private key. */
    rsa_private_arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE );

    if( rsa_private_arena == NULL ) {
        PORT_FreeArena( rsa_public_arena, PR_TRUE );
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }

    /* Build RSA Public Key. */
    rsa_public_key = (SECKEYLowPublicKey *)
                     PORT_ArenaZAlloc( rsa_public_arena,
                                       sizeof( SECKEYLowPublicKey ) );

    if( rsa_public_key == NULL ) {
        PORT_FreeArena( rsa_public_arena, PR_TRUE );
        PORT_FreeArena( rsa_private_arena, PR_TRUE );
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }

    /* Build RSA Private Key. */
    rsa_private_key = (SECKEYLowPrivateKey *)
                      PORT_ArenaZAlloc( rsa_private_arena,
                                        sizeof( SECKEYLowPrivateKey ) );

    if( rsa_private_key == NULL ) {
        SECKEY_LowDestroyPublicKey( rsa_public_key );
        PORT_FreeArena( rsa_private_arena, PR_TRUE );
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }


    /*********************************/
    /* Fill RSA Public Key Contents. */
    /*********************************/

    /* (a) arena */
    rsa_public_key->arena = rsa_public_arena;

    /* (b) keyType */
    rsa_public_key->keyType = rsaKey;

    /* (c) modulus */
    rsa_public_key->u.rsa.modulus.data =
                    (unsigned char *)
                    PORT_ArenaAlloc( rsa_public_arena,
                                     FIPS_RSA_MODULUS_LENGTH );

    if( !rsa_public_key->u.rsa.modulus.data )
        goto rsa_memory_loser;

    rsa_public_key->u.rsa.modulus.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_public_key->u.rsa.modulus.data, rsa_modulus,
                 FIPS_RSA_MODULUS_LENGTH );
    rsa_public_key->u.rsa.modulus.len = FIPS_RSA_MODULUS_LENGTH;

    /* (d) publicExponent */
    rsa_public_key->u.rsa.publicExponent.data =
                    (unsigned char *)
                    PORT_ArenaAlloc( rsa_public_arena,
                                     FIPS_RSA_PUBLIC_EXPONENT_LENGTH );

    if( !rsa_public_key->u.rsa.publicExponent.data )
        goto rsa_memory_loser;

    rsa_public_key->u.rsa.publicExponent.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_public_key->u.rsa.publicExponent.data, rsa_public_exponent,
                 FIPS_RSA_PUBLIC_EXPONENT_LENGTH );
    rsa_public_key->u.rsa.publicExponent.len = FIPS_RSA_PUBLIC_EXPONENT_LENGTH;


    /**********************************/
    /* Fill RSA Private Key Contents. */
    /**********************************/

    /* (a) arena */
    rsa_private_key->arena = rsa_private_arena;

    /* (b) keyType */
    rsa_private_key->keyType = rsaKey;

    /* (c) modulus */
    rsa_private_key->u.rsa.modulus.data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_MODULUS_LENGTH );

    if( !rsa_private_key->u.rsa.modulus.data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.modulus.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.modulus.data, rsa_modulus,
                 FIPS_RSA_MODULUS_LENGTH );
    rsa_private_key->u.rsa.modulus.len = FIPS_RSA_MODULUS_LENGTH;

    /* (d) version */
    rsa_private_key->u.rsa.version.data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_VERSION_LENGTH );

    if( !rsa_private_key->u.rsa.version.data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.version.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.version.data, rsa_version,
                 FIPS_RSA_PRIVATE_VERSION_LENGTH );
    rsa_private_key->u.rsa.version.len = FIPS_RSA_PRIVATE_VERSION_LENGTH;

    /* (e) publicExponent */
    rsa_private_key->u.rsa.publicExponent.data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PUBLIC_EXPONENT_LENGTH );

    if( !rsa_private_key->u.rsa.publicExponent.data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.publicExponent.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.publicExponent.data,
                 rsa_public_exponent, FIPS_RSA_PUBLIC_EXPONENT_LENGTH );
    rsa_private_key->u.rsa.publicExponent.len = FIPS_RSA_PUBLIC_EXPONENT_LENGTH;

    /* (f) privateExponent */
    rsa_private_key->u.rsa.privateExponent.data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_EXPONENT_LENGTH );

    if( !rsa_private_key->u.rsa.privateExponent.data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.privateExponent.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.privateExponent.data,
                 rsa_private_exponent, FIPS_RSA_PRIVATE_EXPONENT_LENGTH );
    rsa_private_key->u.rsa.privateExponent.len =
                     FIPS_RSA_PRIVATE_EXPONENT_LENGTH;

    /* (g) prime[0] */
    rsa_private_key->u.rsa.prime[0].data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_PRIME0_LENGTH );

    if( !rsa_private_key->u.rsa.prime[0].data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.prime[0].type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.prime[0].data, rsa_private_prime0,
                 FIPS_RSA_PRIVATE_PRIME0_LENGTH );
    rsa_private_key->u.rsa.prime[0].len = FIPS_RSA_PRIVATE_PRIME0_LENGTH;

    /* (h) prime[1] */
    rsa_private_key->u.rsa.prime[1].data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_PRIME1_LENGTH );

    if( !rsa_private_key->u.rsa.prime[1].data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.prime[1].type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.prime[1].data, rsa_private_prime1,
                 FIPS_RSA_PRIVATE_PRIME1_LENGTH );
    rsa_private_key->u.rsa.prime[1].len = FIPS_RSA_PRIVATE_PRIME1_LENGTH;

    /* (i) primeExponent[0] */
    rsa_private_key->u.rsa.primeExponent[0].data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_PRIME_EXPONENT0_LENGTH );

    if( !rsa_private_key->u.rsa.primeExponent[0].data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.primeExponent[0].type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.primeExponent[0].data,
                 rsa_private_prime_exponent0,
                 FIPS_RSA_PRIVATE_PRIME_EXPONENT0_LENGTH );
    rsa_private_key->u.rsa.primeExponent[0].len =
                     FIPS_RSA_PRIVATE_PRIME_EXPONENT0_LENGTH;

    /* (j) primeExponent[1] */
    rsa_private_key->u.rsa.primeExponent[1].data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_PRIME_EXPONENT1_LENGTH );

    if( !rsa_private_key->u.rsa.primeExponent[1].data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.primeExponent[1].type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.primeExponent[1].data,
                 rsa_private_prime_exponent1,
                 FIPS_RSA_PRIVATE_PRIME_EXPONENT1_LENGTH );
    rsa_private_key->u.rsa.primeExponent[1].len =
                     FIPS_RSA_PRIVATE_PRIME_EXPONENT1_LENGTH;

    /* (k) coefficient */
    rsa_private_key->u.rsa.coefficient.data =
                     (unsigned char *)
                     PORT_ArenaAlloc( rsa_private_arena,
                                      FIPS_RSA_PRIVATE_COEFFICIENT_LENGTH );

    if( !rsa_private_key->u.rsa.coefficient.data )
        goto rsa_memory_loser;

    rsa_private_key->u.rsa.coefficient.type = FIPS_RSA_TYPE;
    PORT_Memcpy( rsa_private_key->u.rsa.coefficient.data,
                 rsa_private_coefficient,
                 FIPS_RSA_PRIVATE_COEFFICIENT_LENGTH );
    rsa_private_key->u.rsa.coefficient.len =
                     FIPS_RSA_PRIVATE_COEFFICIENT_LENGTH;


    /**************************************************/
    /* RSA Single-Round Known Answer Encryption Test. */
    /**************************************************/

    /* Perform RSA Public Key Encryption. */
    rsa_status = RSA_PublicKeyOp( rsa_public_key, rsa_computed_ciphertext,
                                  rsa_known_plaintext, FIPS_RSA_CRYPTO_LENGTH );

    if( ( rsa_status != SECSuccess ) ||
        ( PORT_Memcmp( rsa_computed_ciphertext, rsa_known_ciphertext,
                       FIPS_RSA_ENCRYPT_LENGTH ) != 0 ) )
        goto rsa_loser;

    /**************************************************/
    /* RSA Single-Round Known Answer Decryption Test. */
    /**************************************************/

    /* Perform RSA Private Key Decryption. */
    rsa_status = RSA_PrivateKeyOp( rsa_private_key, rsa_computed_plaintext,
                                   rsa_known_ciphertext,
                                   FIPS_RSA_CRYPTO_LENGTH );

    if( ( rsa_status != SECSuccess ) ||
        ( PORT_Memcmp( rsa_computed_plaintext, rsa_known_plaintext,
                       FIPS_RSA_DECRYPT_LENGTH ) != 0 ) )
        goto rsa_loser;


    /*************************************************/
    /* RSA Single-Round Known Answer Signature Test. */
    /*************************************************/

    /* Perform RSA signature with the RSA private key. */
    rsa_status = RSA_Sign( rsa_private_key, rsa_computed_signature,
                           &rsa_bytes_signed,
                           FIPS_RSA_SIGNATURE_LENGTH, rsa_known_message,
                           FIPS_RSA_MESSAGE_LENGTH );

    if( ( rsa_status != SECSuccess ) ||
        ( rsa_bytes_signed != FIPS_RSA_SIGNATURE_LENGTH ) ||
        ( PORT_Memcmp( rsa_computed_signature, rsa_known_signature,
                       FIPS_RSA_SIGNATURE_LENGTH ) != 0 ) )
        goto rsa_loser;


    /****************************************************/
    /* RSA Single-Round Known Answer Verification Test. */
    /****************************************************/

    /* Perform RSA verification with the RSA public key. */
    rsa_status = RSA_CheckSign( rsa_public_key,
                                rsa_computed_signature,
                                FIPS_RSA_SIGNATURE_LENGTH,
                                rsa_known_message,
                                FIPS_RSA_MESSAGE_LENGTH );

    /* Note:  Since the "rsa_computed_signature" is generated by the call */
    /*        to RSA_Sign(), and the call to RSA_CheckSign() is merely    */
    /*        checking its contents for validity, no data contents are    */
    /*        altered by a call to this function.                         */
    if( rsa_status != SECSuccess )
        goto rsa_loser;

    /* Dispose of all RSA key material. */
    SECKEY_LowDestroyPublicKey( rsa_public_key );
    SECKEY_LowDestroyPrivateKey( rsa_private_key );

    return( CKR_OK );


rsa_memory_loser:

    SECKEY_LowDestroyPublicKey( rsa_public_key );
    SECKEY_LowDestroyPrivateKey( rsa_private_key );

    return( CKR_HOST_MEMORY );


rsa_loser:

    SECKEY_LowDestroyPublicKey( rsa_public_key );
    SECKEY_LowDestroyPrivateKey( rsa_private_key );

    return( CKR_DEVICE_ERROR );
}


static CK_RV
pk11_fips_DSA_PowerUpSelfTest( void )
{
    /* DSA Known P (512-bits), Q (160-bits), and G (512-bits) Values. */
    unsigned char *dsa_P = (unsigned char *)
                           "\x8d\xf2\xa4\x94\x49\x22\x76\xaa"
                           "\x3d\x25\x75\x9b\xb0\x68\x69\xcb"
                           "\xea\xc0\xd8\x3a\xfb\x8d\x0c\xf7"
                           "\xcb\xb8\x32\x4f\x0d\x78\x82\xe5"
                           "\xd0\x76\x2f\xc5\xb7\x21\x0e\xaf"
                           "\xc2\xe9\xad\xac\x32\xab\x7a\xac"
                           "\x49\x69\x3d\xfb\xf8\x37\x24\xc2"
                           "\xec\x07\x36\xee\x31\xc8\x02\x91";
    unsigned char *dsa_Q = (unsigned char *)
                           "\xc7\x73\x21\x8c\x73\x7e\xc8\xee"
                           "\x99\x3b\x4f\x2d\xed\x30\xf4\x8e"
                           "\xda\xce\x91\x5f";
    unsigned char *dsa_G = (unsigned char *)
                           "\x62\x6d\x02\x78\x39\xea\x0a\x13"
                           "\x41\x31\x63\xa5\x5b\x4c\xb5\x00"
                           "\x29\x9d\x55\x22\x95\x6c\xef\xcb"
                           "\x3b\xff\x10\xf3\x99\xce\x2c\x2e"
                           "\x71\xcb\x9d\xe5\xfa\x24\xba\xbf"
                           "\x58\xe5\xb7\x95\x21\x92\x5c\x9c"
                           "\xc4\x2e\x9f\x6f\x46\x4b\x08\x8c"
                           "\xc5\x72\xaf\x53\xe6\xd7\x88\x02";

    /* DSA Known Random Values (known random key block       is 160-bits)  */
    /*                     and (known random signature block is 160-bits). */
    unsigned char *dsa_known_random_key_block       = (unsigned char *)
                                                      "Mozilla Rules World!";
    unsigned char *dsa_known_random_signature_block = (unsigned char *)
                                                      "Random DSA Signature";

    /* DSA Known Digest (160-bits) */
    unsigned char *dsa_known_digest                 = (unsigned char *)
                                                      "DSA Signature Digest";

    /* DSA Known Signature (320-bits). */
    unsigned char *dsa_known_signature = (unsigned char *)
                                         "\x39\x0d\x84\xb1\xf7\x52\x89\xba"
                                         "\xec\x1e\xa8\xe2\x00\x8e\x37\x8f"
                                         "\xc2\xf5\xf8\x70\x11\xa8\xc7\x02"
                                         "\x0e\x75\xcf\x6b\x54\x4a\x52\xe8"
                                         "\xd8\x6d\x4a\xe8\xee\x56\x8e\x59";

    /* DSA variables. */
    unsigned char          dsa_computed_signature[FIPS_DSA_SIGNATURE_LENGTH];
    PQGParams *            dsa_pqg;
    DSAKeyGenContext *     dsa_key_gen_context;
    SECKEYLowPublicKey *   dsa_public_key;
    SECKEYLowPrivateKey *  dsa_private_key;
    DSASignContext *       dsa_signature_context;
    DSAVerifyContext *     dsa_verification_context;
    unsigned int           dsa_bytes_signed;
    SECStatus              dsa_status;


    /*************************************/
    /* Compose DSA PQG Parameter Values. */
    /*************************************/

    /* Create some space for the PQG parameters. */
    dsa_pqg = (PQGParams *) PORT_ZAlloc( sizeof( PQGParams ) );

    if( dsa_pqg == NULL ) {
      PORT_SetError( SEC_ERROR_NO_MEMORY );
      return( CKR_HOST_MEMORY );
    }

    /* Allocate space for prime (P). */
    dsa_pqg->prime.data = (unsigned char *)
                          PORT_ZAlloc( FIPS_DSA_PRIME_LENGTH );

    if( dsa_pqg->prime.data == NULL ) {
        PORT_ZFree( dsa_pqg, sizeof( PQGParams ) );
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }

    /* Allocate space for subPrime (Q). */
    dsa_pqg->subPrime.data = (unsigned char *)
                             PORT_ZAlloc( FIPS_DSA_SUBPRIME_LENGTH );

    if( dsa_pqg->subPrime.data == NULL ) {
        PORT_ZFree( dsa_pqg->prime.data, FIPS_DSA_PRIME_LENGTH );
        PORT_ZFree( dsa_pqg, sizeof( PQGParams ) );
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }

    /* Allocate space for base (G). */
    dsa_pqg->base.data = (unsigned char *)
                         PORT_ZAlloc( FIPS_DSA_BASE_LENGTH );

    if( dsa_pqg->base.data == NULL ) {
        PORT_ZFree( dsa_pqg->prime.data, FIPS_DSA_PRIME_LENGTH );
        PORT_ZFree( dsa_pqg->subPrime.data, FIPS_DSA_SUBPRIME_LENGTH );
        PORT_ZFree( dsa_pqg, sizeof( PQGParams ) );
        PORT_SetError( SEC_ERROR_NO_MEMORY );
        return( CKR_HOST_MEMORY );
    }


    /************************************/
    /* Fill DSA PQG Parameter Contents. */
    /************************************/

    /* (a) prime P */
    dsa_pqg->prime.type = FIPS_DSA_TYPE;
    PORT_Memcpy( dsa_pqg->prime.data, dsa_P, FIPS_DSA_PRIME_LENGTH );
    dsa_pqg->prime.len = FIPS_DSA_PRIME_LENGTH;

    /* (b) subPrime Q */
    dsa_pqg->subPrime.type = FIPS_DSA_TYPE;
    PORT_Memcpy( dsa_pqg->subPrime.data, dsa_Q, FIPS_DSA_SUBPRIME_LENGTH );
    dsa_pqg->subPrime.len = FIPS_DSA_SUBPRIME_LENGTH;

    /* (c) base G */
    dsa_pqg->base.type = FIPS_DSA_TYPE;
    PORT_Memcpy( dsa_pqg->base.data, dsa_G, FIPS_DSA_BASE_LENGTH );
    dsa_pqg->base.len = FIPS_DSA_BASE_LENGTH;


    /*******************************************/
    /* Generate a DSA public/private key pair. */
    /*******************************************/

    /* Generate a DSA public/private key pair. */
    dsa_key_gen_context = DSA_CreateKeyGenContext( dsa_pqg );

    if( dsa_key_gen_context == NULL )
        goto dsa_memory_loser;

    /* Fill in DSA public/private key pair. */
    dsa_status = DSA_KeyGen( dsa_key_gen_context, &dsa_public_key,
                             &dsa_private_key, dsa_known_random_key_block );

    DSA_DestroyKeyGenContext( dsa_key_gen_context );
    if( dsa_status != SECSuccess )
        goto dsa_memory_loser;

    /* Dispose of all DSA PQG Parameter material. */
    PORT_ZFree( dsa_pqg->prime.data, dsa_pqg->prime.len );
    PORT_ZFree( dsa_pqg->subPrime.data, dsa_pqg->subPrime.len );
    PORT_ZFree( dsa_pqg->base.data, dsa_pqg->base.len );
    PORT_ZFree( dsa_pqg, sizeof( PQGParams ) );


    /*************************************************/
    /* DSA Single-Round Known Answer Signature Test. */
    /*************************************************/

    /* Create a context for DSA signatures. */
    dsa_signature_context = DSA_CreateSignContext( dsa_private_key );

    if( dsa_signature_context == NULL )
        return( CKR_HOST_MEMORY );

    /* Set up DSA signature process. */
    dsa_status = DSA_Presign( dsa_signature_context,
                              dsa_known_random_signature_block );

    if( dsa_status != SECSuccess ) {
        DSA_DestroySignContext( dsa_signature_context, PR_TRUE );
        return( CKR_DEVICE_ERROR );
    }

    /* Perform DSA signature process. */
    dsa_status = DSA_Sign( dsa_signature_context, dsa_computed_signature,
                           &dsa_bytes_signed, FIPS_DSA_SIGNATURE_LENGTH,
                           dsa_known_digest, FIPS_DSA_DIGEST_LENGTH );

    DSA_DestroySignContext( dsa_signature_context, PR_TRUE );

    if( ( dsa_status != SECSuccess ) ||
        ( dsa_bytes_signed != FIPS_DSA_SIGNATURE_LENGTH ) ||
        ( PORT_Memcmp( dsa_computed_signature, dsa_known_signature,
                       FIPS_DSA_SIGNATURE_LENGTH ) != 0 ) )
        return( CKR_DEVICE_ERROR );


    /****************************************************/
    /* DSA Single-Round Known Answer Verification Test. */
    /****************************************************/

    /* Create a context to verify DSA signatures. */
    dsa_verification_context = DSA_CreateVerifyContext( dsa_public_key );

    if( dsa_verification_context == NULL )
        return( CKR_DEVICE_ERROR );

    /* Perform DSA verification process. */
    dsa_status = DSA_Verify( dsa_verification_context, dsa_computed_signature,
                             FIPS_DSA_SIGNATURE_LENGTH, dsa_known_digest,
                             FIPS_DSA_DIGEST_LENGTH );

    DSA_DestroyVerifyContext( dsa_verification_context, PR_TRUE );

    /* Note:  Since the "dsa_computed_signature" is generated by the call */
    /*        to DSA_Sign(), and the call to DSA_Verify() is merely       */
    /*        checking its contents for validity, no data contents are    */
    /*        altered by a call to this function.                         */

    /* Verify DSA signature. */
    if( dsa_status != SECSuccess )
        return( CKR_DEVICE_ERROR );

    return( CKR_OK );


dsa_memory_loser:

    PORT_ZFree( dsa_pqg->prime.data, dsa_pqg->prime.len );
    PORT_ZFree( dsa_pqg->subPrime.data, dsa_pqg->subPrime.len );
    PORT_ZFree( dsa_pqg->base.data, dsa_pqg->base.len );
    PORT_ZFree( dsa_pqg, sizeof( PQGParams ) );

    return( CKR_HOST_MEMORY );
}


CK_RV
pk11_fipsPowerUpSelfTest( void )
{
    CK_RV rv;

    /* RC2 Power-Up SelfTest(s). */
    rv = pk11_fips_RC2_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* RC4 Power-Up SelfTest(s). */
    rv = pk11_fips_RC4_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* DES Power-Up SelfTest(s). */
    rv = pk11_fips_DES_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* DES3 Power-Up SelfTest(s). */
    rv = pk11_fips_DES3_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* MD2 Power-Up SelfTest(s). */
    rv = pk11_fips_MD2_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* MD5 Power-Up SelfTest(s). */
    rv = pk11_fips_MD5_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* SHA-1 Power-Up SelfTest(s). */
    rv = pk11_fips_SHA1_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* RSA Power-Up SelfTest(s). */
    rv = pk11_fips_RSA_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* DSA Power-Up SelfTest(s). */
    rv = pk11_fips_DSA_PowerUpSelfTest();

    if( rv != CKR_OK )
        return rv;

    /* Passed Power-Up SelfTest(s). */
    return( CKR_OK );
}

