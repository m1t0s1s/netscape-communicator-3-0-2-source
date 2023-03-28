/*
 * This file implements the Symkey wrapper and the PKCS context
 * Interfaces.
 */
#include "seccomon.h"
#include "secmod.h"
#include "prlock.h"
#include "secmodi.h"
#include "pkcs11.h"
#include "pk11func.h"
#include "secitem.h"
#include "key.h"
#include "rsa.h"
#include "secpkcs5.h"
#include "secasn1.h"
#include "sechash.h"

extern int SEC_ERROR_INVALID_KEY;
extern int SEC_ERROR_BAD_KEY;
extern int SEC_ERROR_NO_MODULE;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_LIBRARY_FAILURE;

#define PAIRWISE_SECITEM_TYPE			siBuffer
#define PAIRWISE_DIGEST_LENGTH			SHA1_LENGTH /* 160-bits */
#define PAIRWISE_MESSAGE_LENGTH			20          /* 160-bits */

/*
 * strip leading zero's from key material
 */
static void
pk11_SignedToUnsigned(CK_ATTRIBUTE *attrib) {
    char *ptr = (char *)attrib->pValue;
    unsigned long len = attrib->ulValueLen;

    while (len && (*ptr == 0)) {
	len--;
	ptr++;
    }
    attrib->pValue = ptr;
    attrib->ulValueLen = len;
}

/*
 * get a new session on a slot. If we run out of session, use the slot's
 * 'exclusive' session. In this case owner becomes false.
 */
static CK_SESSION_HANDLE
pk11_GetNewSession(PK11SlotInfo *slot,PRBool *owner)
{
    CK_SESSION_HANDLE session;
    *owner =  PR_TRUE;
    if ( PK11_GETTAB(slot)->C_OpenSession(slot->slotID,CKF_SERIAL_SESSION, 
			slot,pk11_notify,&session) != CKR_OK) {
	*owner = PR_FALSE;
	session = slot->session;
    }

    return session;
}

SECStatus
PK11_CreateNewObject(PK11SlotInfo *slot,CK_ATTRIBUTE *template, int count,
						CK_OBJECT_HANDLE *objectID)
{
	CK_SESSION_HANDLE rwsession;
	CK_RV crv;
	SECStatus rv = SECSuccess;

	rwsession = PK11_GetRWSession(slot);
	crv = PK11_GETTAB(slot)->C_CreateObject(rwsession, template,
							count,objectID);
	if(crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    rv = SECFailure;
	}
	PK11_RestoreROSession(slot, rwsession);
	return rv;
}

/*
 * create a symetric key:
 *      Slot is the slot to create the key in.
 *      type is the mechainism type 
 */
PK11SymKey *
PK11_CreateSymKey(PK11SlotInfo *slot, CK_MECHANISM_TYPE type, void *wincx)
{

    PK11SymKey *symKey = (PK11SymKey *)PORT_Alloc(sizeof(PK11SymKey));
    if (symKey == NULL) {
	return NULL;
    }
#ifdef PKCS11_USE_THREADS
    symKey->refLock = PR_NewLock(0);
    if (symKey->refLock == NULL) {
	PORT_Free(symKey);
	return NULL;
    }
#else
    symKey->refLock = NULL;
#endif
    symKey->type = type;
    symKey->data.data = NULL;
    symKey->data.len = 0;
    symKey->objectID = CK_INVALID_KEY;
    symKey->slot = slot;
    symKey->cx = wincx;
    symKey->size = 0;
    symKey->refCount = 1;
    PK11_ReferenceSlot(slot);
    return symKey;
}

/*
 * destroy a symetric key
 */
void
PK11_FreeSymKey(PK11SymKey *symKey)
{
    PRBool destroy = PR_FALSE;

    PK11_USE_THREADS(PR_Lock(symKey->refLock);)
    if (symKey->refCount-- == 1) {
	destroy= PR_TRUE;
    }
    PK11_USE_THREADS(PR_Unlock(symKey->refLock);)
    if (destroy) {
	if (symKey->objectID != CK_INVALID_KEY) {
	    (void) PK11_GETTAB(symKey->slot)->
		C_DestroyObject(symKey->slot->session, symKey->objectID);
	}
	if (symKey->data.data) {
	    PORT_Memset(symKey->data.data, 0, symKey->data.len);
	    PORT_Free(symKey->data.data);
	}
	PK11_USE_THREADS(PR_DestroyLock(symKey->refLock);)
	PK11_FreeSlot(symKey->slot);
	PORT_Free(symKey);
    }
}

PK11SymKey *
PK11_ReferenceSymKey(PK11SymKey *symKey)
{
    PK11_USE_THREADS(PR_Lock(symKey->refLock);)
    symKey->refCount++;
    PK11_USE_THREADS(PR_Unlock(symKey->refLock);)
    return symKey;
}

/*
 * turn key bits into an appropriate key object
 */
PK11SymKey *
PK11_ImportSymKey(PK11SlotInfo *slot, CK_MECHANISM_TYPE type,
                     CK_ATTRIBUTE_TYPE operation, SECItem *key,void *wincx)
{
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_BBOOL ckfalse = FALSE; /* sigh */
    CK_ATTRIBUTE keyTemplate[] = {
	{ CKA_CLASS, NULL, 0},
	{ CKA_KEY_TYPE,NULL, 0},
	{ CKA_SENSITIVE,NULL, 0},
	{ 0, NULL, 0},
	{ CKA_VALUE,NULL, 0},
    };
    int keyTemplateCount = sizeof(keyTemplate)/sizeof(keyTemplate[0]);
    PK11SymKey *symKey;
    CK_ATTRIBUTE *attrs = keyTemplate;
    CK_RV crv;

    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, sizeof(keyClass) ); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, sizeof(keyType) ); attrs++;
    PK11_SETATTRS(attrs, CKA_SENSITIVE, &ckfalse, sizeof(CK_BBOOL) ); attrs++;
    PK11_SETATTRS(attrs, operation, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_VALUE, key->data, key->len);

    symKey = PK11_CreateSymKey(slot,type,wincx);
    if (symKey == NULL) {
	return NULL;
    }

    symKey->size = key->len;

    if (SECITEM_CopyItem(NULL,&symKey->data,key) != SECSuccess) {
	PK11_FreeSymKey(symKey);
	return NULL;
    }

    keyType = PK11_GetKeyType(type,key->len);

    /* import the keys */
    crv = PK11_GETTAB(slot)->C_CreateObject(slot->session, keyTemplate,
			keyTemplateCount,&symKey->objectID) ;
    if ( crv != CKR_OK) {
	PK11_FreeSymKey(symKey);
	PORT_SetError( PK11_MapError(crv));
	return NULL;
    }

    return symKey;
}

/*
 * import a public key into the desired slot
 */
CK_OBJECT_HANDLE
pk11_ImportTokenPublicKey(PK11SlotInfo *slot, 
				SECKEYPublicKey *pubKey, PRBool isToken)
{
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_BBOOL ckfalse = FALSE; /* sigh */
    CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_OBJECT_HANDLE objectID;
    CK_ATTRIBUTE *template = NULL;
    CK_ATTRIBUTE *signedattr = NULL;
    int signedcount = 0;
    int templateCount = 0;
    CK_RV crv;

    /* if we already have an object in the desired slot, use it */
    if (!isToken && pubKey->pkcs11Slot == slot) {
	return pubKey->pkcs11ID;
    }

    /* free the existing key */
    if (pubKey->pkcs11Slot != NULL) {
	PK11SlotInfo *oSlot = pubKey->pkcs11Slot;
	(void) PK11_GETTAB(oSlot)->C_DestroyObject(oSlot->session,
							pubKey->pkcs11ID);
	PK11_FreeSlot(oSlot);
	pubKey->pkcs11Slot = NULL;
    }

    /* now import the key */
    {
	CK_ATTRIBUTE rsaTemplate[] = {
	    { CKA_CLASS, NULL, 0 },
	    { CKA_KEY_TYPE, NULL, 0 },
	    { CKA_TOKEN,  NULL, 0 },
	    { CKA_WRAP,  NULL, 0 },
	    { CKA_ENCRYPT, NULL, 0 },
	    { CKA_VERIFY, NULL, 0 },
	    { CKA_MODULUS, NULL, 0 },
	    { CKA_PUBLIC_EXPONENT, NULL, 0 },
	};
        CK_ATTRIBUTE *attrs;

	CK_ATTRIBUTE dsaTemplate[] = {
	    { CKA_CLASS, NULL, 0},
	    { CKA_KEY_TYPE, NULL, 0},
	    { CKA_TOKEN,  NULL, 0 },
	    { CKA_VERIFY, NULL, 0},
	    { CKA_PRIME, NULL, 0},
	    { CKA_SUBPRIME, NULL, 0},
	    { CKA_BASE, NULL, 0},
	    { CKA_VALUE, NULL, 0},
	};
#ifdef notdef
	CK_ATTRIBUTE dhTemplate[] = {
	    { CKA_CLASS, &keyClass, sizeof(keyClass) },
	    { CKA_KEY_TYPE, &keyType, sizeof(keyType) },
	    { CKA_DERIVE, &cktrue, sizeof(CK_BBOOL) },
	    { CKA_PRIME,    pubKey->u.dh.p.data,pubKey->u.dh.p.len},
	    { CKA_BASE,     pubKey->u.dh.g.data,pubKey->u.dh.g.len},
	    { CKA_VALUE,    pubKey->u.dh.key.data, pubKey->u.dh.key.len}
	};
#endif

        switch (pubKey->keyType) {
        case rsaKey:
	    attrs = template = rsaTemplate;
	    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
						sizeof(keyClass) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, 
						sizeof(keyType) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_TOKEN, isToken ? &cktrue : &ckfalse,
						 sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_WRAP, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_ENCRYPT, &cktrue, 
						sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_VERIFY, &cktrue, sizeof(CK_BBOOL)); attrs++;
 	    signedattr = attrs;
 	    signedcount = 2;
	    PK11_SETATTRS(attrs, CKA_MODULUS, pubKey->u.rsa.modulus.data,
					 pubKey->u.rsa.modulus.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PUBLIC_EXPONENT, 
	     	pubKey->u.rsa.publicExponent.data,
					 pubKey->u.rsa.publicExponent.len);
	    templateCount = sizeof(rsaTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_RSA;
	    break;
        case dsaKey:
	    attrs = template = dsaTemplate;
	    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
						sizeof(keyClass)); attrs++;
	    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, 
						sizeof(keyType)); attrs++;
	    PK11_SETATTRS(attrs, CKA_TOKEN, isToken ? &cktrue : &ckfalse,
						 sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_VERIFY, &cktrue, sizeof(CK_BBOOL)); attrs++;
 	    signedattr = attrs;
 	    signedcount = 4;
	    PK11_SETATTRS(attrs, CKA_PRIME,    pubKey->u.dsa.params.prime.data,
				pubKey->u.dsa.params.prime.len); attrs++;
	    PK11_SETATTRS(attrs,CKA_SUBPRIME,pubKey->u.dsa.params.subPrime.data,
				pubKey->u.dsa.params.subPrime.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_BASE,  pubKey->u.dsa.params.base.data,
					pubKey->u.dsa.params.base.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_VALUE,    pubKey->u.dsa.publicValue.data, 
					pubKey->u.dsa.publicValue.len);
	    templateCount = sizeof(dsaTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_DSA;
	    break;
#ifdef notdef
        case dhKey:
	    template = dhTemplate;
	    templateCount = sizeof(dhTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_DH;
 	    signedattr = attrs;
 	    signedcount = 4;
	    break;
#endif
	/* what about fortezza??? */
	default:
	    PORT_SetError( SEC_ERROR_BAD_KEY );
	    return CK_INVALID_KEY;
	}

	/* not all PKCS #11 implementations like unsigned values */
	/* Hack until internal PKCS #11 can handle unsigned input */
	if (isToken) {
	    CK_ATTRIBUTE *ap;
	    for (ap=signedattr; signedcount; ap++, signedcount--) {
		pk11_SignedToUnsigned(ap);
	    }
	} 
	crv = PK11_GETTAB(slot)->C_CreateObject(slot->session, template,
						templateCount,&objectID);
	if ( crv != CKR_OK) {
	    PORT_SetError (PK11_MapError(crv));
	    return CK_INVALID_KEY;
	}
    }

    pubKey->pkcs11ID = objectID;
    pubKey->pkcs11Slot = PK11_ReferenceSlot(slot);

    return objectID;
}

CK_OBJECT_HANDLE
PK11_ImportPublicKey(PK11SlotInfo *slot, SECKEYPublicKey *pubKey)
{
    return pk11_ImportTokenPublicKey(slot, pubKey, PR_FALSE);
}

/*
 * extract a symetric key value. NOTE: if the key is sensitive, we will
 * not be able to do this operation. This function is used to move
 * keys from one token to another */
SECStatus
PK11_ExtractKeyValue(PK11SymKey *symKey)
{
    int len;
    CK_RV crv;
    CK_ATTRIBUTE keyTemplate[] = {
	{ CKA_VALUE, NULL, 0 },
    };

    if (symKey->data.data != NULL) return SECSuccess;

    if (symKey->slot == NULL) {
	PORT_SetError( SEC_ERROR_INVALID_KEY );
	return SECFailure;
    }

    crv = PK11_GETTAB(symKey->slot)->C_GetAttributeValue(symKey->slot->session,
		symKey->objectID,keyTemplate,1) ;
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }

    symKey->data.data = PORT_Alloc(keyTemplate[0].ulValueLen);
    if (symKey->data.data == NULL) return SECFailure;

    keyTemplate[0].pValue = symKey->data.data;
    len = keyTemplate[0].ulValueLen;
    if (symKey->size == 0) symKey->size = len;
    crv = PK11_GETTAB(symKey->slot)->C_GetAttributeValue(symKey->slot->session,
		symKey->objectID,keyTemplate,1) ;
    if (crv != CKR_OK) {
	/* the data is not valid, make sure no one tries to use it*/
	PORT_Free(symKey->data.data);
	symKey->data.data = NULL;
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }

    /* Make sure the module is consistant */
    PORT_Assert(len >= (int)keyTemplate[0].ulValueLen);
    symKey->data.len = keyTemplate[0].ulValueLen;
    return SECSuccess;
}

SECItem *
PK11_GetKeyData(PK11SymKey *symKey)
{
    return &symKey->data;
}

/*
 * extract a public key from a slot and id
 */
