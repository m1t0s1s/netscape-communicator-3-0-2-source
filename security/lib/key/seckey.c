#include "key.h"
#include "secrng.h"
#include "crypto.h"
#include "secoid.h"
#include "secitem.h"
#include "secder.h"
#include "base64.h"
#include "secasn1.h"
#include "cert.h"
#include "pk11func.h"

extern int SEC_ERROR_NO_MEMORY;

DERTemplate CERTSubjectPublicKeyInfoTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTSubjectPublicKeyInfo) },
    { DER_INLINE,
	  offsetof(CERTSubjectPublicKeyInfo,algorithm),
	  SECAlgorithmIDTemplate, },
    { DER_BIT_STRING,
	  offsetof(CERTSubjectPublicKeyInfo,subjectPublicKey), },
    { 0, }
};

DERTemplate CERTPublicKeyAndChallengeTemplate[] =
{
    { DER_SEQUENCE, 0, NULL, sizeof(CERTPublicKeyAndChallenge) },
    { DER_ANY, offsetof(CERTPublicKeyAndChallenge,spki), },
    { DER_IA5_STRING, offsetof(CERTPublicKeyAndChallenge,challenge), },
    { 0, }
};

const SEC_ASN1Template SECKEY_RSAPublicKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SECKEYPublicKey) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYPublicKey,u.rsa.modulus), },
    { SEC_ASN1_INTEGER, offsetof(SECKEYPublicKey,u.rsa.publicExponent), },
    { 0, }
};

const SEC_ASN1Template SECKEY_RSAPrivateKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SECKEYPrivateKey) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.version) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.modulus) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.publicExponent) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.privateExponent) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.prime[0]) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.prime[1]) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.primeExponent[0]) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.primeExponent[1]) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.rsa.coefficient) },
    { 0 }                                                                     
};                                                                            

const SEC_ASN1Template SECKEY_DSAPublicKeyTemplate[] = {
    { SEC_ASN1_INTEGER, offsetof(SECKEYPublicKey,u.dsa.publicValue), },
    { 0, }
};

const SEC_ASN1Template SECKEY_DSAPrivateKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SECKEYLowPrivateKey) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.dsa.publicValue) },
    { SEC_ASN1_INTEGER, offsetof(SECKEYLowPrivateKey,u.dsa.privateValue) },
    { 0, }
};

const SEC_ASN1Template SECKEY_PQGParamsTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(PQGParams) },
    { SEC_ASN1_INTEGER, offsetof(PQGParams,prime) },
    { SEC_ASN1_INTEGER, offsetof(PQGParams,subPrime) },
    { SEC_ASN1_INTEGER, offsetof(PQGParams,base) },
    { 0, }
};

/*
 * NOTE: This only generates RSA Private Key's. If you need more,
 * We need to pass in some more params...
 */
SECKEYPrivateKey *
SECKEY_CreateRSAPrivateKey(int keySizeInBits,SECKEYPublicKey **pubk, void *cx)
{
    SECKEYPrivateKey *privk;
    PK11SlotInfo *slot = PK11_GetBestSlot(CKM_RSA_PKCS_KEY_PAIR_GEN,cx);
    PK11RSAGenParams param;

    param.keySizeInBits = keySizeInBits;
    param.pe = 65537L;
    
    privk = PK11_GenerateKeyPair(slot,CKM_RSA_PKCS_KEY_PAIR_GEN,&param,pubk,
					PR_FALSE, PR_TRUE, cx);
    PK11_FreeSlot(slot);
    return(privk);
}

void
SECKEY_LowDestroyPrivateKey(SECKEYLowPrivateKey *privk)
{
    if (privk && privk->arena) {
	PORT_FreeArena(privk->arena, PR_TRUE);
    }
}

void
SECKEY_LowDestroyPublicKey(SECKEYLowPublicKey *pubk)
{
    if (pubk && pubk->arena) {
	PORT_FreeArena(pubk->arena, PR_FALSE);
    }
}

void
SECKEY_DestroyPrivateKey(SECKEYPrivateKey *privk)
{
    if (privk) {
	if (privk->pkcs11Slot) {
	    if (privk->pkcs11IsTemp) {
	    	PK11_DestroyObject(privk->pkcs11Slot,privk->pkcs11ID);
	    }
	    PK11_FreeSlot(privk->pkcs11Slot);

	}
    	if (privk->arena) {
	    PORT_FreeArena(privk->arena, PR_TRUE);
	}
    }
}

