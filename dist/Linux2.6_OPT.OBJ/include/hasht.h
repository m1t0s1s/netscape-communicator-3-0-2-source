#ifndef _HASHT_H_
#define _HASHT_H_
/*
 * hasht.h - public data structures for the hashing library
 *
 * $Id: hasht.h,v 1.3.20.1 1997/04/25 23:59:58 jwz Exp $
 */

/* Opaque objects */
typedef struct MD2ContextStr MD2Context;
typedef struct MD5ContextStr MD5Context;
typedef struct SHA1ContextStr SHA1Context;
typedef struct SECHashObjectStr SECHashObject;

/*
 * The hash functions the security library supports
 * NOTE the order must match the definition of SECHashObjects[]!
 */
typedef enum {
    HASH_AlgNULL,
    HASH_AlgMD2,
    HASH_AlgMD5,
    HASH_AlgSHA1,
    HASH_AlgTOTAL
} HASH_HashType;

/*
 * Number of bytes each hash algorithm produces
 */
#define MD2_LENGTH	16
#define MD5_LENGTH	16
#define SHA1_LENGTH	20

/*
 * Structure to hold hash computation info and routines
 */
struct SECHashObjectStr {
    unsigned int length;
    void * (*create)(void);
    void * (*clone)(void *);
    void (*destroy)(void *, PRBool);
    void (*begin)(void *);
    void (*update)(void *, const unsigned char *, unsigned int);
    void (*end)(void *, unsigned char *, unsigned int *, unsigned int);
};

extern SECHashObject SECHashObjects[];

/*only those functions below the PKCS #11 line should use SECRawHashObjects*/
extern SECHashObject SECRawHashObjects[];

#endif /* _HASHT_H_ */