SECKEYPublicKey *
PK11_ExtractPublicKey(PK11SlotInfo *slot,KeyType keyType,CK_OBJECT_HANDLE id)
{
    CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY;
    CK_KEY_TYPE pk11KeyType = CKK_GENERIC_SECRET;
    PRArenaPool *arena;
    SECKEYPublicKey *pubKey;
    int templateCount = 0;
    CK_RV crv;

    /* if we didn't know the key type, get it */
    if (keyType== nullKey) {
	CK_ATTRIBUTE initTemplate[1];
	int initCount = sizeof(initTemplate)/sizeof(initTemplate[0]);
	PK11_SETATTRS(initTemplate, CKA_KEY_TYPE, &pk11KeyType, 
							sizeof(pk11KeyType));

	crv = PK11_GETTAB(slot)->C_GetAttributeValue(slot->session,
						id,initTemplate,initCount);
	if (crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    return NULL;
	}
	switch (pk11KeyType) {
	case CKK_RSA:
	    keyType = rsaKey;
	    break;
	case CKK_DSA:
	    keyType = dsaKey;
	    break;
	default:
	    PORT_SetError( SEC_ERROR_BAD_KEY );
	    return NULL;
	}
    }


    /* now we need to create space for the public key */
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;


    pubKey = (SECKEYPublicKey *) 
			PORT_ArenaZAlloc(arena, sizeof(SECKEYPublicKey));
    if (pubKey == NULL) {
	PORT_FreeArena (arena, PR_FALSE);
	return NULL;
    }

    pubKey->arena = arena;
    pubKey->keyType = keyType;
    pubKey->pkcs11Slot = PK11_ReferenceSlot(slot);
    pubKey->pkcs11ID = id;

    switch (pubKey->keyType) {
    case rsaKey:
	{
	    CK_ATTRIBUTE rsaTemplate[] = {
		{ CKA_CLASS, NULL, 0 },
		{ CKA_KEY_TYPE, NULL, 0 },
		{ CKA_MODULUS, NULL, 0},
		{ CKA_PUBLIC_EXPONENT, NULL, 0},
	    };
	    CK_ATTRIBUTE *attrs = rsaTemplate;
	    unsigned char *dataPtr;

	    templateCount = sizeof(rsaTemplate)/sizeof(CK_ATTRIBUTE);
	    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
						sizeof(keyClass)); attrs++;
	    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &pk11KeyType, 
						sizeof(pk11KeyType) ); attrs++;
	    crv = PK11_GETTAB(slot)->C_GetAttributeValue
			(slot->session, id, rsaTemplate, templateCount);
	    if (crv != CKR_OK) break;

	    if ((keyClass != CKO_PUBLIC_KEY) || (pk11KeyType != CKK_RSA)) {
		crv = CKR_OBJECT_HANDLE_INVALID;
		break;
	    } 
	    pubKey->u.rsa.modulus.len = rsaTemplate[2].ulValueLen;
	    dataPtr = PORT_ArenaAlloc(arena,pubKey->u.rsa.modulus.len+1);
	    if ( dataPtr == NULL) {
		crv = CKR_HOST_MEMORY;
		break;
	    } 
	    *dataPtr = 0;
	    pubKey->u.rsa.modulus.data = dataPtr+1;
	    rsaTemplate[2].pValue = pubKey->u.rsa.modulus.data;
	    pubKey->u.rsa.publicExponent.len = rsaTemplate[3].ulValueLen;
	    dataPtr = PORT_ArenaAlloc(arena,pubKey->u.rsa.publicExponent.len+1);
	    if ( dataPtr == NULL) {
		crv = CKR_HOST_MEMORY;
		break;
	    } 
	    *dataPtr = 0;
	    pubKey->u.rsa.publicExponent.data = dataPtr+1;
	    rsaTemplate[3].pValue = pubKey->u.rsa.publicExponent.data;
	    crv = PK11_GETTAB(slot)->C_GetAttributeValue
			(slot->session, id, rsaTemplate, templateCount);

	    if (pubKey->u.rsa.modulus.data[0] & 0x80) {
		pubKey->u.rsa.modulus.data = pubKey->u.rsa.modulus.data-1;
		pubKey->u.rsa.modulus.len++;
	    }
	    if (pubKey->u.rsa.publicExponent.data[0] & 0x80) {
		pubKey->u.rsa.publicExponent.data = 
				pubKey->u.rsa.publicExponent.data-1;
		pubKey->u.rsa.publicExponent.len++;
	    }
	}
	break;
    case dsaKey:
	{
	    CK_ATTRIBUTE dsaTemplate[] = {
		{ CKA_CLASS, NULL, 0},
		{ CKA_KEY_TYPE, NULL, 0},
		{ CKA_PRIME, NULL, 0 },
		{ CKA_SUBPRIME, NULL, 0 },
		{ CKA_BASE, NULL, 0 },
		{ CKA_VALUE, NULL, 0 },
	    };
	    CK_ATTRIBUTE *attrs = dsaTemplate;
	    unsigned char *dataPtr;

	    templateCount = sizeof(dsaTemplate)/sizeof(CK_ATTRIBUTE);
	    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
						sizeof(keyClass)); attrs++;
	    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &pk11KeyType, 
						sizeof(pk11KeyType) ); attrs++;
	    crv = PK11_GETTAB(slot)->C_GetAttributeValue
			(slot->session, id, dsaTemplate, templateCount);
	    if (crv != CKR_OK) break;

	    if ((keyClass != CKO_PUBLIC_KEY) || (pk11KeyType != CKK_DSA)) {
		crv = CKR_OBJECT_HANDLE_INVALID;
		break;
	    } 
	    pubKey->u.dsa.params.prime.len = dsaTemplate[2].ulValueLen;
	    dataPtr = PORT_ArenaAlloc(arena,pubKey->u.dsa.params.prime.len+1);
	    if ( dataPtr == NULL) {
		crv = CKR_HOST_MEMORY;
		break;
	    } 
	    *dataPtr = 0;
	    pubKey->u.dsa.params.prime.data = dataPtr+1;
	    dsaTemplate[2].pValue = pubKey->u.dsa.params.prime.data;

	    pubKey->u.dsa.params.subPrime.len = dsaTemplate[3].ulValueLen;
	    dataPtr=PORT_ArenaAlloc(arena,pubKey->u.dsa.params.subPrime.len+1);
	    if ( dataPtr == NULL) {
		crv = CKR_HOST_MEMORY;
		break;
	    } 
	    *dataPtr = 0;
	    pubKey->u.dsa.params.subPrime.data = dataPtr+1;
	    dsaTemplate[3].pValue = pubKey->u.dsa.params.subPrime.data;

	    pubKey->u.dsa.params.base.len = dsaTemplate[4].ulValueLen;
	    dataPtr = PORT_ArenaAlloc(arena,pubKey->u.dsa.params.base.len+1);
	    if ( dataPtr == NULL) {
		crv = CKR_HOST_MEMORY;
		break;
	    } 
	    *dataPtr = 0;
	    pubKey->u.dsa.params.base.data = dataPtr+1;
	    dsaTemplate[4].pValue = pubKey->u.dsa.params.base.data;

	    pubKey->u.dsa.publicValue.len = dsaTemplate[5].ulValueLen;
	    dataPtr = PORT_ArenaAlloc(arena,pubKey->u.dsa.publicValue.len+1);
	    if ( dataPtr == NULL) {
		crv = CKR_HOST_MEMORY;
		break;
	    } 
	    *dataPtr = 0;
	    pubKey->u.dsa.publicValue.data = dataPtr+1;
	    dsaTemplate[5].pValue = pubKey->u.dsa.publicValue.data;

	    crv = PK11_GETTAB(slot)->C_GetAttributeValue
			(slot->session, id, dsaTemplate, templateCount);

	    if (pubKey->u.dsa.params.prime.data[0] & 0x80) {
		pubKey->u.dsa.params.prime.data = 
				pubKey->u.dsa.params.prime.data-1;
		pubKey->u.dsa.params.prime.len++;
	    }
	    if (pubKey->u.dsa.params.subPrime.data[0] & 0x80) {
		pubKey->u.dsa.params.subPrime.data = 
				pubKey->u.dsa.params.subPrime.data-1;
		pubKey->u.dsa.params.subPrime.len++;
	    }
	    if (pubKey->u.dsa.params.base.data[0] & 0x80) {
		pubKey->u.dsa.params.base.data = 
				pubKey->u.dsa.params.base.data-1;
		pubKey->u.dsa.params.base.len++;
	    }
	    if (pubKey->u.dsa.publicValue.data[0] & 0x80) {
		pubKey->u.dsa.publicValue.data = 
				pubKey->u.dsa.publicValue.data-1;
		pubKey->u.dsa.publicValue.len++;
	    }
	}
	break;
#ifdef notdef
    case dhKey:

	CK_ATTRIBUTE dhTemplate[] = {
	    { CKA_CLASS, &keyClass, sizeof(keyClass) },
	    { CKA_KEY_TYPE, &keyType, sizeof(keyType) },
	    { CKA_PRIME,    pubKey->u.dh.p.data,pubKey->u.dh.p.len},
	    { CKA_BASE,     pubKey->u.dh.g.data,pubKey->u.dh.g.len},
	    { CKA_VALUE,    pubKey->u.dh.key.data, pubKey->u.dh.key.len}
	};
	    template = dhTemplate;
	    templateCount = sizeof(dhTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_DH;
	    break;
#else
    case dhKey:
    case fortezzaKey:
    case nullKey:
#endif
    default:
	crv = CKR_OBJECT_HANDLE_INVALID;
	break;
    }

    if (crv != CKR_OK) {
	PORT_FreeArena(arena,PR_FALSE);
	PK11_FreeSlot(slot);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    return pubKey;
}

/*
 * Build a Private Key structure from raw PKCS #11 information.
 */
SECKEYPrivateKey *
PK11_MakePrivKey(PK11SlotInfo *slot, KeyType keyType, 
			PRBool isTemp, CK_OBJECT_HANDLE privID, void *wincx)
{
    PRArenaPool *arena;
    SECKEYPrivateKey *privKey;

    /* don't know? look it up */
    if (keyType == nullKey) {
	CK_KEY_TYPE pk11Type = CKK_RSA;
	CK_BBOOL isToken = PR_TRUE;
	CK_ATTRIBUTE dattrs[] = {
	   { CKA_KEY_TYPE, NULL, 0},
	   { CKA_TOKEN,  NULL, 0},
	};
	CK_ATTRIBUTE *attrs = dattrs;
	PK11_SETATTRS(attrs, CKA_KEY_TYPE, &pk11Type,sizeof(pk11Type));attrs++;
	PK11_SETATTRS(attrs, CKA_TOKEN, &isToken, sizeof(isToken));
	PK11_GETTAB(slot)->C_GetAttributeValue(slot->session,privID,dattrs,2);
	isTemp = !isToken;
	switch (pk11Type) {
	case CKK_RSA: keyType = rsaKey; break;
	case CKK_DSA: keyType = dsaKey; break;
	case CKK_DH: keyType = dhKey; break;
	case CKK_KEA: keyType = fortezzaKey; break;
	default:
		break;
	}
    }

    /* now we need to create space for the private key */
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;

    privKey = (SECKEYPrivateKey *) 
			PORT_ArenaZAlloc(arena, sizeof(SECKEYPrivateKey));
    if (privKey == NULL) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }

    privKey->arena = arena;
    privKey->keyType = keyType;
    privKey->pkcs11Slot = PK11_ReferenceSlot(slot);
    privKey->pkcs11ID = privID;
    privKey->pkcs11IsTemp = isTemp;
    privKey->wincx = wincx;

    return privKey;
}

/* return the keylength if possible.  '0' if not */
unsigned int
PK11_GetKeyLength(PK11SymKey *key)
{
   if (key->size != 0) return key->size ;
   if (key->data.data == NULL) {
	PK11_ExtractKeyValue(key);
   }
   /* key is probably secret. Look up it's type and length */
   /*  this is PKCS #11 version 2.0 functionality. */
   if (key->size == 0) {
	CK_ULONG keyLength = 0;
	CK_RV crv;
	CK_ATTRIBUTE key_attrs = {CKA_VALUE_LEN, NULL, 0};
	PK11_SETATTRS(&key_attrs, CKA_VALUE_LEN, 
						&keyLength, sizeof(keyLength));
	crv = PK11_GETTAB(key->slot)->C_GetAttributeValue(key->slot->session,
			key->objectID,&key_attrs, 1);
	/* doesn't have a length field, check the known PKCS #11 key types,
	 * which don't have this field */
	if (crv != CKR_OK) {
	    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
	    PK11_SETATTRS(&key_attrs, CKA_KEY_TYPE,&keyType, sizeof(keyType));
	    switch (keyType) {
	    case CKK_DES: key->size = 8; break;
	    case CKK_DES2: key->size = 16; break;
	    case CKK_DES3: key->size = 24; break;
	    case CKK_SKIPJACK: key->size = 10; break;
	    case CKK_BATON: key->size = 20; break;
	    case CKK_JUNIPER: key->size = 20; break;
	    default: break;
	    }
	} else {
	    key->size = (unsigned int)keyLength;
	}
    }
	
   return key->size;
}

/* return the strenght of a key. This is different from length in that
 * 1) it returns the size in bits, and 2) it returns only the secret portions
 * of the key minus any checksums or parity.
 */
unsigned int
PK11_GetKeyStrength(PK11SymKey *key) 
{
     int size=0;
     switch (PK11_GetKeyType(key->type,0)) {
     case CKK_DES:
	return 56;
     case CKK_DES3:
     case CKK_DES2:
	size = PK11_GetKeyLength(key);
	if (size == 16) {
	   /* double des */
	   return 112; /* 16*7 */
	}
	return 168;
    default:
	break;
    }
    return PK11_GetKeyLength(key) * 8;
}


/*
 * get the length of a signature object based on the key
 */
int
PK11_SignatureLen(SECKEYPrivateKey *key)
{
    CK_ATTRIBUTE template = { CKA_MODULUS, NULL, 0 };
    PK11SlotInfo *slot = key->pkcs11Slot;
    int val;

    switch (key->keyType) {
    case rsaKey:
	val = PK11_GetPrivateModulusLen(key);
	if (val == -1) {
	    return MAX_RSA_MODULUS_LEN; /* failed, assume the longest len */
	}
	return (unsigned long) val;
	
    case fortezzaKey:
    case dsaKey:
	return 40;

    default:
	break;
    }
    PORT_SetError( SEC_ERROR_INVALID_KEY );
    return 0;
}

/*
 * Get the modulus length for raw parsing
 */
int
PK11_GetPrivateModulusLen(SECKEYPrivateKey *key)
{
    CK_ATTRIBUTE template = { CKA_MODULUS, NULL, 0 };
    PK11SlotInfo *slot = key->pkcs11Slot;
    CK_RV crv;
    int length;

    switch (key->keyType) {
    case rsaKey:
	crv = PK11_GetAttributes(NULL, slot, key->pkcs11ID, &template, 1);
	if (crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    return -1;
	}
	length = template.ulValueLen;
	if ( *(unsigned char *)template.pValue == 0) {
	    length--;
	}
	if (template.pValue != NULL)
	    PORT_Free(template.pValue);
	return (int) length;
	
    case fortezzaKey:
    case dsaKey:
    default:
	break;
    }
    if (template.pValue != NULL)
	PORT_Free(template.pValue);
    PORT_SetError( SEC_ERROR_INVALID_KEY );
    return -1;
}