void
SECKEY_DestroyPublicKey(SECKEYPublicKey *pubk)
{
    if (pubk) {
	if (pubk->pkcs11Slot) {
	    PK11_DestroyObject(pubk->pkcs11Slot,pubk->pkcs11ID);
	    PK11_FreeSlot(pubk->pkcs11Slot);
	}
    	if (pubk->arena) {
	    PORT_FreeArena(pubk->arena, PR_FALSE);
	}
    }
}

SECStatus
SECKEY_CopySubjectPublicKeyInfo(PRArenaPool *arena,
			     CERTSubjectPublicKeyInfo *to,
			     CERTSubjectPublicKeyInfo *from)
{
    SECStatus rv;

    rv = SECOID_CopyAlgorithmID(arena, &to->algorithm, &from->algorithm);
    if (rv == SECSuccess)
	rv = SECITEM_CopyItem(arena, &to->subjectPublicKey, &from->subjectPublicKey);

    return rv;
}

SECKEYPublicKey *
CERT_ExtractPublicKey(CERTSubjectPublicKeyInfo *spki)
{
    SECKEYPublicKey *pubk;
    SECItem os;
    SECStatus rv;
    PRArenaPool *arena;
    int tag;

    arena = PORT_NewArena (DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	return NULL;

    pubk = (SECKEYPublicKey *) PORT_ArenaZAlloc(arena, sizeof(SECKEYPublicKey));
    if (pubk == NULL) {
	PORT_FreeArena (arena, PR_FALSE);
	return NULL;
    }

    pubk->arena = arena;
    pubk->pkcs11Slot = 0;
    pubk->pkcs11ID = CK_INVALID_KEY;


    /* Convert bit string length from bits to bytes */
    os = spki->subjectPublicKey;
    DER_ConvertBitString (&os);

    tag = SECOID_GetAlgorithmTag(&spki->algorithm);
    switch ( tag ) {
      case SEC_OID_X500_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
	pubk->keyType = rsaKey;
	rv = SEC_ASN1DecodeItem(arena, pubk, SECKEY_RSAPublicKeyTemplate, &os);
	if (rv == SECSuccess)
	    return pubk;
	break;
      case SEC_OID_ANSIX9_DSA_SIGNATURE:
	pubk->keyType = dsaKey;
	rv = SEC_ASN1DecodeItem(arena, pubk, SECKEY_DSAPublicKeyTemplate, &os);
	if (rv != SECSuccess) break;
	rv = SEC_ASN1DecodeItem(arena, &pubk->u.dsa.params,
				SECKEY_PQGParamsTemplate,
				&spki->algorithm.parameters);
	if (rv == SECSuccess) return pubk;
	break;
      case SEC_OID_MISSI_KEA_DSS_OLD:
      case SEC_OID_MISSI_KEA_DSS:
      case SEC_OID_MISSI_DSS_OLD:
      case SEC_OID_MISSI_DSS:
	pubk->keyType = fortezzaKey;
#ifdef notdef
	rv = FortezzaDecodeCertKey(arena, pubk, &os,
				   &spki->algorithm.parameters);
#else
	rv = SECFailure;
#endif
	if (rv == SECSuccess)
	    return pubk;
	break;
      default:
	rv = SECFailure;
	break;
    }

    SECKEY_DestroyPublicKey (pubk);
    return NULL;
}

unsigned
SECKEY_PublicKeyStrength(SECKEYPublicKey *pubk)
{
    unsigned char b0;

    /* interpret modulus length as key strength... in
     * fortezza that's the public key length */

    switch (pubk->keyType) {
    case rsaKey:
    	b0 = pubk->u.rsa.modulus.data[0];
    	return b0 ? pubk->u.rsa.modulus.len : pubk->u.rsa.modulus.len - 1;
    case dsaKey:
    	b0 = pubk->u.dsa.publicValue.data[0];
    	return b0 ? pubk->u.dsa.publicValue.len :
	    pubk->u.dsa.publicValue.len - 1;
    case fortezzaKey:
	return MAX(pubk->u.fortezza.KEAKey.len, pubk->u.fortezza.DSSKey.len);
    default:
	break;
    }
    return 0;
}

unsigned
SECKEY_LowPublicModulusLen(SECKEYLowPublicKey *pubk)
{
    unsigned char b0;

    /* interpret modulus length as key strength... in
     * fortezza that's the public key length */

    switch (pubk->keyType) {
    case rsaKey:
    	b0 = pubk->u.rsa.modulus.data[0];
    	return b0 ? pubk->u.rsa.modulus.len : pubk->u.rsa.modulus.len - 1;
    default:
	break;
    }
    return 0;
}

unsigned
SECKEY_LowPrivateModulusLen(SECKEYLowPrivateKey *privk)
{

    unsigned char b0;

    switch (privk->keyType) {
    case rsaKey:
	b0 = privk->u.rsa.modulus.data[0];
	return b0 ? privk->u.rsa.modulus.len : privk->u.rsa.modulus.len - 1;
    default:
	break;
    }
    return 0;
}

SECKEYPrivateKey *
SECKEY_CopyPrivateKey(SECKEYPrivateKey *privk)
{
    SECKEYPrivateKey *copyk;
    PRArenaPool *arena;
    
    if (privk == NULL) {
	return NULL;
    }
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    copyk = (SECKEYPrivateKey *) PORT_ArenaZAlloc (arena, sizeof (SECKEYPrivateKey));
    if (copyk) {
	copyk->arena = arena;
	copyk->keyType = privk->keyType;

	/* copy the PKCS #11 parameters */
	copyk->pkcs11Slot = PK11_ReferenceSlot(privk->pkcs11Slot);
	/* if the key we're referencing was a temparary key we have just
	 * created, that we want to go away when we're through, we need
	 * to make a copy of it */
	if (privk->pkcs11IsTemp) {
	    copyk->pkcs11ID = 
			PK11_CopyKey(privk->pkcs11Slot,privk->pkcs11ID);
	    if (copyk->pkcs11ID == CK_INVALID_KEY) goto fail;
	} else {
	    copyk->pkcs11ID = privk->pkcs11ID;
	}
	copyk->pkcs11IsTemp = privk->pkcs11IsTemp;
	copyk->wincx = privk->wincx;
	return copyk;
    } else {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
    }

fail:
    PORT_FreeArena (arena, PR_FALSE);
    return NULL;
}

SECKEYPublicKey *
SECKEY_CopyPublicKey(SECKEYPublicKey *pubk)
{
    SECKEYPublicKey *copyk;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    copyk = (SECKEYPublicKey *) PORT_ArenaZAlloc (arena, sizeof (SECKEYPublicKey));
    if (copyk != NULL) {
	SECStatus rv = SECSuccess;

	copyk->arena = arena;
	copyk->keyType = pubk->keyType;
	copyk->pkcs11Slot = NULL;	/* go get own reference */
	copyk->pkcs11ID = CK_INVALID_KEY;
	switch (pubk->keyType) {
	  case rsaKey:
	    rv = SECITEM_CopyItem(arena, &copyk->u.rsa.modulus,
				  &pubk->u.rsa.modulus);
	    if (rv == SECSuccess) {
		rv = SECITEM_CopyItem (arena, &copyk->u.rsa.publicExponent,
				       &pubk->u.rsa.publicExponent);
		if (rv == SECSuccess)
		    return copyk;
	    }
	    break;
	  case dsaKey:
	    rv = SECITEM_CopyItem(arena, &copyk->u.dsa.publicValue,
				  &pubk->u.dsa.publicValue);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.dsa.params.prime,
				  &pubk->u.dsa.params.prime);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.dsa.params.subPrime,
				  &pubk->u.dsa.params.subPrime);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.dsa.params.base,
				  &pubk->u.dsa.params.base);
	    if (rv == SECSuccess) return copyk;
	    break;
	  case fortezzaKey:
	    copyk->u.fortezza.KEAversion = pubk->u.fortezza.KEAversion;
	    copyk->u.fortezza.DSSversion = pubk->u.fortezza.DSSversion;
	    PORT_Memcpy(copyk->u.fortezza.KMID, pubk->u.fortezza.KMID,
			sizeof(pubk->u.fortezza.KMID));
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.clearance, 
				  &pubk->u.fortezza.clearance);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.KEApriviledge, 
				&pubk->u.fortezza.KEApriviledge);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.DSSpriviledge, 
				&pubk->u.fortezza.DSSpriviledge);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.KEAKey, 
				&pubk->u.fortezza.KEAKey);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.DSSKey, 
				&pubk->u.fortezza.DSSKey);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.p, 
				  &pubk->u.fortezza.p);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.q, 
				&pubk->u.fortezza.q);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &copyk->u.fortezza.g, 
				&pubk->u.fortezza.g);
	    if (rv == SECSuccess) return copyk;
	    break;
	  case nullKey:
	    return copyk;
	  default:
	    rv = SECFailure;
	    break;
	}
	if (rv == SECSuccess)
	    return copyk;

	SECKEY_DestroyPublicKey (copyk);
    } else {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena (arena, PR_FALSE);
    return NULL;
}


