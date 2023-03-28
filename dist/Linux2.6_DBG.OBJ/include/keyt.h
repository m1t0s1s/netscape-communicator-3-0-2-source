#ifndef _KEYT_H_
#define _KEYT_H_
/*
 * keyt.h - public data structures for the private key library
 *
 * $Id: keyt.h,v 1.7.2.2 1997/03/22 23:38:48 jwz Exp $
 */

#include "secasn1t.h"

typedef struct SECKEYEncryptedPrivateKeyInfoStr SECKEYEncryptedPrivateKeyInfo;
typedef struct SECKEYPrivateKeyInfoStr SECKEYPrivateKeyInfo;
typedef struct SECKEYPrivateKeyStr SECKEYPrivateKey;
typedef struct SECKEYPublicKeyStr SECKEYPublicKey;
typedef struct SECKEYLowPrivateKeyStr SECKEYLowPrivateKey;
typedef struct SECKEYLowPublicKeyStr SECKEYLowPublicKey;
typedef struct SECKEYKeyDBHandleStr SECKEYKeyDBHandle;
typedef struct SECKEYDBKeyStr SECKEYDBKey;

#include "secmodt.h"
#include "pkcs11t.h"

/*
** A PKCS#8 private key info object
*/
struct SECKEYPrivateKeyInfoStr {
    PRArenaPool *arena;
    SECItem version;
    SECAlgorithmID algorithm;
    SECItem privateKey;
};
#define SEC_PRIVATE_KEY_INFO_VERSION		0	/* what we *create* */

/*
** A PKCS#8 private key info object
*/
struct SECKEYEncryptedPrivateKeyInfoStr {
    PRArenaPool *arena;
    SECAlgorithmID algorithm;
    SECItem encryptedData;
};

typedef enum { nullKey, rsaKey, dsaKey, fortezzaKey, dhKey } KeyType;

typedef struct RSAPublicKeyStr RSAPublicKey;
typedef struct RSAPrivateKeyStr RSAPrivateKey;

struct RSAPublicKeyStr {
    SECItem modulus;
    SECItem publicExponent;
};

struct RSAPrivateKeyStr {
    SECItem modulus;
    SECItem version;
    SECItem publicExponent;
    SECItem privateExponent;
    SECItem prime[2];
    SECItem primeExponent[2];
    SECItem coefficient;
};

typedef struct PQGParamsStr PQGParams;
typedef struct DSAPublicKeyStr DSAPublicKey;
typedef struct DSAPrivateKeyStr DSAPrivateKey;

struct PQGParamsStr {
    PRArenaPool *arena;
    SECItem prime;    /* p */
    SECItem subPrime; /* q */
    SECItem base;     /* g */
};

struct DSAPublicKeyStr {
    PQGParams params;
    SECItem publicValue;
};

struct DSAPrivateKeyStr {
    PQGParams params;
    SECItem publicValue;
    SECItem privateValue;
};

typedef struct FortezzaPublicKeyStr FortezzaPublicKey;
typedef struct FortezzaPrivateKeyStr FortezzaPrivateKey;

struct FortezzaPublicKeyStr {
    int      KEAversion;
    int      DSSversion;
    unsigned char    KMID[8];
    SECItem clearance;
    SECItem KEApriviledge;
    SECItem DSSpriviledge;
    SECItem KEAKey;
    SECItem DSSKey;
    SECItem p;
    SECItem q;
    SECItem g;
};

struct FortezzaPrivateKeyStr {
     int certificate;
     unsigned char serial[8];
     int socket;
};

/*
** An RSA public key object.
*/
struct SECKEYLowPublicKeyStr {
    PRArenaPool *arena;
    KeyType keyType ;
    union {
        RSAPublicKey rsa;
	DSAPublicKey dsa;
    } u;
};

/*
** A Generic  public key object.
*/
struct SECKEYPublicKeyStr {
    PRArenaPool *arena;
    KeyType keyType;
    PK11SlotInfo *pkcs11Slot;
    CK_OBJECT_HANDLE pkcs11ID;
    union {
        RSAPublicKey rsa;
	DSAPublicKey dsa;
        FortezzaPublicKey fortezza;
    } u;
};

/*
** Low Level private key object
** This is only used by the raw Crypto engines (crypto), keydb (keydb),
** and PKCS #11. Everyone else uses the high level key structure.
*/
struct SECKEYLowPrivateKeyStr {
    PRArenaPool *arena;
    KeyType keyType;
    union {
        RSAPrivateKey rsa;
	DSAPrivateKey dsa;
        FortezzaPrivateKey fortezza;
    } u;
};

/*
** A generic key structure
*/ 
struct SECKEYPrivateKeyStr {
    PRArenaPool *arena;
    KeyType keyType;
    PK11SlotInfo *pkcs11Slot;	/* pkcs11 slot this key lives in */
    CK_OBJECT_HANDLE pkcs11ID;  /* ID of pkcs11 object */
    PRBool pkcs11IsTemp;	/* temp pkcs11 object, delete it when done */
    void *wincx;		/* context for errors and pw prompts */
};

/*
 * a key in/for the data base
 */
struct SECKEYDBKeyStr {
    PRArenaPool *arena;
    int version;
    char *nickname;
    SECItem salt;
    SECItem derPK;
};

/*
 * Handle structure for open key databases
 */
struct SECKEYKeyDBHandleStr {
    DB *db;
    DB *updatedb;		/* used when updating an old version */
    SECItem *global_salt;	/* password hashing salt for this db */
    int version;		/* version of the database */
    PRBool dialog_pending;	/* is the "setup password" dialog up? */
    PRBool checked;
};

#define PRIVATE_KEY_DB_FILE_VERSION 3

#define SEC_PRIVATE_KEY_VERSION			0	/* what we *create* */

/*
** Typedef for callback to get a password "key".
*/
typedef SECItem * (* SECKEYGetPasswordKey)(void *arg,
					   SECKEYKeyDBHandle *handle);

/*
** Typedef for callback for traversing key database.
**      "key" is the key used to index the data in the database (nickname)
**      "data" is the key data
**      "pdata" is the user's data 
*/
typedef SECStatus (* SECKEYTraverseKeysFunc)(DBT *key, DBT *data, void *pdata);

extern const SEC_ASN1Template SECKEY_EncryptedPrivateKeyInfoTemplate[];
extern const SEC_ASN1Template SECKEY_RSAPublicKeyTemplate[];
extern const SEC_ASN1Template SECKEY_RSAPrivateKeyTemplate[];
extern const SEC_ASN1Template SECKEY_DSAPublicKeyTemplate[];
extern const SEC_ASN1Template SECKEY_DSAPrivateKeyTemplate[];
extern const SEC_ASN1Template SECKEY_PrivateKeyInfoTemplate[];
extern const SEC_ASN1Template SECKEY_PointerToEncryptedPrivateKeyInfoTemplate[];
extern const SEC_ASN1Template SECKEY_PQGParamsTemplate[];

#endif /* _KEYT_H_ */