/* Make a Key type to an appropriate signing/verification mechanism */
static CK_MECHANISM_TYPE
pk11_mapSignKeyType(KeyType keyType)
{
    switch (keyType) {
    case rsaKey:
	return CKM_RSA_PKCS;
    case fortezzaKey:
    case dsaKey:
	return CKM_DSA;
    default:
	break;
    }
    return CKM_INVALID_MECHANISM;
}

static CK_MECHANISM_TYPE
pk11_mapWrapKeyType(KeyType keyType)
{
    switch (keyType) {
    case rsaKey:
	return CKM_RSA_PKCS;
    /* Add fortezza?? */
    default:
	break;
    }
    return CKM_INVALID_MECHANISM;
}

/*
 * copy a key (or any other object) on a token
 */
CK_OBJECT_HANDLE
PK11_CopyKey(PK11SlotInfo *slot, CK_OBJECT_HANDLE srcObject)
{
    CK_OBJECT_HANDLE destObject;
    CK_RV crv;

    crv = PK11_GETTAB(slot)->C_CopyObject(slot->session,srcObject,NULL,0,
				&destObject);
    if (crv == CKR_OK) return destObject;
    PORT_SetError( PK11_MapError(crv) );
    return CK_INVALID_KEY;
}

/*
 * The next two utilities are to deal with the fact that a given operation
 * may be a multi-slot affair. This creates a new key object that is copied
 * into the new slot.
 */
static PK11SymKey *
pk11_CopyToSlot(PK11SlotInfo *slot,CK_MECHANISM_TYPE type,
		 	CK_ATTRIBUTE_TYPE operation, PK11SymKey *symKey)
{
    SECStatus rv;

    /* Extract the raw key data if possible */
    if (symKey->data.data == NULL) {
	rv = PK11_ExtractKeyValue(symKey);
	/* KEY is sensitive, we're hosed... */
	if (rv != SECSuccess) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}
    }
    return PK11_ImportSymKey(slot, type, operation, &symKey->data, symKey->cx);
}

/*
 * Make sure the slot we are in the correct slot for the operation
 */
static PK11SymKey *
pk11_ForceSlot(PK11SymKey *symKey,CK_MECHANISM_TYPE type,
						CK_ATTRIBUTE_TYPE operation)
{
    PK11SlotInfo *slot = symKey->slot;
    PK11SymKey *newKey = NULL;

    if ((slot== NULL) || !PK11_DoesMechanism(slot,type)) {
	slot = PK11_GetBestSlot(type,symKey->cx);
	if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}
	newKey = pk11_CopyToSlot(slot, type, operation, symKey);
	PK11_FreeSlot(slot);
    }
    return newKey;
}

/*
 * Use the token to Generate a key. keySize must be 'zero' for fixed key
 * length algorithms. NOTE: this means we can never generate a DES2 key
 * from this interface!
 */
PK11SymKey *
PK11_KeyGen(PK11SlotInfo *slot,CK_MECHANISM_TYPE type,int keySize,void *wincx)
{
    CK_ULONG key_size = 0;
    /* we have to use these native types because when we call PKCS 11 modules
     * we have to make sure that we are using the correct sizes for all the
     * parameters. */
    CK_BBOOL ckfalse = FALSE; /* Sigh */
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_ATTRIBUTE genTemplate[] = {
	{ CKA_SENSITIVE, NULL, 0},
	{ CKA_ENCRYPT, NULL, 0},
	/* the next entry must be the last one */
	{ CKA_VALUE_LEN, NULL, 0},
    };
    int count = sizeof(genTemplate)/sizeof(genTemplate[0]);
    CK_MECHANISM mechanism;
    CK_MECHANISM_TYPE key_gen_mechanism;
    PK11SymKey *symKey;
    CK_RV crv;
    CK_ATTRIBUTE *attrs = genTemplate;

    PK11_SETATTRS(attrs, CKA_SENSITIVE, &ckfalse, sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_ENCRYPT, &cktrue, sizeof(CK_BBOOL)); attrs++;
    /* the next entry must be the last one */
    PK11_SETATTRS(attrs, CKA_VALUE_LEN, &key_size, sizeof(key_size)); attrs++;

    /* find a slot to generate the key into */
    if ((slot == NULL) || (!PK11_DoesMechanism(slot,type))) {
        slot = PK11_GetBestSlot(type,wincx);
        if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}
    } else {
	PK11_ReferenceSlot(slot);
    }

    /* Initialize the Key Gen Mechanism */
    key_gen_mechanism = PK11_GetKeyGen(type);
    if (key_gen_mechanism == CKM_FAKE_RANDOM) {
	PK11_FreeSlot(slot);
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return NULL;
    }
    mechanism.mechanism = key_gen_mechanism;
    mechanism.pParameter = NULL;
    mechanism.ulParameterLen = 0;

    key_size = keySize;

    /* if key_size was not specified, the mechanism must know it's own size */
    if (key_size == 0) {
	count--;
    }

    /* get our key Structure */
    symKey = PK11_CreateSymKey(slot,type,wincx);
    PK11_FreeSlot(slot);
    if (symKey == NULL) {
	return NULL;
    }
    symKey->size = keySize;

    crv = PK11_GETTAB(symKey->slot)->C_GenerateKey(symKey->slot->session,
			 &mechanism, genTemplate, count, &symKey->objectID);
    if (crv != CKR_OK) {
	PK11_FreeSymKey(symKey);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }
    return symKey;
}

CK_BBOOL
PK11_HasAttributeSet( PK11SlotInfo *slot, CK_OBJECT_HANDLE id,
				                      CK_ATTRIBUTE_TYPE type )
{
    CK_BBOOL ckvalue = FALSE;
    CK_ATTRIBUTE template;
    CK_RV crv;

    /* Prepare to retrieve the attribute. */
    PK11_SETATTRS( &template, type, &ckvalue, sizeof( CK_BBOOL ) );

    /* Retrieve attribute value. */
    crv = PK11_GETTAB( slot )->C_GetAttributeValue( slot->session, id,
                                                    &template, 1 );
    if( crv != CKR_OK ) {
        PORT_SetError( PK11_MapError( crv ) );
        return FALSE;
    }

    return ckvalue;
}

/*
 * PKCS #11 pairwise consistency check utilized to validate key pair.
 */
static SECStatus
pk11_PairwiseConsistencyCheck(SECKEYPublicKey *pubKey, 
	SECKEYPrivateKey *privKey, CK_MECHANISM *mech, void* wincx )
{
    /* Variables used for Encrypt/Decrypt functions. */
    unsigned char *known_message = (unsigned char *)"Known Crypto Message";
    CK_BBOOL isEncryptable = FALSE;
    unsigned char plaintext[PAIRWISE_MESSAGE_LENGTH];
    CK_ULONG bytes_decrypted;
    PK11SlotInfo *slot;
    CK_OBJECT_HANDLE id;
    unsigned char *ciphertext;
    unsigned char *text_compared;
    CK_ULONG max_bytes_encrypted;
    CK_ULONG bytes_encrypted;
    CK_ULONG bytes_compared;
    CK_RV crv;

    /* Variables used for Signature/Verification functions. */
    unsigned char *known_digest = (unsigned char *)"Mozilla Rules World!";
    SECItem  signature;
    SECItem  digest;    /* always uses SHA-1 digest */
    int signature_length;
    SECStatus rv;

    /**************************************************/
    /* Pairwise Consistency Check of Encrypt/Decrypt. */
    /**************************************************/

    isEncryptable = PK11_HasAttributeSet( privKey->pkcs11Slot, 
					privKey->pkcs11ID, CKA_DECRYPT );

    /* If the encryption attribute is set; attempt to encrypt */
    /* with the public key and decrypt with the private key.  */
    if( isEncryptable ) {
	/* Find a module to encrypt against */
	slot = PK11_GetBestSlot(pk11_mapWrapKeyType(privKey->keyType),wincx);
	if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return SECFailure;
	}

	id = PK11_ImportPublicKey(slot,pubKey);
	if (id == CK_INVALID_KEY) {
	    PK11_FreeSlot(slot);
	    return SECFailure;
	}
	/* Prepare for encryption using the public key. */
	crv = PK11_GETTAB( slot )->C_EncryptInit( slot->session,
						  mech, id );
        if( crv != CKR_OK ) {
	    PORT_SetError( PK11_MapError( crv ) );
	    PK11_FreeSlot(slot);
	    return SECFailure;
	}

	/* Compute max bytes encrypted from modulus length of private key. */
	max_bytes_encrypted = PK11_GetPrivateModulusLen( privKey );

	/* Allocate space for ciphertext. */
	ciphertext = (unsigned char *) PORT_Alloc( max_bytes_encrypted );
	if( ciphertext == NULL ) {
	    PORT_SetError( SEC_ERROR_NO_MEMORY );
	    PK11_FreeSlot(slot);
	    return SECFailure;
	}

	/* Initialize bytes encrypted to max bytes encrypted. */
	bytes_encrypted = max_bytes_encrypted;

	/* Encrypt using the public key. */
	crv = PK11_GETTAB( slot )->C_Encrypt( slot->session,
					      known_message,
					      PAIRWISE_MESSAGE_LENGTH,
					      ciphertext,
					      &bytes_encrypted );
	PK11_FreeSlot(slot);
	if( crv != CKR_OK ) {
	    PORT_SetError( PK11_MapError( crv ) );
	    PORT_Free( ciphertext );
	    return SECFailure;
	}

	/* Always use the smaller of these two values . . . */
	bytes_compared = ( bytes_encrypted > PAIRWISE_MESSAGE_LENGTH )
			 ? PAIRWISE_MESSAGE_LENGTH
			 : bytes_encrypted;

	/* If there was a failure, the plaintext */
	/* goes at the end, therefore . . .      */
	text_compared = ( bytes_encrypted > PAIRWISE_MESSAGE_LENGTH )
			? (ciphertext + bytes_encrypted -
			  PAIRWISE_MESSAGE_LENGTH )
			: ciphertext;

	/* Check to ensure that ciphertext does */
	/* NOT EQUAL known input message text   */
	/* per FIPS PUB 140-1 directive.        */
	if( ( bytes_encrypted != max_bytes_encrypted ) ||
	    ( PORT_Memcmp( text_compared, known_message,
			   bytes_compared ) == 0 ) ) {
	    /* Set error to Invalid PRIVATE Key. */
	    PORT_SetError( SEC_ERROR_INVALID_KEY );
	    PORT_Free( ciphertext );
	    return SECFailure;
	}

	slot = privKey->pkcs11Slot;
	/* Prepare for decryption using the private key. */
	crv = PK11_GETTAB( slot )->C_DecryptInit( slot->session,
						  mech,
						  privKey->pkcs11ID );
	if( crv != CKR_OK ) {
	    PORT_SetError( PK11_MapError(crv) );
	    PORT_Free( ciphertext );
	    PK11_FreeSlot(slot);
	    return SECFailure;
	}

	/* Initialize bytes decrypted to be the */
	/* expected PAIRWISE_MESSAGE_LENGTH.    */
	bytes_decrypted = PAIRWISE_MESSAGE_LENGTH;

	/* Decrypt using the private key.   */
	/* NOTE:  No need to reset the      */
	/*        value of bytes_encrypted. */
	crv = PK11_GETTAB( slot )->C_Decrypt( slot->session,
					      ciphertext,
					      bytes_encrypted,
					      plaintext,
					      &bytes_decrypted );

	/* Finished with ciphertext; free it. */
	PORT_Free( ciphertext );

	if( crv != CKR_OK ) {
	   PORT_SetError( PK11_MapError(crv) );
	    PK11_FreeSlot(slot);
	   return SECFailure;
	}

	/* Check to ensure that the output plaintext */
	/* does EQUAL known input message text.      */
	if( ( bytes_decrypted != PAIRWISE_MESSAGE_LENGTH ) ||
	    ( PORT_Memcmp( plaintext, known_message,
			   PAIRWISE_MESSAGE_LENGTH ) != 0 ) ) {
	    /* Set error to Bad PUBLIC Key. */
	    PORT_SetError( SEC_ERROR_BAD_KEY );
	    PK11_FreeSlot(slot);
	    return SECFailure;
	}
    }

    /**********************************************/
    /* Pairwise Consistency Check of Sign/Verify. */
    /**********************************************/

    /* Initialize signature and digest data. */
    signature.data = NULL;
    digest.data = NULL;

    /* Determine length of signature. */
    signature_length = PK11_SignatureLen( privKey );
    if( signature_length == 0 )
	goto failure;

    /* Allocate space for signature data. */
    signature.data = (unsigned char *) PORT_Alloc( signature_length );
    if( signature.data == NULL ) {
	PORT_SetError( SEC_ERROR_NO_MEMORY );
	goto failure;
    }

    /* Allocate space for known digest data. */
    digest.data = (unsigned char *) PORT_Alloc( PAIRWISE_DIGEST_LENGTH );
    if( digest.data == NULL ) {
	PORT_SetError( SEC_ERROR_NO_MEMORY );
	goto failure;
    }

    /* "Fill" signature type and length. */
    signature.type = PAIRWISE_SECITEM_TYPE;
    signature.len  = signature_length;

    /* "Fill" digest with known SHA-1 digest parameters. */
    digest.type = PAIRWISE_SECITEM_TYPE;
    PORT_Memcpy( digest.data, known_digest, PAIRWISE_DIGEST_LENGTH );
    digest.len = PAIRWISE_DIGEST_LENGTH;

    /* Sign the known hash using the private key. */
    rv = PK11_Sign( privKey, &signature, &digest );
    if( rv != SECSuccess )
	goto failure;

    /* Verify the known hash using the public key. */
    rv = PK11_Verify( pubKey, &signature, &digest, wincx );
    if( rv != SECSuccess )
	goto failure;

    /* Free signature and digest data. */
    PORT_Free( signature.data );
    PORT_Free( digest.data );

    return SECSuccess;

failure:
    if( signature.data != NULL )
	PORT_Free( signature.data );
    if( digest.data != NULL )
	PORT_Free( digest.data );

    return SECFailure;
}



/*
 * take a private key in one pkcs11 module and load it into another:
 *  NOTE: the source private key is a rare animal... it can't be sensitive.
 *  This is used to do a key gen using one pkcs11 module and storing the
 *  result into another.
 */