SECKEYPublicKey *
SECKEY_ConvertToPublicKey(SECKEYPrivateKey *privk)
{
    SECKEYPublicKey *pubk;
    PRArenaPool *arena;
    CERTCertificate *cert;
    SECStatus rv;

    /*
     * First try to look up the cert.
     */
    cert = PK11_GetCertFromPrivateKey(privk);
    if (cert) {
	pubk = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
	CERT_DestroyCertificate(cert);
	return pubk;
    }

    /* couldn't find the cert, build pub key by hand */
    arena = PORT_NewArena (DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }
    pubk = (SECKEYPublicKey *)PORT_ArenaZAlloc(arena,
						   sizeof (SECKEYPublicKey));
    if (pubk == NULL) {
	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }
    pubk->keyType = privk->keyType;
    pubk->pkcs11Slot = NULL;
    pubk->pkcs11ID = CK_INVALID_KEY;
    /*
     * fortezza is at the head of this switch, since we don't want to
     * allocate an arena... CERT_ExtractPublicKey will to that for us.
     */
    switch(privk->keyType) {
      case fortezzaKey:
      case nullKey:
	/* Nothing to query, if the cert isn't there, we're hosed */
	break;
      case dsaKey:
	rv = PK11_ReadAttribute(privk->pkcs11Slot, privk->pkcs11ID,
				CKA_PRIME, arena, &pubk->u.dsa.params.prime);
	if (rv != SECSuccess)
	    break;
	rv = PK11_ReadAttribute(privk->pkcs11Slot, privk->pkcs11ID,
				CKA_SUBPRIME, arena,
				&pubk->u.dsa.params.subPrime);
	if (rv != SECSuccess)
	    break;
	rv = PK11_ReadAttribute(privk->pkcs11Slot, privk->pkcs11ID,
				CKA_BASE, arena, &pubk->u.dsa.params.base);
	if (rv != SECSuccess)
	    break;
	rv = PK11_ReadAttribute(privk->pkcs11Slot, privk->pkcs11ID,
				CKA_ID, arena, &pubk->u.dsa.publicValue);
	if (rv != SECSuccess)
	    break;
	return pubk;
	break;
      case rsaKey:
	rv = PK11_ReadAttribute(privk->pkcs11Slot,privk->pkcs11ID,
				CKA_MODULUS,arena,&pubk->u.rsa.modulus);
	if (rv != SECSuccess)  break;
	rv = PK11_ReadAttribute(privk->pkcs11Slot,privk->pkcs11ID,
			CKA_PUBLIC_EXPONENT,arena,&pubk->u.rsa.publicExponent);
	if (rv != SECSuccess)  break;
	return pubk;
	break;
    default:
	break;
    }

    PORT_FreeArena (arena, PR_FALSE);
    return NULL;
}


