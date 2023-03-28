#ifndef _CRYPTOT_H_
#define _CRYPTOT_H_
/*
 * cryptot.h - public data structures for the crypto library
 *
 * $Id: cryptot.h,v 1.5 1997/02/28 18:19:53 tomw Exp $
 */

typedef struct DESContextStr DESContext;
typedef struct RC2ContextStr RC2Context;
typedef struct RC4ContextStr RC4Context;
typedef struct RC5ContextStr RC5Context;
typedef struct RSAPublicContextStr RSAPublicContext;
typedef struct RSAPrivateContextStr RSAPrivateContext;
typedef struct SGNContextStr SGNContext;
typedef struct VFYContextStr VFYContext;
typedef struct SGNDigestInfoStr SGNDigestInfo;

/* RC2 operation modes */
#define SEC_RC2					0
#define SEC_RC2_CBC				1

/* RC5 operation modes */
#define SEC_RC5					0
#define SEC_RC5_CBC				1

/* DES operation modes */
#define SEC_DES					0
#define SEC_DES_CBC				1
#define SEC_DES_EDE3				2
#define SEC_DES_EDE3_CBC			3

/*
 * RSA block types
 *
 * The actual values are important -- they are fixed, *not* arbitrary.
 * The explicit value assignments are not needed (because C would give
 * us those same values anyway) but are included as a reminder...
 */
typedef enum {
    RSA_BlockPrivate0 = 0,	/* unused, really */
    RSA_BlockPrivate = 1,	/* pad for a private-key operation */
    RSA_BlockPublic = 2,	/* pad for a public-key operation */
    RSA_BlockOAEP = 3,	/* use OAEP padding */
				/* XXX is this only for a public-key
				   operation? If so, add "Public" */
    RSA_BlockRaw = 4,		/* simply justify the block appropriately */
    RSA_BlockTotal
} RSA_BlockType;

/*
** A PKCS#1 digest-info object
*/
struct SGNDigestInfoStr {
    PRArenaPool *arena;
    SECAlgorithmID digestAlgorithm;
    SECItem digest;
};

/******************************************/
/*
** DES symmetric block cypher
*/

#define DES_KEY_LENGTH	8

extern DERTemplate SGNDigestInfoTemplate[];

#endif /* _CRYPTOT_H_ */