SECKEYPrivateKey *
pk11_loadPrivKey(PK11SlotInfo *slot,SECKEYPrivateKey *privKey, 
		SECKEYPublicKey *pubKey, PRBool token, PRBool sensitive) 
{
    CK_ATTRIBUTE privTemplate[] = {
        /* class must be first */
	{ CKA_CLASS, NULL, 0 },
	{ CKA_KEY_TYPE, NULL, 0 },
	/* these three must be next */
	{ CKA_TOKEN, NULL, 0 },
	{ CKA_PRIVATE, NULL, 0 },
	{ CKA_SENSITIVE, NULL, 0 },
	{ CKA_ID, NULL, 0 },
#ifdef notdef
	{ CKA_LABEL, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 },
#endif
	/* RSA */
	{ CKA_MODULUS, NULL, 0 },
	{ CKA_PRIVATE_EXPONENT, NULL, 0 },
	{ CKA_PUBLIC_EXPONENT, NULL, 0 },
	{ CKA_PRIME_1, NULL, 0 },
	{ CKA_PRIME_2, NULL, 0 },
	{ CKA_EXPONENT_1, NULL, 0 },
	{ CKA_EXPONENT_2, NULL, 0 },
	{ CKA_COEFFICIENT, NULL, 0 },
    };
    CK_ATTRIBUTE *attrs = NULL, *ap;
    int templateSize = sizeof(privTemplate)/sizeof(privTemplate[0]);
    PRArenaPool *arena;
    CK_OBJECT_HANDLE objectID;
    int i, count = 0;
    int extra_count = 0;
    CK_RV crv;
    SECStatus rv;

    for (i=0; i < templateSize; i++) {
	if (privTemplate[i].type == CKA_MODULUS) {
	    attrs= &privTemplate[i];
	    count = i;
	    break;
	}
    }
    PORT_Assert(attrs != NULL);
    if (attrs == NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
    }

    switch (privKey->keyType) {
    case rsaKey:
	count = templateSize;
	extra_count = templateSize - (attrs - privTemplate);
	break;
    case dsaKey:
	ap->type = CKA_PRIME; ap++; count++; extra_count++;
	ap->type = CKA_SUBPRIME; ap++; count++; extra_count++;
	ap->type = CKA_BASE; ap++; count++; extra_count++;
	ap->type = CKA_VALUE; ap++; count++; extra_count++;
	break;
    case dhKey:
	ap->type = CKA_PRIME; ap++; count++; extra_count++;
	ap->type = CKA_BASE; ap++; count++; extra_count++;
	ap->type = CKA_VALUE; ap++; count++; extra_count++;
	break;
     default:
	count = 0;
	extra_count = 0;
	break;
     }

     if (count == 0) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
     }

     arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
     if (arena == NULL) return NULL;
     /*
      * read out the old attributes.
      */
     crv = PK11_GetAttributes(arena, privKey->pkcs11Slot, privKey->pkcs11ID,
		privTemplate,count);
     if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	PORT_FreeArena(arena, PR_TRUE);
	return NULL;
     }

     /* Reset sensitive, token, and private */
     *(CK_BBOOL *)(privTemplate[2].pValue) = token ? TRUE : FALSE;
     *(CK_BBOOL *)(privTemplate[3].pValue) = token ? TRUE : FALSE;
     *(CK_BBOOL *)(privTemplate[4].pValue) = sensitive ? TRUE : FALSE;

     /* Sigh, not everyone can handle zero padded key values, give
      * them the raw data as unsigned */
     for (ap=attrs; extra_count; ap++, extra_count--) {
	pk11_SignedToUnsigned(ap);
     }

     /* now Store the puppies */
     rv = PK11_CreateNewObject(slot,privTemplate, count, &objectID);
     PORT_FreeArena(arena, PR_TRUE);
     if (rv != SECSuccess) {
	return NULL;
     }

     /* try loading the public key as a token object */
     pk11_ImportTokenPublicKey(slot, pubKey, PR_TRUE);
     if (pubKey->pkcs11Slot) {
	PK11_FreeSlot(pubKey->pkcs11Slot);
	pubKey->pkcs11Slot = NULL;
	pubKey->pkcs11ID = CK_INVALID_KEY;
     }

     /* build new key structure */
     return PK11_MakePrivKey(slot, privKey->keyType, !token, 
						objectID, privKey->wincx);
}


/*
 * Use the token to Generate a key. keySize must be 'zero' for fixed key
 * length algorithms. NOTE: this means we can never generate a DES2 key
 * from this interface!
 */