SECKEYLowPublicKey *
SECKEY_LowConvertToPublicKey(SECKEYLowPrivateKey *privk)
{
    SECKEYLowPublicKey *pubk;
    PRArenaPool *arena;


    arena = PORT_NewArena (DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
        PORT_SetError (SEC_ERROR_NO_MEMORY);
        return NULL;
    }

    switch(privk->keyType) {
      case rsaKey:
      case nullKey:
	pubk = (SECKEYLowPublicKey *)PORT_ArenaZAlloc(arena,
						sizeof (SECKEYLowPublicKey));
	if (pubk != NULL) {
	    SECStatus rv;

	    pubk->arena = arena;
	    pubk->keyType = privk->keyType;
	    if (privk->keyType == nullKey) return pubk;
	    rv = SECITEM_CopyItem(arena, &pubk->u.rsa.modulus,
				  &privk->u.rsa.modulus);
	    if (rv == SECSuccess) {
		rv = SECITEM_CopyItem (arena, &pubk->u.rsa.publicExponent,
				       &privk->u.rsa.publicExponent);
		if (rv == SECSuccess)
		    return pubk;
	    }
	    SECKEY_LowDestroyPublicKey (pubk);
	} else {
	    PORT_SetError (SEC_ERROR_NO_MEMORY);
	}
	break;
      case dsaKey:
	pubk = (SECKEYLowPublicKey *)PORT_ArenaZAlloc(arena,
						    sizeof(SECKEYLowPublicKey));
	if (pubk != NULL) {
	    SECStatus rv;

	    pubk->arena = arena;
	    pubk->keyType = privk->keyType;
	    rv = SECITEM_CopyItem(arena, &pubk->u.dsa.publicValue,
				  &privk->u.dsa.publicValue);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &pubk->u.dsa.params.prime,
				  &privk->u.dsa.params.prime);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &pubk->u.dsa.params.subPrime,
				  &privk->u.dsa.params.subPrime);
	    if (rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(arena, &pubk->u.dsa.params.base,
				  &privk->u.dsa.params.base);
	    if (rv == SECSuccess) return pubk;
	}
	break;
	/* XXXX Need DSA and DH cases */
	/* No Fortezza in Low Key implementations (Fortezza keys aren't
	 * stored in our data base */
    default:
	break;
    }

    PORT_FreeArena (arena, PR_FALSE);
    return NULL;
}