SECKEYPrivateKey *
PK11_GenerateKeyPair(PK11SlotInfo *slot,CK_MECHANISM_TYPE type, 
   void *param, SECKEYPublicKey **pubKey, PRBool token, 
					PRBool sensitive, void *wincx)
{
    /* we have to use these native types because when we call PKCS 11 modules
     * we have to make sure that we are using the correct sizes for all the
     * parameters. */
    CK_BBOOL ckfalse = FALSE; /* Sigh */
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_ULONG modulusBits;
    CK_BYTE publicExponent[4];
    CK_ATTRIBUTE privTemplate[] = {
	{ CKA_SENSITIVE, NULL, 0},
	{ CKA_TOKEN,  NULL, 0},
	{ CKA_PRIVATE,  NULL, 0},
    };
    CK_ATTRIBUTE rsaPubTemplate[] = {
	{ CKA_MODULUS_BITS, NULL, 0},
	{ CKA_PUBLIC_EXPONENT, NULL, 0},
    };
    CK_ATTRIBUTE dsaPubTemplate[] = {
	{ CKA_PRIME, NULL, 0 },
	{ CKA_SUBPRIME, NULL, 0 },
	{ CKA_BASE, NULL, 0 },
    };
    int dsaPubCount = sizeof(dsaPubTemplate)/sizeof(dsaPubTemplate[0]);
    /*CK_ULONG key_size = 0;*/
    CK_ATTRIBUTE *pubTemplate;
    int privCount = sizeof(privTemplate)/sizeof(privTemplate[0]);
    int rsaPubCount = sizeof(rsaPubTemplate)/sizeof(rsaPubTemplate[0]);
    int pubCount = 0;
    PK11RSAGenParams *rsaParams;
    PQGParams *dsaParams;
    CK_MECHANISM mechanism;
    CK_MECHANISM test_mech;
    CK_SESSION_HANDLE session_handle;
    CK_RV crv;
    CK_OBJECT_HANDLE privID,pubID;
    SECKEYPrivateKey *privKey;
    KeyType keyType;
    PRBool restore;
    int peCount,i;
    CK_ATTRIBUTE *attrs;
    SECItem *pubKeyIndex;
    CK_ATTRIBUTE setTemplate;
    SECStatus rv;

    PORT_Assert(slot != NULL);
    if (slot == NULL) {
	PORT_SetError( SEC_ERROR_NO_MODULE);
	return NULL;
    }

    /* if our slot really doesn't do this mechanism, Generate the key
     * in our internal token and write it out */
    if (!PK11_DoesMechanism(slot,type)) {
	PK11SlotInfo *int_slot = PK11_GetBestSlot(type,wincx);
	if (slot == int_slot) {
	    PK11_FreeSlot(int_slot);
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return NULL;
	}
	if (int_slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}
	privKey = PK11_GenerateKeyPair(int_slot,type, param, pubKey, PR_FALSE, 
							PR_FALSE, wincx);
	PK11_FreeSlot(int_slot);
	if (privKey != NULL) {
	    SECKEYPrivateKey *newPrivKey = pk11_loadPrivKey(slot,privKey,
						*pubKey,token,sensitive);
	    SECKEY_DestroyPrivateKey(privKey);
	    if (newPrivKey == NULL) {
		SECKEY_DestroyPublicKey(*pubKey);
		*pubKey = NULL;
	    }
	    return newPrivKey;
	}
	return NULL;
   }


    mechanism.mechanism = type;
    mechanism.pParameter = NULL;
    mechanism.ulParameterLen = 0;
    test_mech.pParameter = NULL;
    test_mech.ulParameterLen = 0;
    attrs = privTemplate;
    PK11_SETATTRS(attrs, CKA_SENSITIVE, sensitive ? &cktrue : &ckfalse, 
						sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_TOKEN, token ? &cktrue : &ckfalse,
						 sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_PRIVATE, token ? &cktrue : &ckfalse,
							 sizeof(CK_BBOOL));

    switch (type) {
    case CKM_RSA_PKCS_KEY_PAIR_GEN:
	rsaParams = (PK11RSAGenParams *)param;
	modulusBits = rsaParams->keySizeInBits;
	peCount = 0;
	for (i=0; i < 4; i++) {
	    if (peCount || (rsaParams->pe & 
				((unsigned long)0xff000000L >> (i*8)))) {
		publicExponent[peCount] = 
				(CK_BYTE)((rsaParams->pe >> (3-i)*8) & 0xff);
		peCount++;
	    }
	}
	PORT_Assert(peCount != 0);
	attrs = rsaPubTemplate;
	PK11_SETATTRS(attrs, CKA_MODULUS_BITS, 
				&modulusBits, sizeof(modulusBits)); attrs++;
	PK11_SETATTRS(attrs, CKA_PUBLIC_EXPONENT, publicExponent, peCount);
	pubTemplate = rsaPubTemplate;
	pubCount = rsaPubCount;
	keyType = rsaKey;
	test_mech.mechanism = CKM_RSA_PKCS;
	break;
    case CKM_DSA_KEY_PAIR_GEN:
	dsaParams = (PQGParams *)param;
	attrs = dsaPubTemplate;
	PK11_SETATTRS(attrs, CKA_PRIME, dsaParams->prime.data,
				dsaParams->prime.len); attrs++;
	PK11_SETATTRS(attrs, CKA_SUBPRIME, dsaParams->subPrime.data,
					dsaParams->subPrime.len); attrs++;
	PK11_SETATTRS(attrs, CKA_BASE, dsaParams->base.data,
						dsaParams->base.len); 
	pubTemplate = dsaPubTemplate;
	pubCount = dsaPubCount;
	keyType = dsaKey;
	test_mech.mechanism = CKM_DSA;
	break;
    default:
	PORT_SetError( SEC_ERROR_BAD_KEY );
	return NULL;
    }

    if (token) {
	session_handle = PK11_GetRWSession(slot);
	restore = PR_TRUE;
    } else {
	session_handle = slot->session;
	restore = PR_FALSE;
    }

    crv = PK11_GETTAB(slot)->C_GenerateKeyPair(session_handle, &mechanism,
	pubTemplate,pubCount,privTemplate,privCount,&privID,&pubID);


    if (crv != CKR_OK) {
	if (restore)  PK11_RestoreROSession(slot,session_handle);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    *pubKey = PK11_ExtractPublicKey(slot, keyType, pubID);
    if (*pubKey == NULL) {
	if (restore)  PK11_RestoreROSession(slot,session_handle);
	PK11_DestroyObject(slot,pubID);
	PK11_DestroyObject(slot,privID);
	return NULL;
    }

    /* set the ID to the public key so we can find it again */
    pubKeyIndex =  NULL;
    switch (type) {
    case CKM_RSA_PKCS_KEY_PAIR_GEN:
	pubKeyIndex = &(*pubKey)->u.rsa.modulus;
	break;
    case CKM_DSA_KEY_PAIR_GEN:
	pubKeyIndex = &(*pubKey)->u.dsa.publicValue;
	break;
    }
    PORT_Assert(pubKeyIndex != NULL);

    PK11_SETATTRS(&setTemplate, CKA_ID, pubKeyIndex->data, pubKeyIndex->len);
    crv = PK11_GETTAB(slot)->C_SetAttributeValue(session_handle, privID,
		&setTemplate, 1);
    if (crv == CKR_OK && PK11_HasAttributeSet( slot, pubID, CKA_TOKEN)) {
    	crv = PK11_GETTAB(slot)->C_SetAttributeValue(session_handle, pubID,
		&setTemplate, 1);
    }

    if (restore) {
	PK11_RestoreROSession(slot,session_handle);
    }

    if (crv != CKR_OK) {
	PK11_DestroyObject(slot,pubID);
	PK11_DestroyObject(slot,privID);
	PORT_SetError( PK11_MapError(crv) );
	*pubKey = NULL;
	return NULL;
    }

    privKey = PK11_MakePrivKey(slot,keyType,!token,privID,wincx);
    if (privKey == NULL) {
	SECKEY_DestroyPublicKey(*pubKey);
	PK11_DestroyObject(slot,privID);
	*pubKey = NULL;
	return NULL;  /* due to pairwise consistency check */
    }

    /*
     * despite our request not to create a token version of the
     * public key, one was created anyway (sigh)... free it.
     */
    if (PK11_HasAttributeSet( slot, pubID, CKA_TOKEN)) {
	PK11_FreeSlot((*pubKey)->pkcs11Slot);
	(*pubKey)->pkcs11Slot = NULL;
	(*pubKey)->pkcs11ID = CK_INVALID_KEY;
    }

    /* Perform PKCS #11 pairwise consistency check. */
    rv = pk11_PairwiseConsistencyCheck( *pubKey, privKey, &test_mech, wincx );
    if( rv != SECSuccess ) {
	SECKEY_DestroyPublicKey( *pubKey );
	SECKEY_DestroyPrivateKey( privKey );
	*pubKey = NULL;
	privKey = NULL;
	return NULL;
    }

    return privKey;
}

/*
 * This function does a straight public key wrap (which only RSA can do).
 * Use PK11_PubGenKey and PK11_WrapSymKey to implement the FORTEZZA and
 * Diffie-Hellman Ciphers. */
SECStatus
PK11_PubWrapSymKey(CK_MECHANISM_TYPE type, SECKEYPublicKey *pubKey,
				PK11SymKey *symKey, SECItem *wrappedKey)
{
    PK11SlotInfo *slot;
    CK_ULONG len =  wrappedKey->len;
    PK11SymKey *newKey = NULL;
    CK_OBJECT_HANDLE id;
    CK_MECHANISM mechanism;
    CK_RV crv;

    /* if this slot doesn't support the mechanism, go to a slot that does */
    newKey = pk11_ForceSlot(symKey,type,CKA_ENCRYPT);
    if (newKey != NULL) {
	symKey = newKey;
    }

    if ((symKey == NULL) || (symKey->slot == NULL)) {
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return SECFailure;
    }

    slot = symKey->slot;
    mechanism.mechanism = pk11_mapWrapKeyType(pubKey->keyType);
    mechanism.pParameter = NULL;
    mechanism.ulParameterLen = 0;

    id = PK11_ImportPublicKey(slot,pubKey);

    crv = PK11_GETTAB(slot)->C_WrapKey(slot->session,&mechanism,
		id,symKey->objectID,wrappedKey->data,&len);
    if (newKey) {
	PK11_FreeSymKey(newKey);
    }

    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    wrappedKey->len = len;
    return SECSuccess;
} 

/*
 * this little function uses the Encrypt function to wrap a key, just in
 * case someone neglected to implement wrap!!
 */
static SECStatus
pk11_HandWrap(PK11SymKey *wrappingKey, SECItem *param, CK_MECHANISM_TYPE type,
			 SECItem *inKey, SECItem *outKey)
{
    PK11SlotInfo *slot;
    CK_ULONG len;
    SECItem *data;
    CK_MECHANISM mech;
    CK_RV crv;

    slot = wrappingKey->slot;
    /* use NULL IV's for wrapping */
    mech.mechanism = type;
    if (param) {
	mech.pParameter = param->data;
	mech.ulParameterLen = param->len;
    } else {
	mech.pParameter = NULL;
	mech.ulParameterLen = 0;
    }
    crv = PK11_GETTAB(slot)->C_EncryptInit(slot->session,&mech,
							wrappingKey->objectID);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }

    /* keys are almost always aligned, but if we get this far,
     * we've gone above and beyond anyway... */
    data = PK11_BlockData(inKey,PK11_GetBlockSize(type,param));
    if (data == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    len = outKey->len;
    crv = PK11_GETTAB(slot)->C_Encrypt(slot->session,data->data,data->len,
							   outKey->data, &len);
    SECITEM_FreeItem(data,PR_TRUE);
    outKey->len = len;
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

/*
 * This function does a TEK based wrap. Weird things can happen if
 * Some time we should probably look into changing it to do handle FORTEZZA
 * and Diffie-Hellman, which would require a parameter pointer and a pointer
 * to the encrypter's private key... of course we're in deep problems if the
 * encrypter's private key is in a different slot from the symKey (sigh).
 */
SECStatus
PK11_WrapSymKey(CK_MECHANISM_TYPE type, PK11SymKey *wrappingKey,
				PK11SymKey *symKey, SECItem *wrappedKey)
{
    PK11SlotInfo *slot;
    CK_ULONG len;
    PK11SymKey *newKey = NULL;
    SECItem *param;
    CK_MECHANISM mechanism;
    CK_RV crv;
    SECStatus rv;

    /* if this slot doesn't support the mechanism, go to a slot that does */
    /* Force symKey and wrappingKey into the same slot */
    if ((wrappingKey->slot == NULL) || (symKey->slot != wrappingKey->slot)) {
	/* first try copying the wrapping Key to the symKey slot */
	if (symKey->slot && PK11_DoesMechanism(symKey->slot,type)) {
	    newKey = pk11_CopyToSlot(symKey->slot,type,CKA_WRAP,wrappingKey);
	}
	/* Nope, try it the other way */
	if (newKey == NULL) {
	    if (wrappingKey->slot) {
	        newKey = pk11_CopyToSlot(wrappingKey->slot,
					symKey->type, CKA_ENCRYPT, symKey);
	    }
	    /* just not playing... one last thing, can we get symKey's data?
	     * If it's possible, we it should already be in the 
	     * symKey->data.data pointer because pk11_CopyToSlot would have
	     * tried to put it there. */
	    if (newKey == NULL) {
		/* Can't get symKey's data: Game Over */
		if (symKey->data.data == NULL) {
		    PORT_SetError( SEC_ERROR_NO_MODULE );
		    return SECFailure;
		}
		param = PK11_ParamFromIV(type,NULL);
		rv = pk11_HandWrap(wrappingKey, param, type,
						&symKey->data,wrappedKey);
		if (param) SECITEM_FreeItem(param,PR_TRUE);
		return rv;
	    }
	    /* we successfully moved the sym Key */
	    symKey = newKey;
	} else {
	    /* we successfully moved the wrapping Key */
	    wrappingKey = newKey;
	}
    }

    /* at this point both keys are in the same token */
    slot = wrappingKey->slot;
    mechanism.mechanism = type;
    /* use NULL IV's for wrapping */
    param = PK11_ParamFromIV(type,NULL);
    if (param) {
	mechanism.pParameter = param->data;
	mechanism.ulParameterLen = param->len;
    } else {
	mechanism.pParameter = NULL;
	mechanism.ulParameterLen = 0;
    }

    crv = PK11_GETTAB(slot)->C_WrapKey(slot->session, &mechanism,
		 wrappingKey->objectID, symKey->objectID, 
						wrappedKey->data, &len);
    wrappedKey->len = len;
    rv = SECSuccess;
    if (crv != CKR_OK) {
	/* can't wrap it? try hand wrapping it... */
	do {
	    if (symKey->data.data == NULL) {
		rv = PK11_ExtractKeyValue(symKey);
		if (rv != SECSuccess) break;
	    }
	    rv = pk11_HandWrap(wrappingKey, param, type, &symKey->data,
								 wrappedKey);
	} while (PR_FALSE);
    }
    if (newKey) PK11_FreeSymKey(newKey);
    if (param) SECITEM_FreeItem(param,PR_TRUE);
    return rv;
} 


/*
 * This Generates a wrapping key based on a privateKey, publicKey, and two
 * random numbers. For Mail usage RandomB should be NULL. In the Sender's
 * case RandomA is generate, outherwize it is passed.
 */
PK11SymKey *
PK11_DeriveSymKey( SECKEYPrivateKey *privKey, SECKEYPublicKey *pubKey, 
   PRBool isSender, SECItem *randomA, SECItem *randomB,
    CK_MECHANISM_TYPE derive, CK_MECHANISM_TYPE target,
						int keySize,void *wincx)
{
    PK11SlotInfo *slot = privKey->pkcs11Slot;
    /*CK_MECHANISM mechanism; */
    PK11SymKey *symKey;

    if (privKey->keyType != pubKey->keyType) {
	PORT_SetError(SEC_ERROR_BAD_KEY);
	return NULL;
    }

    /* get our key Structure */
    symKey = PK11_CreateSymKey(slot,target,wincx);
    if (symKey == NULL) {
	return NULL;
    }

    switch (privKey->keyType) {
    case rsaKey:
    case dsaKey:
    case nullKey:
    case fortezzaKey:
    case dhKey:
	PORT_SetError(SEC_ERROR_BAD_KEY);
	break;
#ifdef notdef
    case fortezzaKey:
	{
	    CK_FORTEZZA_EXCHANGE_PARAM param;
	    param.isSender = (CK_BOOL) isSender;
	    if (isSender) {
	    	param.usRandomLen = FORTEZZA_RANDOM_LENGTH;
	    	param.pRandomA = PORT_Alloc(FORTEZZA_RANDOM_LENGTH);
	    } else {
		param.usRandomA = randomA.data;
		param.usRandomLen = randomA.len;
	    }
	    if (param.pRandomA == NULL) {
		PORT_SetError( SEC_ERROR_NO_MEMORY ):
		break;
	    }
	    param.pRandomB = NULL;
	    if (RandomB)
		 param.pRandomB = RandomB.data;
	    param.usPublicDataLen = pubKey->u.fortezza.KEAKey.len
	    param.PPublicData = pubKey->u.fortezza.KEAKey.data;

	    mechanism.mechanism = derive;
	    mechanism.pParameter = &param;
	    mechanism.usParameterSize = sizeof(param);

	    /* get a new symKey structure */
	    crv=PK11_GETTAB(slot)->C_DeriveKey(slot->session, &mechanism, NULL,
						0, &symKey->objectID);
	    if (crv == CKR_OK) return symKey;
	    PORT_SetError( PK11_MapError(crv) );
	}
	break;
    case dhKey:
	{
	    CK_BBOOL ckfalse = FALSE; /* Sigh */
	    CK_BBOOL cktrue = TRUE; /* sigh */
	    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
	    CK_BBOOL cktrue = TRUE; /* sigh */
	    CK_ULONG key_size = 0;
	    CK_ATTRIBUTE keyTemplate[] = {
		{ CKA_CLASS, &keyClass, sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, sizeof(keyType) },
		{ CKA_SENSITIVE, &ckfalse, sizeof(CK_BBOOL) },
		{ CKA_WRAP, &cktrue, 1},
		{ CKA_UNWRAP, &cktrue, 1},
		{ CKA_VALUE_LEN, &key_size, sizeof(key_size) },
	    };
	    int templateCount = sizeof(keyTemplate)/sizeof(keyTemplate[0]);

	    keyType = PK11_GetKeyType(target,keySize);
	    key_size = keySize;
	    symKey->keySize = keySize;
	    if (key_size == 0) templateCount--;

	    mechanism.mechanism = derive;
	    mechanism.pParameter = pubKey->u.dh.key.data; 
	    mechanism.usParameterSize = pubKey->u.dh.key.len;
	    crv = PK11_GETTAB(slot)->C_DeriveKey(slot->session, &mechanism,
				 keyTempate, templateCount, &symKey->objectID);
	    if (crv == CKR_OK) return symKey;
	    PORT_SetError( PK11_MapError(crv) );
	}
	break;
#endif
   }

   PK11_FreeSymKey(symKey);
   return NULL;
}

/*
 * this little function uses the Decrypt function to unwrap a key, just in
 * case someone neglected to implement unwrap!! NOTE: The key size may
 * not be preserved properly for some algorithms!
 */
static PK11SymKey *
pk11_HandUnwrap(PK11SlotInfo *slot, CK_OBJECT_HANDLE wrappingKey,
 		CK_MECHANISM *mech, SECItem *inKey, CK_MECHANISM_TYPE target,
						int key_size, void * wincx)
{
    CK_ULONG len;
    SECItem outKey;
    PK11SymKey *symKey;
    CK_RV crv;

    /* use NULL IV's for wrapping */
    crv = PK11_GETTAB(slot)->C_DecryptInit(slot->session,mech,wrappingKey);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    /* keys are almost always aligned, but if we get this far,
     * we've gone above and beyond anyway... */
    outKey.data = PORT_Alloc(inKey->len);
    if (outKey.data == NULL) {
	PORT_SetError( SEC_ERROR_NO_MEMORY );
	return NULL;
    }
    len = inKey->len;
    crv = PK11_GETTAB(slot)->C_Decrypt(slot->session,inKey->data,inKey->len,
							   outKey.data, &len);
    outKey.len = (key_size == 0) ? len : key_size;
    if (crv != CKR_OK) {
	PORT_Free(outKey.data);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    if (PK11_DoesMechanism(slot,target)) {
    	symKey = PK11_ImportSymKey(slot, target, CKA_DECRYPT, &outKey, wincx);
    } else {
	slot = PK11_GetBestSlot(target,wincx);
	if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    PORT_Free(outKey.data);
	    return NULL;
	}
	symKey = PK11_ImportSymKey(slot, target, CKA_DECRYPT, &outKey, wincx);
	PK11_FreeSlot(slot);
    }
    PORT_Free(outKey.data);
    return symKey;
}

/*
 * The wrap/unwrap function is pretty much the same for private and
 * public keys. It's just getting the Object ID and slot right. This is
 * the combined unwrap function.
 */
static PK11SymKey *
pk11_AnyUnwrapKey(PK11SlotInfo *slot, CK_OBJECT_HANDLE wrappingKey,
     CK_MECHANISM_TYPE wrapType, SECItem *wrappedKey,
			CK_MECHANISM_TYPE target, int keySize, void *wincx)
{
    CK_BBOOL ckfalse = FALSE; /* Sigh */
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_ULONG key_size = 0;
    CK_ATTRIBUTE keyTemplate[] = {
	{ CKA_CLASS, NULL, 0},
	{ CKA_KEY_TYPE, NULL, 0},
	{ CKA_SENSITIVE, NULL, 0},
	{ CKA_DECRYPT, NULL, 0},
	{ CKA_VALUE_LEN, NULL, 0},
    };
    int templateCount = sizeof(keyTemplate)/sizeof(keyTemplate[0]);
    PK11SymKey *symKey;
    CK_MECHANISM mechanism;
    SECItem *param;
    CK_RV crv;
    CK_ATTRIBUTE *attrs = keyTemplate;

    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, sizeof(keyClass)); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, sizeof(keyType)); attrs++;
    PK11_SETATTRS(attrs, CKA_SENSITIVE, &ckfalse, sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_DECRYPT, &cktrue, 1); attrs++;
    PK11_SETATTRS(attrs, CKA_VALUE_LEN, &key_size, sizeof(key_size)); attrs++;


    /* get our key Structure */
    symKey = PK11_CreateSymKey(slot,target,wincx);
    if (symKey == NULL) {
	return NULL;
    }

    keyType = PK11_GetKeyType(target,keySize);
    key_size = keySize;
    symKey->size = keySize;
    if (key_size == 0) templateCount--;

    mechanism.mechanism = wrapType;
    /* use NULL IV's for wrapping */
    param = PK11_ParamFromIV(wrapType,NULL);
    if (param) {
	mechanism.pParameter = param->data;
	mechanism.ulParameterLen = param->len;
    } else {
	mechanism.pParameter = NULL;
	mechanism.ulParameterLen = 0;
    }
    crv = PK11_GETTAB(slot)->C_UnwrapKey(slot->session,&mechanism,wrappingKey,
		wrappedKey->data, wrappedKey->len, keyTemplate, templateCount, 
							  &symKey->objectID);
    SECITEM_FreeItem(param,PR_TRUE);
    if (crv != CKR_OK) {
	/* try hand Unwrapping */
	PK11_FreeSymKey(symKey);
	symKey = pk11_HandUnwrap(slot, wrappingKey, &mechanism, wrappedKey,
						target, keySize, wincx);
   }

   return symKey;
}

/* use a symetric key to unwrap another symetric key */
PK11SymKey *
PK11_UnwrapSymKey( PK11SymKey *wrappingKey, CK_MECHANISM_TYPE wrapType,
		SECItem *wrappedKey, CK_MECHANISM_TYPE target, int keySize)
{
    return pk11_AnyUnwrapKey(wrappingKey->slot, wrappingKey->objectID,
		wrapType,  wrappedKey, target, keySize, wrappingKey->cx);
}

/* unwrap a key symetric key with a private key. */
PK11SymKey *
PK11_PubUnwrapSymKey(SECKEYPrivateKey *wrappingKey, SECItem *wrappedKey,
	 			  CK_MECHANISM_TYPE target, int keySize)
{
    CK_MECHANISM_TYPE wrapType = pk11_mapWrapKeyType(wrappingKey->keyType);
    PK11_HandlePasswordCheck(wrappingKey->pkcs11Slot,wrappingKey->wincx);
    
    return pk11_AnyUnwrapKey(wrappingKey->pkcs11Slot, wrappingKey->pkcs11ID,
	wrapType, wrappedKey, target, keySize, wrappingKey->wincx);

}

/*
 * Recover the Signed data. We need this because !#$#$#!# Verify can't
 * figure out which hash algorithm to use until we decrypt this!. (sigh)
 */
SECStatus
PK11_VerifyRecover(SECKEYPublicKey *key,
			 	SECItem *sig, SECItem *dsig, void *wincx)
{
    PK11SlotInfo *slot = key->pkcs11Slot;
    CK_OBJECT_HANDLE id = key->pkcs11ID;
    CK_MECHANISM mech = {0, NULL, 0 };
    CK_ULONG len;
    CK_RV crv;

    mech.mechanism = pk11_mapSignKeyType(key->keyType);

    if (slot == NULL) {
	slot = PK11_GetBestSlot(mech.mechanism,wincx);
	if (slot == NULL) {
	    	PORT_SetError( SEC_ERROR_NO_MODULE );
		return SECFailure;
	}
	id = PK11_ImportPublicKey(slot,key);
    }

    crv = PK11_GETTAB(slot)->C_VerifyRecoverInit(slot->session,&mech,id);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    len = dsig->len;
    crv = PK11_GETTAB(slot)->C_VerifyRecover(slot->session,sig->data,
						sig->len, dsig->data, &len);
    dsig->len = len;
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

/*
 * verify a signature from its hash.
 */
SECStatus
PK11_Verify(SECKEYPublicKey *key, SECItem *sig, SECItem *hash, void *wincx)
{
    PK11SlotInfo *slot = key->pkcs11Slot;
    CK_OBJECT_HANDLE id = key->pkcs11ID;
    CK_MECHANISM mech = {0, NULL, 0 };
    CK_RV crv;

    mech.mechanism = pk11_mapSignKeyType(key->keyType);

    if (slot == NULL) {
	slot = PK11_GetBestSlot(mech.mechanism,wincx);
	if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return SECFailure;
	}
	id = PK11_ImportPublicKey(slot,key);
    }

    crv = PK11_GETTAB(slot)->C_VerifyInit(slot->session,&mech,id);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    crv = PK11_GETTAB(slot)->C_Verify(slot->session,hash->data,
					hash->len, sig->data, sig->len);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

/*
 * sign a hash. The algorithm is determined by the key.
 */
SECStatus
PK11_Sign(SECKEYPrivateKey *key, SECItem *sig, SECItem *hash)
{
    PK11SlotInfo *slot = key->pkcs11Slot;
    CK_MECHANISM mech = {0, NULL, 0 };
    CK_ULONG len;
    CK_RV crv;

    mech.mechanism = pk11_mapSignKeyType(key->keyType);

    PK11_HandlePasswordCheck(slot, key->wincx);
    crv = PK11_GETTAB(slot)->C_SignInit(slot->session,&mech,key->pkcs11ID);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    len = sig->len;
    crv = PK11_GETTAB(slot)->C_Sign(slot->session,hash->data,
					hash->len, sig->data, &len);
    sig->len = len;
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

/*
 * Now SSL 2.0 uses raw RSA stuff. These next to functions *must* use
 * RSA keys, or they'll fail. We do the checks up front. If anyone comes
 * up with a meaning for rawdecrypt for any other public key operation,
 * then we need to move this check into some of PK11_PubDecrypt callser,
 * (namely SSL 2.0).
 */
SECStatus
PK11_PubDecryptRaw(SECKEYPrivateKey *key, unsigned char *data, 
	unsigned *outLen, unsigned int maxLen, unsigned char *enc,
							 unsigned encLen)
{
    PK11SlotInfo *slot = key->pkcs11Slot;
    CK_MECHANISM mech = {CKM_RSA_X_509, NULL, 0 };
    CK_ULONG out = maxLen;
    CK_RV crv;

    if (key->keyType != rsaKey) {
	PORT_SetError( SEC_ERROR_INVALID_KEY );
	return SECFailure;
    }

    /* REALLY? do we want to do a PK11_handle check here? for simple
     * decryption? Well it's probably mute anyway since only servers
     * wind up using this function */
    PK11_HandlePasswordCheck(slot, key->wincx);
    crv = PK11_GETTAB(slot)->C_DecryptInit(slot->session,&mech,key->pkcs11ID);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    crv = PK11_GETTAB(slot)->C_Decrypt(slot->session,enc, encLen,
								data, &out);
    *outLen = out;
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

/* The encrypt version of the above function */
SECStatus
PK11_PubEncryptRaw(SECKEYPublicKey *key, unsigned char *enc,
		unsigned char *data, unsigned dataLen, void *wincx)
{
    PK11SlotInfo *slot;
    CK_MECHANISM mech = {CKM_RSA_X_509, NULL, 0 };
    CK_OBJECT_HANDLE id;
    CK_ULONG out = dataLen;
    CK_RV crv;

    if (key->keyType != rsaKey) {
	PORT_SetError( SEC_ERROR_BAD_KEY );
	return SECFailure;
    }

    slot = PK11_GetBestSlot(mech.mechanism, wincx);
    if (slot == NULL) {
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return SECFailure;
    }

    id = PK11_ImportPublicKey(slot,key);

    crv = PK11_GETTAB(slot)->C_EncryptInit(slot->session,&mech,id);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    crv = PK11_GETTAB(slot)->C_Encrypt(slot->session,data,dataLen,enc,&out);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

	
/**********************************************************************
 *
 *                   Now Deal with Crypto Contexts
 *
 **********************************************************************/

/*
 * Free up a Cipher Context
 */
void
PK11_DestroyContext(PK11Context *context, PRBool freeit)
{
    if (context->ownSession) {
        (void) PK11_GETTAB(context->slot)->C_CloseSession(context->session);
    }
    /* initialize the critical fields of the context */
    if (context->savedData != NULL ) PORT_Free(context->savedData);
    if (context->key) PK11_FreeSymKey(context->key);
    if (context->param) SECITEM_FreeItem(context->param, PR_TRUE);
    PK11_FreeSlot(context->slot);
    if (freeit) PORT_Free(context);
}

/*
 * save the current context. Allocate Space if necessary.
 */
void *
pk11_saveContext(PK11Context *context,void *space, unsigned long *savedLength)
{
    CK_ULONG length;
    CK_RV crv;

    if (space == NULL) {
	crv =PK11_GETTAB(context->slot)->C_GetOperationState(context->session,
				NULL,&length);
	if (crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    return NULL;
	}
	context->savedData = PORT_Alloc(length);
	if (context->savedData == NULL) return NULL;
	*savedLength = length;
    }
    crv = PK11_GETTAB(context->slot)->C_GetOperationState(context->session,
					space,savedLength);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }
    return space;
}

/*
 * restore the current context
 */
SECStatus
pk11_restoreContext(PK11Context *context,void *space, unsigned long savedLength)
{
    CK_RV crv;
    CK_OBJECT_HANDLE objectID = (context->key) ? context->key->objectID:
			CK_INVALID_KEY;

    PORT_Assert(space != NULL);
    if (space == NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    crv = PK11_GETTAB(context->slot)->C_SetOperationState(context->session,
        space, savedLength, objectID, 0);
    if (crv != CKR_OK) {
       PORT_SetError( PK11_MapError(crv));
       return SECFailure;
   }
   return SECSuccess;
}

/*
 * Context initialization. Used by all flavors of CreateContext
 */
static SECStatus 
pk11_context_init(PK11Context *context, CK_MECHANISM *mech_info)
{
    CK_RV crv;
    PK11SymKey *symKey = context->key;

    switch (context->operation) {
    case CKA_ENCRYPT:
	crv=PK11_GETTAB(context->slot)->C_EncryptInit(context->session,
				mech_info, symKey->objectID);
	break;
    case CKA_DECRYPT:
	crv=PK11_GETTAB(context->slot)->C_DecryptInit(context->session,
				mech_info, symKey->objectID);
	break;
    case CKA_SIGN:
	crv=PK11_GETTAB(context->slot)->C_SignInit(context->session,
				mech_info, symKey->objectID);
	break;
    case CKA_DIGEST:
	crv=PK11_GETTAB(context->slot)->C_DigestInit(context->session,
				mech_info);
	break;
    default:
	crv = CKR_OPERATION_NOT_INITIALIZED;
	break;
    }

    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
        return SECFailure;
    }

    /*
     * handle session starvation case.. use our last session to multiplex
     */
    if (!context->ownSession) {
	context->savedData = pk11_saveContext(context,context->savedData,
				&context->savedLength);
	if (context->savedData == NULL) return SECFailure;
	/* clear out out session for others to use */
	return PK11_Finalize(context);
    }
    return SECSuccess;
}


/*
 * Common Helper Function do come up with a new context.
 */
static PK11Context *pk11_CreateNewContextInSlot(CK_MECHANISM_TYPE type,
     PK11SlotInfo *slot, CK_ATTRIBUTE_TYPE operation, PK11SymKey *symKey,
							     SECItem *param)
{
    CK_MECHANISM mech_info;
    PK11Context *context;
    SECStatus rv;
	
    context = (PK11Context *) PORT_Alloc(sizeof(PK11Context));
    if (context == NULL) {
	return NULL;
    }

    /* initialize the critical fields of the context */
    context->operation = operation;
    context->key = symKey ? PK11_ReferenceSymKey(symKey) : NULL;
    context->slot = PK11_ReferenceSlot(slot);
    context->session = pk11_GetNewSession(slot,&context->ownSession);
    context->cx = symKey ? symKey->cx : NULL;
    /* get our session */
    context->savedData = NULL;

    /* save the parameters so that some digesting stuff can do multiple
     * begins on a single context */
    context->type = type;
    context->param = SECITEM_DupItem(param);
    context->init = PR_FALSE;
    if (context->param == NULL) {
	PK11_DestroyContext(context,PR_TRUE);
	return NULL;
    }

    mech_info.mechanism = type;
    mech_info.pParameter = param->data;
    mech_info.ulParameterLen = param->len;
    rv = pk11_context_init(context,&mech_info);

    if (rv != SECSuccess) {
	PK11_DestroyContext(context,PR_TRUE);
	return NULL;
    }
    context->init = PR_TRUE;
    return context;
}


/*
 * put together the various PK11_Create_Context calls used by different
 * parts of libsec.
 */
PK11Context *
PK11_CreateContextByRawKey(PK11SlotInfo *slot, CK_MECHANISM_TYPE type,
     CK_ATTRIBUTE_TYPE operation, SECItem *key, SECItem *param, void *wincx)
{
    PK11SymKey *symKey;
    PK11Context *context;

    /* first get a slot */
    if (slot == NULL) {
	slot = PK11_GetBestSlot(type,wincx);
	if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}
    } else {
	PK11_ReferenceSlot(slot);
    }

    /* now import the key */
    symKey = PK11_ImportSymKey(slot, type, operation,  key, wincx);
    if (symKey == NULL) return NULL;

    context = PK11_CreateContextBySymKey(type, operation, symKey, param);

    PK11_FreeSymKey(symKey);
    PK11_FreeSlot(slot);

    return context;
}


/*
 * Create a context from a key. We really should make sure we aren't using
 * the same key in multiple session!
 */
PK11Context *
PK11_CreateContextBySymKey(CK_MECHANISM_TYPE type,CK_ATTRIBUTE_TYPE operation,
			PK11SymKey *symKey, SECItem *param)
{
    PK11SymKey *newKey;
    PK11Context *context;

    /* if this slot doesn't support the mechanism, go to a slot that does */
    newKey = pk11_ForceSlot(symKey,type,operation);
    if (newKey == NULL) {
	PK11_ReferenceSymKey(symKey);
    } else {
	symKey = newKey;
    }

    /* Context Adopts the symKey.... */
    context = pk11_CreateNewContextInSlot(type, symKey->slot, operation, symKey,
							     param);
    PK11_FreeSymKey(symKey);
    return context;
}

/*
 * Digest contexts don't need keys, but the do need to find a slot.
 * Macing should use PK11_CreateContextBySymKey.
 */
PK11Context *
PK11_CreateDigestContext(SECOidTag hashAlg)
{
    /* digesting has to work without authentication to the slot */
    CK_MECHANISM_TYPE type;
    PK11SlotInfo *slot;
    PK11Context *context;
    SECItem param;

    type = PK11_AlgtagToMechanism(hashAlg);
    slot = PK11_GetBestSlot(type, NULL);
    if (slot == NULL) {
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return NULL;
    }

    /* maybe should really be PK11_GenerateNewParam?? */
    param.data = NULL;
    param.len = 0;

    context = pk11_CreateNewContextInSlot(type, slot, CKA_DIGEST, NULL, &param);
    PK11_FreeSlot(slot);
    return context;
}

/*
 * create a new context which is the clone of the state of old context.
 */
PK11Context * PK11_CloneContext(PK11Context *old)
{
     PK11Context *new;
     PRBool needFree = PR_FALSE;
     SECStatus rv = SECSuccess;
     void *data;
     unsigned long len;

     new = pk11_CreateNewContextInSlot(old->type, old->slot, old->operation,
						old->key, old->param);
     if (new == NULL) return NULL;

     /* now clone the save state. First we need to find the save state
      * of the old session. If the old context owns it's session,
      * the state needs to be saved, otherwise the state is in saveData. */
     if (old->ownSession) {
	data=pk11_saveContext(old,NULL,&len);
	needFree = PR_TRUE;
     } else {
	data = old->savedData;
	len = old->savedLength;
     }

     if (data == NULL) {
	PK11_DestroyContext(new,PR_TRUE);
	return NULL;
     }

     /* now copy that state into our new context. Again we have different
      * work if the new context owns it's own session. If it does, we
      * restore the state gathered above. If it doesn't, we copy the
      * saveData pointer... */
     if (new->ownSession) {
	rv = pk11_restoreContext(new,data,len);
     } else {
	XP_ASSERT(new->savedData != NULL);
	if ((new->savedData == NULL) || (new->savedLength < len)) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    rv = SECFailure;
	} else {
	    PORT_Memcpy(new->savedData,data,len);
	    new->savedLength = len;
	}
    }

    if (needFree) PORT_Free(data);

    if (rv != SECSuccess) {
	PK11_DestroyContext(new,PR_TRUE);
	return NULL;
    }
    return new;
}

/*
 * This is a hack to get FIPS compliance until we can convert
 * libjar and ssl to use PK11_ hashing functions. It returns PR_FALSE
 * if we can't get a PK11 Context.
 */
PRBool
PK11_HashOK(SECOidTag algID) {
    PK11Context *cx;

    cx = PK11_CreateDigestContext(algID);
    if (cx == NULL) return PR_FALSE;
    PK11_DestroyContext(cx, PR_TRUE);
    return PR_TRUE;
}



/*
 * start a new digesting or Mac'ing operation on this context
 */
SECStatus PK11_DigestBegin(PK11Context *cx)
{
    CK_MECHANISM mech_info;
    SECStatus rv;

    if (cx->init == PR_TRUE) {
	return SECSuccess;
    }

    mech_info.mechanism = cx->type;
    mech_info.pParameter = cx->param->data;
    mech_info.ulParameterLen = cx->param->len;
    rv = pk11_context_init(cx,&mech_info);

    if (rv != SECSuccess) {
	return SECFailure;
    }
    cx->init = PR_TRUE;
    return SECSuccess;
}

SECStatus
PK11_HashBuf(SECOidTag hashAlg, unsigned char *out, unsigned char *in, 
								int32 len) {
    PK11Context *context;
    unsigned int max_length;
    unsigned int out_length;
    SECStatus rv;

    context = PK11_CreateDigestContext(hashAlg);
    if (context == NULL) return SECFailure;

    rv = PK11_DigestBegin(context);
    if (rv != SECSuccess) {
	PK11_DestroyContext(context, PR_TRUE);
	return rv;
    }

    rv = PK11_DigestOp(context, in, len);
    if (rv != SECSuccess) {
	PK11_DestroyContext(context, PR_TRUE);
	return rv;
    }

    /* Sigh, we need the output length */
    switch (hashAlg) {
    case  SEC_OID_SHA1: max_length = SHA1_LENGTH; break;
    case  SEC_OID_MD2: max_length = MD2_LENGTH; break;
    case  SEC_OID_MD5: max_length = MD5_LENGTH; break;
    default: max_length = 16; break;
    }

    rv = PK11_DigestFinal(context,out,&out_length,max_length);
    PK11_DestroyContext(context, PR_TRUE);
    return rv;
}


/*
 * execute a bulk encryption operation
 */
SECStatus
PK11_CipherOp(PK11Context *context, unsigned char * out, int *outlen, 
				int maxout, unsigned char *in, int inlen)
{
    CK_RV crv;
    CK_ULONG length = maxout;
    PK11SymKey *symKey = context->key;
    SECStatus rv;

    /* if we ran out of session, we need to restore our previously stored
     * state.
     */
    if (!context->ownSession) {
        rv = pk11_restoreContext(context,context->savedData,
							context->savedLength);
	if (rv != SECSuccess) return rv;
    }

    switch (context->operation) {
    case CKA_ENCRYPT:
	length = maxout;
	crv=PK11_GETTAB(context->slot)->C_EncryptUpdate(context->session,
						in, inlen, out, &length);
	break;
    case CKA_DECRYPT:
	length = maxout;
	crv=PK11_GETTAB(context->slot)->C_DecryptUpdate(context->session,
						in, inlen, out, &length);
	break;
    default:
	crv = CKR_OPERATION_NOT_INITIALIZED;
	break;
    }

    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
        return SECFailure;
    }
    *outlen = length;

    /*
     * handle session starvation case.. use our last session to multiplex
     */
    if (!context->ownSession) {
	context->savedData = pk11_saveContext(context,context->savedData,
				&context->savedLength);
	if (context->savedData == NULL) return SECFailure;
	
	/* clear out out session for others to use */
	return PK11_Finalize(context);
    }
    return SECSuccess;
}