CERTSubjectPublicKeyInfo *
SECKEY_CreateSubjectPublicKeyInfo(SECKEYPublicKey *pubk)
{
    CERTSubjectPublicKeyInfo *spki;
    PRArenaPool *arena;
    SECItem params = { siBuffer, NULL, 0 };

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    spki = (CERTSubjectPublicKeyInfo *) PORT_ArenaZAlloc(arena, sizeof (*spki));
    if (spki != NULL) {
	SECStatus rv;
	SECItem *rv_item;

	switch(pubk->keyType) {
	  case rsaKey:
	    rv = SECOID_SetAlgorithmID(arena, &spki->algorithm,
				     SEC_OID_PKCS1_RSA_ENCRYPTION, 0);
	    if (rv == SECSuccess) {
		/*
		 * DER encode the public key into the subjectPublicKeyInfo.
		 */
		rv_item = SEC_ASN1EncodeItem(arena, &spki->subjectPublicKey,
					     pubk, SECKEY_RSAPublicKeyTemplate);
		if (rv_item != NULL) {
		    /*
		     * The stored value is supposed to be a BIT_STRING,
		     * so convert the length.
		     */
		    spki->subjectPublicKey.len <<= 3;
		    /*
		     * We got a good one; return it.
		     */
		    return spki;
		}
	    }
	    break;
	  case dsaKey:
	    /* DER encode the params. */
	    rv_item = SEC_ASN1EncodeItem(arena, &params, &pubk->u.dsa.params,
					 SECKEY_PQGParamsTemplate);
	    if (rv_item != NULL) {
		rv = SECOID_SetAlgorithmID(arena, &spki->algorithm,
					   SEC_OID_ANSIX9_DSA_SIGNATURE,
					   &params);
		if (rv == SECSuccess) {
		    /*
		     * DER encode the public key into the subjectPublicKeyInfo.
		     */
		    rv_item = SEC_ASN1EncodeItem(arena, &spki->subjectPublicKey,
						 pubk,
						 SECKEY_DSAPublicKeyTemplate);
		    if (rv_item != NULL) {
			/*
			 * The stored value is supposed to be a BIT_STRING,
			 * so convert the length.
			 */
			spki->subjectPublicKey.len <<= 3;
			/*
			 * We got a good one; return it.
			 */
			return spki;
		    }
		}
	    }
	    SECITEM_FreeItem(&params, PR_FALSE);
	    break;
	  case fortezzaKey:
#ifdef notdef
	    /* encode the DSS parameters (PQG) */
	    rv = FortezzaBuildParams(&params,pubk);
	    if (rv != SECSuccess) break;

	    /* set the algorithm */
	    rv = SECOID_SetAlgorithmID(arena, &spki->algorithm,
				       SEC_OID_MISSI_KEA_DSS, &params);
	    PORT_Free(params.data);
	    if (rv == SECSuccess) {
		/*
		 * Encode the public key into the subjectPublicKeyInfo.
		 * Fortezza key material is not standard DER
		 */
		rv = FortezzaEncodeCertKey(arena,&spki->subjectPublicKey,pubk);
		if (rv == SECSuccess) {
		    /*
		     * The stored value is supposed to be a BIT_STRING,
		     * so convert the length.
		     */
		    spki->subjectPublicKey.len <<= 3;

		    /*
		     * We got a good one; return it.
		     */
		    return spki;
		}
	    }
#endif
	    break;
	  default:
	    break;
	}
    } else {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}

void
SECKEY_DestroySubjectPublicKeyInfo(CERTSubjectPublicKeyInfo *spki)
{
    if (spki && spki->arena) {
	PORT_FreeArena(spki->arena, PR_FALSE);
    }
}

/*
 * XXX
 * this only works for RSA keys... need to do something
 * similiar to CERT_ExtractPublicKey for MISSI (Fortezza) Certificates
 */
SECKEYPublicKey *
SECKEY_DecodeDERPublicKey(SECItem *pubkder)
{
    PRArenaPool *arena;
    SECKEYPublicKey *pubk;
    SECStatus rv;

    arena = PORT_NewArena (DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    pubk = (SECKEYPublicKey *) PORT_ArenaZAlloc (arena, sizeof (SECKEYPublicKey));
    if (pubk != NULL) {
	pubk->arena = arena;
	pubk->pkcs11Slot = NULL;
	pubk->pkcs11ID = 0;
	rv = SEC_ASN1DecodeItem(arena, pubk, SECKEY_RSAPublicKeyTemplate,
				pubkder);
	if (rv == SECSuccess)
	    return pubk;
	SECKEY_DestroyPublicKey (pubk);
    } else {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena (arena, PR_FALSE);
    return NULL;
}

/*
 * Decode a base64 ascii encoded DER encoded public key.
 */
SECKEYPublicKey *
SECKEY_ConvertAndDecodePublicKey(char *pubkstr)
{
    SECKEYPublicKey *pubk;
    SECStatus rv;
    SECItem der;

    rv = ATOB_ConvertAsciiToItem (&der, pubkstr);
    if (rv != SECSuccess)
	return NULL;

    pubk = SECKEY_DecodeDERPublicKey (&der);

    PORT_Free (der.data);
    return pubk;
}

CERTSubjectPublicKeyInfo *
SECKEY_DecodeDERSubjectPublicKeyInfo(SECItem *spkider)
{
    PRArenaPool *arena;
    CERTSubjectPublicKeyInfo *spki;
    SECStatus rv;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    spki = (CERTSubjectPublicKeyInfo *)
		PORT_ArenaZAlloc(arena, sizeof (CERTSubjectPublicKeyInfo));
    if (spki != NULL) {
	spki->arena = arena;
	rv = DER_Decode(arena, spki, CERTSubjectPublicKeyInfoTemplate, spkider);
	if (rv == SECSuccess)
	    return spki;
	SECKEY_DestroySubjectPublicKeyInfo(spki);
    } else {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}

/*
 * Decode a base64 ascii encoded DER encoded subject public key info.
 */
CERTSubjectPublicKeyInfo *
SECKEY_ConvertAndDecodeSubjectPublicKeyInfo(char *spkistr)
{
    CERTSubjectPublicKeyInfo *spki;
    SECStatus rv;
    SECItem der;

    rv = ATOB_ConvertAsciiToItem(&der, spkistr);
    if (rv != SECSuccess)
	return NULL;

    spki = SECKEY_DecodeDERSubjectPublicKeyInfo(&der);

    PORT_Free(der.data);
    return spki;
}

/*
 * Decode a base64 ascii encoded DER encoded public key and challenge
 * Verify digital signature and make sure challenge matches
 */
CERTSubjectPublicKeyInfo *
SECKEY_ConvertAndDecodePublicKeyAndChallenge(char *pkacstr, char *challenge,
								void *wincx)
{
    CERTSubjectPublicKeyInfo *spki = NULL;
    CERTPublicKeyAndChallenge pkac;
    SECStatus rv;
    SECItem signedItem;
    PRArenaPool *arena = NULL;
    CERTSignedData sd;
    SECItem sig;
    SECKEYPublicKey *pubKey = NULL;
    int len;
    
    signedItem.data = NULL;
    
    /* convert the base64 encoded data to binary */
    rv = ATOB_ConvertAsciiToItem(&signedItem, pkacstr);
    if (rv != SECSuccess) {
	goto loser;
    }

    /* create an arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto loser;
    }

    /* decode the outer wrapping of signed data */
    rv = DER_Decode(arena, &sd, CERTSignedDataTemplate, &signedItem );
    if ( rv ) {
	goto loser;
    }

    /* decode the public key and challenge wrapper */
    rv = DER_Decode(arena, &pkac, CERTPublicKeyAndChallengeTemplate, &sd.data);
    if ( rv ) {
	goto loser;
    }

    /* decode the subject public key info */
    spki = SECKEY_DecodeDERSubjectPublicKeyInfo(&pkac.spki);
    if ( spki == NULL ) {
	goto loser;
    }
    
    /* get the public key */
    pubKey = CERT_ExtractPublicKey(spki);
    if ( pubKey == NULL ) {
	goto loser;
    }

    /* check the signature */
    sig = sd.signature;
    DER_ConvertBitString(&sig);
    rv = VFY_VerifyData(sd.data.data, sd.data.len, pubKey, &sig, wincx);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    /* check the challenge */
    if ( challenge ) {
	len = PORT_Strlen(challenge);
	/* length is right */
	if ( len != pkac.challenge.len ) {
	    goto loser;
	}
	/* actual data is right */
	if ( PORT_Memcmp(challenge, pkac.challenge.data, len) != 0 ) {
	    goto loser;
	}
    }
    goto done;

loser:
    /* make sure that we return null if we got an error */
    if ( spki ) {
	SECKEY_DestroySubjectPublicKeyInfo(spki);
    }
    spki = NULL;
    
done:
    if ( signedItem.data ) {
	PORT_Free(signedItem.data);
    }
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if ( spki ) {
	SECKEY_DestroySubjectPublicKeyInfo(spki);
    }
    if ( pubKey ) {
	SECKEY_DestroyPublicKey(pubKey);
    }
    
    return spki;
}

void
SECKEY_DestroyPrivateKeyInfo(SECKEYPrivateKeyInfo *pvk,
			     PRBool freeit)
{
    PRArenaPool *poolp;

    if(pvk != NULL) {
	if(pvk->arena) {
	    poolp = pvk->arena;
	    /* zero structure since PORT_FreeArena does not support
	     * this yet.
	     */
	    PORT_Memset(pvk->privateKey.data, 0, pvk->privateKey.len);
	    PORT_Memset((char *)pvk, 0, sizeof(*pvk));
	    if(freeit == PR_TRUE) {
		PORT_FreeArena(poolp, PR_TRUE);
	    } else {
		pvk->arena = poolp;
	    }
	} else {
	    SECITEM_ZfreeItem(&pvk->version, PR_FALSE);
	    SECITEM_ZfreeItem(&pvk->privateKey, PR_FALSE);
	    SECOID_DestroyAlgorithmID(&pvk->algorithm, PR_FALSE);
	    PORT_Memset((char *)pvk, 0, sizeof(pvk));
	    if(freeit == PR_TRUE) {
		PORT_Free(pvk);
	    }
	}
    }
}

void
SECKEY_DestroyEncryptedPrivateKeyInfo(SECKEYEncryptedPrivateKeyInfo *epki,
				      PRBool freeit)
{
    PRArenaPool *poolp;

    if(epki != NULL) {
	if(epki->arena) {
	    poolp = epki->arena;
	    /* zero structure since PORT_FreeArena does not support
	     * this yet.
	     */
	    PORT_Memset(epki->encryptedData.data, 0, epki->encryptedData.len);
	    PORT_Memset((char *)epki, 0, sizeof(*epki));
	    if(freeit == PR_TRUE) {
		PORT_FreeArena(poolp, PR_TRUE);
	    } else {
		epki->arena = poolp;
	    }
	} else {
	    SECITEM_ZfreeItem(&epki->encryptedData, PR_FALSE);
	    SECOID_DestroyAlgorithmID(&epki->algorithm, PR_FALSE);
	    PORT_Memset((char *)epki, 0, sizeof(epki));
	    if(freeit == PR_TRUE) {
		PORT_Free(epki);
	    }
	}
    }
}

SECStatus
SECKEY_CopyPrivateKeyInfo(PRArenaPool *poolp,
			  SECKEYPrivateKeyInfo *to,
			  SECKEYPrivateKeyInfo *from)
{
    SECStatus rv = SECFailure;

    if((to == NULL) || (from == NULL)) {
	return SECFailure;
    }

    rv = SECOID_CopyAlgorithmID(poolp, &to->algorithm, &from->algorithm);
    if(rv != SECSuccess) {
	return SECFailure;
    }
    rv = SECITEM_CopyItem(poolp, &to->privateKey, &from->privateKey);
    if(rv != SECSuccess) {
	return SECFailure;
    }
    rv = SECITEM_CopyItem(poolp, &to->version, &from->version);

    return rv;
}

SECStatus
SECKEY_CopyEncryptedPrivateKeyInfo(PRArenaPool *poolp, 
				   SECKEYEncryptedPrivateKeyInfo *to,
				   SECKEYEncryptedPrivateKeyInfo *from)
{
    SECStatus rv = SECFailure;

    if((to == NULL) || (from == NULL)) {
	return SECFailure;
    }

    rv = SECOID_CopyAlgorithmID(poolp, &to->algorithm, &from->algorithm);
    if(rv != SECSuccess) {
	return SECFailure;
    }
    rv = SECITEM_CopyItem(poolp, &to->encryptedData, &from->encryptedData);

    return rv;
}

SECKEYLowPrivateKey *
SECKEY_CopyLowPrivateKey(SECKEYLowPrivateKey *privKey)
{
    SECKEYLowPrivateKey *returnKey = NULL;
    SECStatus rv = SECFailure;
    PRArenaPool *poolp;

    if(!privKey) {
	return NULL;
    }

    poolp = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if(!poolp) {
	return NULL;
    }

    returnKey = PORT_ArenaZAlloc(poolp, sizeof(SECKEYLowPrivateKey));
    if(!returnKey) {
	rv = SECFailure;
	goto loser;
    }

    returnKey->keyType = privKey->keyType;
    returnKey->arena = poolp;

    switch(privKey->keyType) {
	case rsaKey:
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.modulus), 
	    				&(privKey->u.rsa.modulus));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.version), 
	    				&(privKey->u.rsa.version));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.publicExponent), 
	    				&(privKey->u.rsa.publicExponent));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.privateExponent), 
	    				&(privKey->u.rsa.privateExponent));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.prime[0]), 
	    				&(privKey->u.rsa.prime[0]));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.prime[1]), 
	    				&(privKey->u.rsa.prime[1]));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.primeExponent[0]), 
	    				&(privKey->u.rsa.primeExponent[0]));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.primeExponent[1]), 
	    				&(privKey->u.rsa.primeExponent[1]));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.rsa.coefficient), 
	    				&(privKey->u.rsa.coefficient));
	    if(rv != SECSuccess) break;
	    break;
	case dsaKey:
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.dsa.publicValue),
	    				&(privKey->u.dsa.publicValue));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.dsa.privateValue),
	    				&(privKey->u.dsa.privateValue));
	    if(rv != SECSuccess) break;
	    returnKey->u.dsa.params.arena = poolp;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.dsa.params.prime),
					&(privKey->u.dsa.params.prime));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.dsa.params.subPrime),
					&(privKey->u.dsa.params.subPrime));
	    if(rv != SECSuccess) break;
	    rv = SECITEM_CopyItem(poolp, &(returnKey->u.dsa.params.base),
					&(privKey->u.dsa.params.base));
	    if(rv != SECSuccess) break;
	    break;
	case fortezzaKey:
	    returnKey->u.fortezza.certificate = 
	    				privKey->u.fortezza.certificate;
	    returnKey->u.fortezza.socket = 
	    				privKey->u.fortezza.socket;
	    PORT_Memcpy(returnKey->u.fortezza.serial, 
	    				privKey->u.fortezza.serial, 8);
	    rv = SECSuccess;
	    break;
	default:
	    rv = SECFailure;
    }

loser:

    if(rv != SECSuccess) {
	PORT_FreeArena(poolp, PR_TRUE);
	returnKey = NULL;
    }

    return returnKey;
}