/*
 * execute a bulk encryption operation
 */
SECStatus
PK11_DigestOp(PK11Context *context, unsigned char * in, unsigned inLen) 
{
    CK_RV crv;
    SECStatus rv;

    /* if we ran out of session, we need to restore our previously stored
     * state.
     */
    if (!context->ownSession) {
        rv = pk11_restoreContext(context,context->savedData,
							context->savedLength);
	if (rv != SECSuccess) return rv;
    }

    switch (context->operation) {
    /* also for MAC'ing */
    case CKA_SIGN:
	crv=PK11_GETTAB(context->slot)->C_SignUpdate(context->session,
						in, inLen);
	break;
    case CKA_DIGEST:
	crv=PK11_GETTAB(context->slot)->C_DigestUpdate(context->session,
						in, inLen);
	break;
    default:
	crv = CKR_OPERATION_NOT_INITIALIZED;
	break;
    }

    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
        return SECFailure;
    }

    /*
     * handle session starvation case.. use our last session to multiplex
     */
    if (!context->ownSession) {
	context->savedData = pk11_saveContext(context,context->savedData,
				&context->savedLength);
	if (context->savedData == NULL) return SECFailure;
	
	/* clear out out session for others to use */
	return PK11_Finalize(context);
    }
    return SECSuccess;
}


/*
 * clean up a cipher operation, so the session can be used by
 * someone new.
 */
SECStatus
PK11_Finalize(PK11Context *context)
{
    CK_ULONG count;
    CK_RV crv;

    if (!context->ownSession) {
	return SECSuccess;
    }

    switch (context->operation) {
    case CKA_ENCRYPT:
	crv=PK11_GETTAB(context->slot)->C_EncryptFinal(context->session,
				NULL,&count);
	break;
    case CKA_DECRYPT:
	crv = PK11_GETTAB(context->slot)->C_DecryptFinal(context->session,
				NULL,&count);
	break;
    case CKA_SIGN:
	crv=PK11_GETTAB(context->slot)->C_SignFinal(context->session,
				NULL,&count);
	break;
    case CKA_DIGEST:
	crv=PK11_GETTAB(context->slot)->C_DigestFinal(context->session,
				NULL,&count);
	break;
    default:
	crv = CKR_OPERATION_NOT_INITIALIZED;
	break;
    }

    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
        return SECFailure;
    }
    return SECSuccess;
}

/*
 *  Return the final digested or signed data...
 *  this routine can either take pre initialized data, or allocate data
 *  either out of an arena or out of the standard heap.
 */
SECStatus
PK11_DigestFinal(PK11Context *context,unsigned char *data, 
			unsigned int *outLen, unsigned int length)
{
    CK_ULONG len;
    CK_RV crv;
    SECStatus rv;


    /* if we ran out of session, we need to restore our previously stored
     * state.
     */
    if (!context->ownSession) {
        rv = pk11_restoreContext(context,context->savedData,
							context->savedLength);
	if (rv != SECSuccess) return rv;
    }

    len = length;
    switch (context->operation) {
    case CKA_SIGN:
	crv=PK11_GETTAB(context->slot)->C_SignFinal(context->session,
				data,&len);
	break;
    case CKA_DIGEST:
	crv=PK11_GETTAB(context->slot)->C_DigestFinal(context->session,
				data,&len);
	break;
    case CKA_ENCRYPT:
	crv=PK11_GETTAB(context->slot)->C_EncryptFinal(context->session,
				data, &len);
	break;
    case CKA_DECRYPT:
	crv = PK11_GETTAB(context->slot)->C_DecryptFinal(context->session,
				data, &len);
	break;
    default:
	crv = CKR_OPERATION_NOT_INITIALIZED;
	break;
    }

    *outLen = (unsigned int) len;

    context->init = PR_FALSE; /* allow Begin to start up again */

    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
	return SECFailure;
    }
    return SECSuccess;
}

SECAlgorithmID *
PK11_CreatePBEAlgorithmID(SECOidTag algorithm, int iteration, SECItem *salt,
			  int keyLength, SECItem *iv, SECItem *op_param)
{
    SECAlgorithmID *algid;

    algid = SEC_PKCS5CreateAlgorithmID(algorithm, salt, iteration, keyLength,
    				       				iv, op_param);
    return algid;
}

PK11SymKey *
PK11_PBEKeyGen(PK11SlotInfo *slot, SECAlgorithmID *algid, SECItem *pwitem,
	       							void *wincx)
{
    CK_ULONG key_size = 0;
    /* we have to use these native types because when we call PKCS 11 modules
     * we have to make sure that we are using the correct sizes for all the
     * parameters. */
    CK_BBOOL ckfalse = FALSE; /* Sigh */
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_ATTRIBUTE genTemplate[] = {
	{ CKA_SENSITIVE, NULL, 0},
	{ CKA_ENCRYPT, NULL, 0},
	/* the next entry must be the last one */
	{ CKA_VALUE_LEN, NULL, 0},
    };
    int count = sizeof(genTemplate)/sizeof(genTemplate[0]);
    CK_MECHANISM mechanism;
    CK_MECHANISM_TYPE key_gen_mechanism;
    PK11SymKey *symKey;
    CK_RV crv;
    CK_ATTRIBUTE *attrs = genTemplate;

    /* pbe stuff */
    CK_NETSCAPE_PBE_PARAMS *pbe_params;
    CK_MECHANISM_TYPE type;
    SECItem *mech;

    mech = PK11_ParamFromAlgid(algid);
    type = PK11_AlgtagToMechanism(SECOID_FindOIDTag(&algid->algorithm));
    if(mech == NULL) {
	return NULL;
    } else {
	pbe_params = (CK_NETSCAPE_PBE_PARAMS *)mech->data;
	pbe_params->pPassword = (CK_CHAR_PTR)PORT_ZAlloc(pwitem->len);
	if(pbe_params->pPassword != NULL) {
	    PORT_Memcpy(pbe_params->pPassword, pwitem->data, pwitem->len);
	    pbe_params->ulPasswordLen = pwitem->len;
	} else {
	    SECITEM_ZfreeItem(mech, PR_TRUE);
	    return NULL;
	}
    }

    PK11_SETATTRS(attrs, CKA_SENSITIVE, &ckfalse, sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_ENCRYPT, &cktrue, sizeof(CK_BBOOL)); attrs++;
    /* the next entry must be the last one */
    PK11_SETATTRS(attrs, CKA_VALUE_LEN, &key_size, sizeof(key_size)); attrs++;

    /* find a slot to generate the key into */
    if ((slot == NULL) || (!PK11_DoesMechanism(slot,type))) {
        slot = PK11_GetBestSlot(type,wincx);
        if (slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}
    } else {
	PK11_ReferenceSlot(slot);
    }

    /* Initialize the Key Gen Mechanism */
    key_gen_mechanism = PK11_GetKeyGen(type);
    if (key_gen_mechanism == CKM_FAKE_RANDOM) {
	PK11_FreeSlot(slot);
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return NULL;
    }
    mechanism.mechanism = type;
    mechanism.pParameter = mech->data;
    mechanism.ulParameterLen = mech->len;

    /* if key_size was not specified, the mechanism must know it's own size */
    if (key_size == 0) {
	count--;
    }

    /* get our key Structure */
    symKey = PK11_CreateSymKey(slot,type,wincx);
    PK11_FreeSlot(slot);
    if (symKey == NULL) {
	SECITEM_ZfreeItem(mech, PR_TRUE);
	return NULL;
    }
    symKey->size = key_size;

    crv = PK11_GETTAB(symKey->slot)->C_GenerateKey(symKey->slot->session,
			 &mechanism, genTemplate, count, &symKey->objectID);
    if (crv != CKR_OK) {
	SECITEM_ZfreeItem(mech, PR_TRUE);
	PK11_FreeSymKey(symKey);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }
    SECITEM_ZfreeItem(mech, PR_TRUE);
    return symKey;
}

SECStatus 
PK11_ImportEncryptedPrivateKeyInfo(PK11SlotInfo *slot,
   SECKEYEncryptedPrivateKeyInfo *epki, SECItem *pwitem,
				   		SECItem *nickname, void *wincx)
{
    SECKEYPrivateKeyInfo *pki = NULL;
    SECItem dbuf;
    CK_MECHANISM_TYPE mechanism;
    SECItem *pbe_param, crypto_param;
    PK11SymKey *key = NULL;
    SECStatus rv = SECSuccess;
    CK_MECHANISM cryptoMech, pbeMech;
    CK_RV crv;
    void *cx;
    int bs;

    if((epki == NULL) || (pwitem == NULL))
	return SECFailure;

    dbuf.data = NULL;
    crypto_param.data = NULL;

    mechanism = PK11_AlgtagToMechanism(SECOID_FindOIDTag(
    	&epki->algorithm.algorithm));
    pbe_param = PK11_ParamFromAlgid(&epki->algorithm);
    key = PK11_PBEKeyGen(slot, &epki->algorithm, pwitem, wincx);
    if((key == NULL) || (pbe_param == NULL)) {
	rv = SECFailure;
	goto loser;
    }

    pbeMech.mechanism = mechanism;
    pbeMech.pParameter = pbe_param->data;
    pbeMech.ulParameterLen = pbe_param->len;

    crv = PK11_MapPBEMechanismToCryptoMechanism(&pbeMech, &cryptoMech, pwitem);
    if(crv != CKR_OK) {
	rv = SECFailure;
	goto loser;
    }
    crypto_param.data = cryptoMech.pParameter;
    crypto_param.len = cryptoMech.ulParameterLen;

    dbuf.data = (unsigned char *)PORT_ZAlloc(epki->encryptedData.len + 64);
    if(dbuf.data == NULL) {
	rv = SECFailure;
        goto loser;
    }
    dbuf.len = epki->encryptedData.len + 64;

    cx = PK11_CreateContextBySymKey(cryptoMech.mechanism, CKA_DECRYPT, 
    				    key, &crypto_param);
    if(cx == NULL) {
	rv = SECFailure;
	goto loser;
    }
    rv = PK11_CipherOp(cx, dbuf.data, (int *)(&dbuf.len), (int)dbuf.len, 
    			epki->encryptedData.data, (int)epki->encryptedData.len);
    PK11_DestroyContext(cx, PR_TRUE);
    if(rv != SECSuccess) {
	goto loser;
    }

    /* remove pad characters 
     * assumes padding via RC2 cbc or DES cbc variant
     */
    bs = PK11_GetBlockSize(cryptoMech.mechanism, &crypto_param);
    if(bs) {
	if(((int)dbuf.data[dbuf.len-1] > 0) && 
	  ((int)dbuf.data[dbuf.len-1] <= bs)) {
	    dbuf.len = (dbuf.len - (int)dbuf.data[dbuf.len-1]);
	} else {
	    /* set an error? */
	    rv = SECFailure;
	    goto loser;
	}
    }

    pki = (SECKEYPrivateKeyInfo *)PORT_ZAlloc(sizeof(SECKEYPrivateKeyInfo));
    if(pki == NULL) {
	rv = SECFailure;
	goto loser;
    }
    rv = SEC_ASN1DecodeItem(NULL, pki, SECKEY_PrivateKeyInfoTemplate, &dbuf);
    if(rv != SECFailure) {
	rv = PK11_ImportPrivateKeyInfo(slot, pki, nickname, wincx);
    }

loser:
    if(pbe_param != NULL) {
	SECITEM_ZfreeItem(pbe_param, PR_TRUE);
	pbe_param = NULL;
    }

    if(crypto_param.data != NULL) {
	SECITEM_ZfreeItem(&crypto_param, PR_FALSE);
    }

    if(dbuf.data != NULL) {
	SECITEM_ZfreeItem(&dbuf, PR_FALSE);
    }

    if(key != NULL) {
    	PK11_FreeSymKey(key);
    }

    return rv;
}

/*
 * import a pprivate key info into the desired slot
 */
SECStatus
PK11_ImportPrivateKeyInfo(PK11SlotInfo *slot, SECKEYPrivateKeyInfo *pki, 
						SECItem *nickname, void *wincx) 
{
    CK_BBOOL cktrue = TRUE; /* sigh */
    CK_OBJECT_CLASS keyClass = CKO_PRIVATE_KEY;
    CK_KEY_TYPE keyType = CKK_RSA;
    CK_OBJECT_HANDLE objectID;
    CK_ATTRIBUTE *template = NULL;
    int templateCount = 0;
    SECStatus rv = SECFailure;
    SECKEYLowPrivateKey *lpk = NULL;

    /* need to change this to use RSA/DSA keys */
    lpk = (SECKEYLowPrivateKey *)PORT_ZAlloc(sizeof(SECKEYLowPrivateKey));
    if(lpk == NULL) {
	goto loser;
    }
    rv = SEC_ASN1DecodeItem(NULL, lpk, SECKEY_RSAPrivateKeyTemplate,
			    &pki->privateKey);
    lpk->keyType = rsaKey; /* XXX RSA */
    lpk->arena = NULL;
    if(rv == SECFailure) {
	return SECFailure;
    }

    rv = SECFailure;

    /* now import the key */
    {
	CK_ATTRIBUTE rsaTemplate[] = {
	    { CKA_CLASS, NULL, 0 },
	    { CKA_KEY_TYPE, NULL, 0 },
	    { CKA_TOKEN, NULL, 0 },
	    { CKA_SENSITIVE, NULL, 0 },
	    { CKA_PRIVATE, NULL, 0 },
	    { CKA_UNWRAP,  NULL, 0 },
	    { CKA_DECRYPT, NULL, 0 },
	    { CKA_SIGN, NULL, 0 },
	    { CKA_SIGN_RECOVER, NULL, 0 },
	    { CKA_ID, NULL, 0 },
	    { CKA_LABEL, NULL, 0 }, 
	    { CKA_MODULUS, NULL, 0 },
	    { CKA_PUBLIC_EXPONENT, NULL, 0 },
	    { CKA_PRIVATE_EXPONENT, NULL, 0 },
	    { CKA_PRIME_1, NULL, 0 },
	    { CKA_PRIME_2, NULL, 0 },
	    { CKA_EXPONENT_1, NULL, 0 },
	    { CKA_EXPONENT_2, NULL, 0 },
	    { CKA_COEFFICIENT, NULL, 0 },
	};
        CK_ATTRIBUTE *attrs;

	CK_ATTRIBUTE dsaTemplate[] = {
	    { CKA_CLASS, NULL, 0},
	    { CKA_KEY_TYPE, NULL, 0},
	    { CKA_TOKEN, NULL, 0 },
	    { CKA_PRIVATE, NULL, 0 },
	    { CKA_SIGN, NULL, 0 },
	    { CKA_SIGN_RECOVER, NULL, 0 },
	    { CKA_ID, NULL, 0 },
	    { CKA_LABEL, NULL, 0 }, 
	    { CKA_PRIME, NULL, 0},
	    { CKA_SUBPRIME, NULL, 0},
	    { CKA_BASE, NULL, 0},
	    { CKA_VALUE, NULL, 0},
	};

        switch (lpk->keyType) {
        case rsaKey:
	    attrs = template = rsaTemplate;
	    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
			  			sizeof(keyClass) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, 
						sizeof(keyType) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_TOKEN, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_SENSITIVE, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_PRIVATE, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_UNWRAP, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_DECRYPT, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_SIGN, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_SIGN_RECOVER, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_ID, lpk->u.rsa.modulus.data,
	    				 lpk->u.rsa.modulus.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_LABEL, nickname->data, nickname->len); attrs++; 
	    PK11_SETATTRS(attrs, CKA_MODULUS, lpk->u.rsa.modulus.data,
						lpk->u.rsa.modulus.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PUBLIC_EXPONENT, 
	     					lpk->u.rsa.publicExponent.data,
					 	lpk->u.rsa.publicExponent.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PRIVATE_EXPONENT, 
	     					lpk->u.rsa.privateExponent.data,
					 	lpk->u.rsa.privateExponent.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PRIME_1, 
	     					lpk->u.rsa.prime[0].data,
					 	lpk->u.rsa.prime[0].len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PRIME_2, 
	     					lpk->u.rsa.prime[1].data,
					 	lpk->u.rsa.prime[1].len); attrs++;
	    PK11_SETATTRS(attrs, CKA_EXPONENT_1, 
	     					lpk->u.rsa.primeExponent[0].data,
					 	lpk->u.rsa.primeExponent[0].len); attrs++;
	    PK11_SETATTRS(attrs, CKA_EXPONENT_2, 
	     					lpk->u.rsa.primeExponent[1].data,
					 	lpk->u.rsa.primeExponent[1].len); attrs++;
	    PK11_SETATTRS(attrs, CKA_COEFFICIENT, 
	     					lpk->u.rsa.coefficient.data,
					 	lpk->u.rsa.coefficient.len);
	    templateCount = sizeof(rsaTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_RSA;
	    break;
        case dsaKey:
	    attrs = template = dsaTemplate;
	    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
						sizeof(keyClass)); attrs++;
	    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, 
						sizeof(keyType)); attrs++;
	    PK11_SETATTRS(attrs, CKA_TOKEN, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_PRIVATE, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_SIGN, &cktrue, sizeof(CK_BBOOL)); attrs++;
	    PK11_SETATTRS(attrs, CKA_SIGN_RECOVER, &cktrue, sizeof(CK_BBOOL)); attrs++;
	    PK11_SETATTRS(attrs, CKA_LABEL, nickname->data, nickname->len); attrs++; 
	    PK11_SETATTRS(attrs, CKA_ID,    lpk->u.dsa.publicValue.data, 
					lpk->u.dsa.publicValue.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PRIME,    lpk->u.dsa.params.prime.data,
				lpk->u.dsa.params.prime.len); attrs++;
	    PK11_SETATTRS(attrs,CKA_SUBPRIME,lpk->u.dsa.params.subPrime.data,
				lpk->u.dsa.params.subPrime.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_BASE,  lpk->u.dsa.params.base.data,
					lpk->u.dsa.params.base.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_VALUE,    lpk->u.dsa.publicValue.data, 
					lpk->u.dsa.publicValue.len); attrs++;
	    templateCount = sizeof(dsaTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_DSA;
	    break;
#ifdef notdef
        case dhKey:
	    template = dhTemplate;
	    templateCount = sizeof(dhTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_DH;
	    break;
#endif
	/* what about fortezza??? */
	default:
	    PORT_SetError(SEC_ERROR_BAD_KEY);
	    goto loser;
	}

        rv = PK11_CreateNewObject(slot,template,templateCount, &objectID);

    }

loser:
    if(lpk != NULL) {
	SECKEY_LowDestroyPrivateKey(lpk);
    }

    return rv;
}

SECKEYPrivateKeyInfo *
PK11_ExportPrivateKeyInfo(SECItem *nickname, CERTCertificate *cert,
			  					void *wincx)
{
    SECKEYPrivateKey *pk = NULL;
    SECKEYPrivateKeyInfo *pki = NULL;
    SECKEYLowPrivateKey *lk = NULL;
    SECItem *dlk = NULL;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECFailure;
    SECItem pkitem;
    CK_RV crv;
    CK_MECHANISM mechanism;
    void *dummy;

    pk = PK11_FindKeyByAnyCert(cert, wincx);
    if(pk == NULL) {
	return NULL;
    }

    pkitem.data = (unsigned char *)PORT_ZAlloc(sizeof(SECKEYLowPrivateKey *));
    mechanism.mechanism = CKM_NETSCAPE_NULL_ENCRYPT;

    /* XXX we are extracting an unencrypted SECKEYLowPrivateKey structure.
     * which needs to be freed along with the buffer into which it is
     * returned.  eventually, we should retrieve an encrypted key using
     * pkcs8/pkcs5.
     */
    crv = PK11_GETTAB(pk->pkcs11Slot)->C_WrapKey(pk->pkcs11Slot->session, 
    		&mechanism, 0, pk->pkcs11ID, pkitem.data, 
		(CK_ULONG_PTR)(&pkitem.len)); 
    if(crv != CKR_OK) {
	rv = SECFailure;
	goto loser;
    }

    lk = *((SECKEYLowPrivateKey **)pkitem.data);
    if(lk == NULL) {
	rv = SECFailure;
	goto loser;
    }

    PORT_Assert(lk->keyType == rsaKey); /* XXX RSA */
    dlk = SEC_ASN1EncodeItem(NULL, NULL, lk, SECKEY_RSAPrivateKeyTemplate);
    if(dlk == NULL) {
	rv = SECFailure;
	goto loser;
    }

    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(arena == NULL) {
	rv = SECFailure;
	goto loser;
    }
    pki = (SECKEYPrivateKeyInfo *)PORT_ArenaZAlloc(arena, 
    					sizeof(SECKEYPrivateKeyInfo));
    if(pki == NULL) {
	rv = SECFailure;
	goto loser;
    }
    dummy = SEC_ASN1EncodeInteger(arena, &(pki->version), 
				  SEC_PRIVATE_KEY_INFO_VERSION);
    if(dummy == NULL) {
	rv = SECFailure;
	goto loser;
    }
    pki->arena = arena;
    switch(lk->keyType) {
	case rsaKey:
	case dsaKey:
	    rv = SECOID_SetAlgorithmID(arena, &pki->algorithm, 
	    	SEC_OID_PKCS1_RSA_ENCRYPTION, NULL);
	    break;
	default:
	    rv = SECFailure;
	    goto loser;
    }
    rv = SECITEM_CopyItem(arena, &pki->privateKey, dlk);

loser:
    if(pk != NULL) {
	SECKEY_DestroyPrivateKey(pk);
	pk = NULL;
    }

    if(dlk != NULL) {
	SECITEM_ZfreeItem(dlk, PR_TRUE);
	dlk = NULL;
    }

    if(lk != NULL) {
	SECKEY_LowDestroyPrivateKey(lk);
	lk = NULL;
    }

    if(pkitem.data != NULL) {
	PORT_Free(pkitem.data);
	pkitem.data = NULL;
    }

    if(rv == SECFailure) {
	if(arena != NULL) {
	    PORT_FreeArena(arena, PR_TRUE);
	    arena = NULL;
	}
	pki = NULL;
    }

    return pki;
}

SECKEYEncryptedPrivateKeyInfo * 
PK11_ExportEncryptedPrivateKeyInfo(PK11SlotInfo *slot, SECOidTag algTag,
   SECItem *pwitem, SECItem *nickname, CERTCertificate *cert, 
				int iteration, int keyLength, void *wincx)
{
    SECKEYPrivateKeyInfo *pki = NULL;
    SECKEYEncryptedPrivateKeyInfo *epki = NULL;
    PRArenaPool *arena = NULL;
    SECAlgorithmID *algid;
    SECItem *dbuf;
    CK_MECHANISM_TYPE mechanism;
    SECItem *pbe_param = NULL, crypto_param;
    PK11SymKey *key = NULL;
    SECStatus rv = SECSuccess;
    void *cx;
    CK_MECHANISM pbeMech;
    CK_MECHANISM cryptoMech;
    CK_RV crv;
    SECItem *blocked_data = NULL;
    int bs;

    if((pwitem == NULL) || (nickname == NULL))
	return NULL;

    pki = PK11_ExportPrivateKeyInfo(nickname, cert, wincx);
    if(pki == NULL) {
	return NULL;
    }
    dbuf = SEC_ASN1EncodeItem(NULL, NULL, pki, SECKEY_PrivateKeyInfoTemplate);
    if(dbuf == NULL) {
	rv = SECFailure;
	goto loser;
    }

    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    epki = (SECKEYEncryptedPrivateKeyInfo *)PORT_ArenaZAlloc(arena, 
    		sizeof(SECKEYEncryptedPrivateKeyInfo));
    if(epki == NULL) {
	rv = SECFailure;
	goto loser;
    }
    epki->arena = arena;
    algid = SEC_PKCS5CreateAlgorithmID(algTag, NULL, iteration, 	
    		keyLength, NULL, NULL);
    if(algid == NULL) {
	rv = SECFailure;
	goto loser;
    }

    mechanism = PK11_AlgtagToMechanism(SECOID_FindOIDTag(&algid->algorithm));
    pbe_param = PK11_ParamFromAlgid(algid);
    pbeMech.mechanism = mechanism;
    pbeMech.pParameter = pbe_param->data;
    pbeMech.ulParameterLen = pbe_param->len;
    key = PK11_PBEKeyGen(slot, algid, pwitem, wincx);

    if((key == NULL) || (pbe_param == NULL)) {
	rv = SECFailure;
	goto loser;
    }

    crv = PK11_MapPBEMechanismToCryptoMechanism(&pbeMech, &cryptoMech, pwitem);
    if(crv != CKR_OK) {
	rv = SECFailure;
	goto loser;
    }
    crypto_param.data = (unsigned char *)cryptoMech.pParameter;
    crypto_param.len = cryptoMech.ulParameterLen;

    epki->encryptedData.data = (unsigned char *)PORT_ArenaZAlloc(arena,
    							dbuf->len + 64);
    if(epki->encryptedData.data == NULL) {
	rv = SECFailure;
	goto loser;
    }

    /* block according to PKCS 8 */
    bs = PK11_GetBlockSize(cryptoMech.mechanism, &crypto_param);
    rv = SECSuccess;
    if(bs) {
	char pad_char;
	pad_char = (char)(bs - (dbuf->len % bs));
	if(dbuf->len % bs) {
	    rv = SECSuccess;
	    blocked_data = PK11_BlockData(dbuf, bs);
	    if(blocked_data) {
		PORT_Memset((blocked_data->data + blocked_data->len - (int)pad_char), 
			    pad_char, (int)pad_char);
	    } else {
		rv = SECFailure;
		goto loser;
	    }
	} else {
	    blocked_data = SECITEM_DupItem(dbuf);
	    if(blocked_data) {
		blocked_data->data = PORT_Realloc(blocked_data->data,
						  blocked_data->len + bs);
		if(blocked_data->data) {
		    blocked_data->len += bs;
		    PORT_Memset((blocked_data->data + dbuf->len), (char)bs, bs);
		} else {
		    rv = SECFailure;
		    goto loser;
		}
	    } else {
		rv = SECFailure;
		goto loser;
	    }
	 }
    }

    cx = PK11_CreateContextBySymKey(cryptoMech.mechanism, CKA_ENCRYPT, 
		key, &crypto_param);
    if(cx == NULL) {
	rv = SECFailure;
	goto loser;
    }

    rv = PK11_CipherOp(cx, epki->encryptedData.data, 
    			(int *)(&epki->encryptedData.len), (int)(dbuf->len + 64),
    			blocked_data->data, (int)blocked_data->len);
    PK11_DestroyContext(cx, PR_TRUE);
    if(rv != SECSuccess) {
	goto loser;
    }

    rv = SECOID_CopyAlgorithmID(arena, &epki->algorithm, algid);

loser:
    if(pbe_param != NULL) {
	SECITEM_ZfreeItem(pbe_param, PR_TRUE);
	pbe_param = NULL;
    }

    if(blocked_data != NULL) {
	SECITEM_ZfreeItem(blocked_data, PR_TRUE);
	blocked_data = NULL;
    }

    if(crypto_param.data != NULL) {
	SECITEM_ZfreeItem(&crypto_param, PR_FALSE);
	crypto_param.data = NULL;
    }

    if(dbuf != NULL) {
	SECITEM_ZfreeItem(dbuf, PR_TRUE);
    }

    if(key != NULL) {
    	PK11_FreeSymKey(key);
    }

    if(pki != NULL) {
	PORT_FreeArena(pki->arena, PR_FALSE);
    }

    if(rv == SECFailure) {
	if(arena != NULL) {
	    PORT_FreeArena(arena, PR_TRUE);
	}
	epki = NULL;
    }

    return epki;
}

