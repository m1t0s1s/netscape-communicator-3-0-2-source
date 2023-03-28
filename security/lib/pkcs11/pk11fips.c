/*
 * This file implements PKCS 11 on top of our existing security modules
 *
 * For more information about PKCS 11 See PKCS 11 Token Inteface Standard.
 *   This implementation has two slots:
 *	slot 1 is our generic crypto support. It does not require login
 *   (unless you've enabled FIPS). It supports Public Key ops, and all they
 *   bulk ciphers and hashes. It can also support Private Key ops for imported
 *   Private keys. It does not have any token storage.
 *	slot 2 is our private key support. It requires a login before use. It
 *   can store Private Keys and Certs as token objects. Currently only private
 *   keys and their associated Certificates are saved on the token.
 *
 *   In this implementation, session objects are only visible to the session
 *   that created or generated them.
 */
#include "seccomon.h"
#include "crypto.h"     /* Required for pk11_fipsPowerUpSelfTest(). */
#include "key.h"
#include "pkcs11.h"
#include "pkcs11i.h"
#include "pk11func.h"

/* The next two strings must be exactly 64 characters long, with the
   first 32 characters meaningful  */
static char *slotDescription     = 
	"Netscape Internal FIPS-140 Cryptographic Services               ";
static char *privSlotDescription = 
	"Netscape FIPS-140 User Private Key Services                     ";


#define NETSCAPE_SLOT_ID 1
#define PRIVATE_KEY_SLOT_ID 2

/*
 * Configuration utils
 */
void
PK11_ConfigureFIPS(char *slotdes, char *pslotdes) 
{
    if (slotdes && (PORT_Strlen(slotdes) == 65)) {
	slotDescription = slotdes;
    }
    if (pslotdes && (PORT_Strlen(pslotdes) == 65)) {
	privSlotDescription = pslotdes;
    }
    return;
}

/*
 * ******************** Password Utilities *******************************
 */
static PRBool isLoggedIn = PR_FALSE;
static PRBool fatalError = PR_FALSE;

/* Fips required checks before any useful crypto graphic services */
static CK_RV pk11_fipsCheck(void) {
    if (isLoggedIn != PR_TRUE) 
	return CKR_USER_NOT_LOGGED_IN;
    if (fatalError) 
	return CKR_DEVICE_ERROR;
    return CKR_OK;
}


#define PK11_FIPSCHECK() \
    CK_RV rv; \
    if ((rv = pk11_fipsCheck()) != CKR_OK) return rv;

#define PK11_FIPSFATALCHECK() \
    if (fatalError) return CKR_DEVICE_ERROR;

#define __PASTE(x,y)	x##y


/* ------------- forward declare all the FIPS functions ------------- */
#undef CK_FUNC
#undef CK_EXTERN
#undef CK_NEED_ARG_LIST
#undef _CK_RV

#define CK_EXTERN 
#define CK_FUNC(name) CK_ENTRY __PASTE(F,name)
#define _CK_RV CK_RV
#define CK_NEED_ARG_LIST 1

#include "pkcs11f.h"

/* ------------- build the CK_CRYPTO_TABLE ------------------------- */
static CK_FUNCTION_LIST pk11_fipsTable = {
    { 1, 10 },

#undef CK_FUNC
#undef CK_EXTERN
#undef CK_NEED_ARG_LIST
#undef _CK_RV

#define CK_EXTERN 
#define CK_FUNC(name) __PASTE(F,name),
#define _CK_RV

#include "pkcs11f.h"

};

#undef CK_FUNC
#undef CK_EXTERN
#undef _CK_RV


#undef __PASTE


/**********************************************************************
 *
 *     Start of PKCS 11 functions 
 *
 **********************************************************************/
/* return the function list */
 CK_RV FC_GetFunctionList(CK_FUNCTION_LIST_PTR *pFunctionList) {
    *pFunctionList = &pk11_fipsTable;
    return CKR_OK;
}


/* C_Initialize initializes the Cryptoki library. */
  CK_RV FC_Initialize(CK_VOID_PTR pReserved) {
    CK_RV rv;


    rv = C_Initialize(pReserved);
    if (rv != CKR_OK) {
	fatalError = PR_TRUE;
	return rv;
    }
    fatalError = PR_FALSE; /* any error has been reset */

    rv = pk11_fipsPowerUpSelfTest();
    if (rv != CKR_OK) {
	fatalError = PR_TRUE;
	return rv;
    }

    SECKEY_SetDefaultKeyDBAlg(SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC);
    return CKR_OK;
}

/*C_Finalize indicates that an application is done with the Cryptoki library.*/
CK_RV FC_Finalize (CK_VOID_PTR pReserved) {
   return C_Finalize (pReserved);
}


/* C_GetInfo returns general information about Cryptoki. */
 CK_RV  FC_GetInfo(CK_INFO_PTR pInfo) {
    return C_GetInfo(pInfo);
}

/* C_GetSlotList obtains a list of slots in the system. */
 CK_RV FC_GetSlotList(CK_BBOOL tokenPresent,
	 		CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR pusCount) {
    return C_GetSlotList(tokenPresent,pSlotList,pusCount) ;
}
	
/* C_GetSlotInfo obtains information about a particular slot in the system. */
 CK_RV FC_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo) {

	CK_RV crv;

	crv = C_GetSlotInfo(slotID,pInfo);
	if (crv != CKR_OK) {
		return crv;
	}

    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	PORT_Memcpy(pInfo->slotDescription,slotDescription,64);
	pInfo->flags = CKF_TOKEN_PRESENT;
	return CKR_OK;
    case PRIVATE_KEY_SLOT_ID:
	PORT_Memcpy(pInfo->slotDescription,privSlotDescription,64);
	pInfo->flags = CKF_TOKEN_PRESENT;
	return CKR_OK;
    default:
	break;
    }
    return CKR_SLOT_ID_INVALID;
}


/* C_GetTokenInfo obtains information about a particular token in the system. */
 CK_RV FC_GetTokenInfo(CK_SLOT_ID slotID,CK_TOKEN_INFO_PTR pInfo) {
    CK_RV rv;

    rv = C_GetTokenInfo(slotID,pInfo);
    if (rv != CKR_OK) return rv;

    /* always turn on the need login flag */
    if (slotID == NETSCAPE_SLOT_ID) {
          pInfo->flags |=  CKF_USER_PIN_INITIALIZED;
    }
    pInfo->flags |=  CKF_LOGIN_REQUIRED;
    return CKR_OK;
}



/* C_GetMechanismList obtains a list of mechanism types supported by a token. */
 CK_RV FC_GetMechanismList(CK_SLOT_ID slotID,
	CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pusCount) {
    PK11_FIPSFATALCHECK();
    return C_GetMechanismList(slotID,pMechanismList,pusCount);
}


/* C_GetMechanismInfo obtains information about a particular mechanism 
 * possibly supported by a token. */
 CK_RV FC_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type,
    					CK_MECHANISM_INFO_PTR pInfo) {
    PK11_FIPSFATALCHECK();
    return C_GetMechanismInfo(slotID,type,pInfo);
}


/* C_InitToken initializes a token. */
 CK_RV FC_InitToken(CK_SLOT_ID slotID,CK_CHAR_PTR pPin,
 				CK_ULONG usPinLen,CK_CHAR_PTR pLabel) {
    return CKR_HOST_MEMORY; /*is this the right function for not implemented*/
}


/* C_InitPIN initializes the normal user's PIN. */
 CK_RV FC_InitPIN(CK_SESSION_HANDLE hSession,
    					CK_CHAR_PTR pPin, CK_ULONG usPinLen) {
    return CKR_USER_NOT_LOGGED_IN;
}


/* C_SetPIN modifies the PIN of user that is currently logged in. */
/* NOTE: This is only valid for the PRIVATE_KEY_SLOT */
 CK_RV FC_SetPIN(CK_SESSION_HANDLE hSession, CK_CHAR_PTR pOldPin,
    CK_ULONG usOldLen, CK_CHAR_PTR pNewPin, CK_ULONG usNewLen) {
    CK_RV rv;
    if ((rv = pk11_fipsCheck()) != CKR_OK) return rv;
    return C_SetPIN(hSession,pOldPin,usOldLen,pNewPin,usNewLen);
}

/* C_OpenSession opens a session between an application and a token. */
 CK_RV FC_OpenSession(CK_SLOT_ID slotID, CK_FLAGS flags,
   CK_VOID_PTR pApplication,CK_NOTIFY Notify,CK_SESSION_HANDLE_PTR phSession) {
    PK11_FIPSFATALCHECK();
    return C_OpenSession(slotID,flags,pApplication,Notify,phSession);
}


/* C_CloseSession closes a session between an application and a token. */
 CK_RV FC_CloseSession(CK_SESSION_HANDLE hSession) {
    return C_CloseSession(hSession);
}


/* C_CloseAllSessions closes all sessions with a token. */
 CK_RV FC_CloseAllSessions (CK_SLOT_ID slotID) {
    return C_CloseAllSessions (slotID);
}


/* C_GetSessionInfo obtains information about the session. */
 CK_RV FC_GetSessionInfo(CK_SESSION_HANDLE hSession,
						CK_SESSION_INFO_PTR pInfo) {
    CK_RV rv;
    PK11_FIPSFATALCHECK();

    rv = C_GetSessionInfo(hSession,pInfo);
    if (rv == CKR_OK) {
	if ((isLoggedIn) && (pInfo->state == CKS_RO_PUBLIC_SESSION)) {
		pInfo->state = CKS_RO_USER_FUNCTIONS;
	}
	if ((isLoggedIn) && (pInfo->state == CKS_RW_PUBLIC_SESSION)) {
		pInfo->state = CKS_RW_USER_FUNCTIONS;
	}
    }
    return rv;
}

/* C_Login logs a user into a token. */
 CK_RV FC_Login(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType,
				    CK_CHAR_PTR pPin, CK_ULONG usPinLen) {
    CK_RV rv;
    PK11_TOSLOT2(hSession);
    PK11_FIPSFATALCHECK();
    rv = C_Login(hSession,userType,pPin,usPinLen);
    if (rv == CKR_OK)
	isLoggedIn = PR_TRUE;
    else if (rv == CKR_USER_ALREADY_LOGGED_IN)
    {
	isLoggedIn = PR_TRUE;

	/* Provide FIPS PUB 140-1 power-up self-tests on demand. */
	rv = pk11_fipsPowerUpSelfTest();
	if (rv == CKR_OK)
		return CKR_USER_ALREADY_LOGGED_IN;
	else
		fatalError = PR_TRUE;
    }
    return rv;
}

/* C_Logout logs a user out from a token. */
 CK_RV FC_Logout(CK_SESSION_HANDLE hSession) {
    PK11_FIPSCHECK();
    PK11_TOSLOT2(hSession);
 
    rv = C_Logout(hSession);
    isLoggedIn = PR_FALSE;
    return rv;
}


/* C_CreateObject creates a new object. */
 CK_RV FC_CreateObject(CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usCount, 
					CK_OBJECT_HANDLE_PTR phObject) {
    PK11_FIPSCHECK();
    return C_CreateObject(hSession,pTemplate,usCount,phObject);
}





/* C_CopyObject copies an object, creating a new object for the copy. */
 CK_RV FC_CopyObject(CK_SESSION_HANDLE hSession,
       CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usCount,
					CK_OBJECT_HANDLE_PTR phNewObject) {
    PK11_FIPSCHECK();
    return C_CopyObject(hSession,hObject,pTemplate,usCount,phNewObject);
}


/* C_DestroyObject destroys an object. */
 CK_RV FC_DestroyObject(CK_SESSION_HANDLE hSession,
		 				CK_OBJECT_HANDLE hObject) {
    PK11_FIPSCHECK();
    return C_DestroyObject(hSession,hObject);
}


/* C_GetObjectSize gets the size of an object in bytes. */
 CK_RV FC_GetObjectSize(CK_SESSION_HANDLE hSession,
    			CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pusSize) {
    PK11_FIPSCHECK();
    return C_GetObjectSize(hSession, hObject, pusSize);
}


/* C_GetAttributeValue obtains the value of one or more object attributes. */
 CK_RV FC_GetAttributeValue(CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    PK11_FIPSCHECK();
    return C_GetAttributeValue(hSession,hObject,pTemplate,usCount);
}


/* C_SetAttributeValue modifies the value of one or more object attributes */
 CK_RV FC_SetAttributeValue (CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    PK11_FIPSCHECK();
    return C_SetAttributeValue(hSession,hObject,pTemplate,usCount);
}



/* C_FindObjectsInit initializes a search for token and session objects 
 * that match a template. */
 CK_RV FC_FindObjectsInit(CK_SESSION_HANDLE hSession,
    			CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    PK11_FIPSCHECK();
    return C_FindObjectsInit(hSession,pTemplate,usCount);
}


/* C_FindObjects continues a search for token and session objects 
 * that match a template, obtaining additional object handles. */
 CK_RV FC_FindObjects(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE_PTR phObject,CK_ULONG usMaxObjectCount,
    					CK_ULONG_PTR pusObjectCount) {
    PK11_FIPSCHECK();
    return C_FindObjects(hSession,phObject,usMaxObjectCount,
    							pusObjectCount);
}


/*
 ************** Crypto Functions:     Encrypt ************************
 */

/* C_EncryptInit initializes an encryption operation. */
 CK_RV FC_EncryptInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_EncryptInit(hSession,pMechanism,hKey);
}

/* C_Encrypt encrypts single-part data. */
 CK_RV FC_Encrypt (CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    		CK_ULONG usDataLen, CK_BYTE_PTR pEncryptedData,
					 CK_ULONG_PTR pusEncryptedDataLen) {
    PK11_FIPSCHECK();
    return C_Encrypt(hSession,pData,usDataLen,pEncryptedData,
							pusEncryptedDataLen);
}


/* C_EncryptUpdate continues a multiple-part encryption operation. */
 CK_RV FC_EncryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart, CK_ULONG usPartLen, CK_BYTE_PTR pEncryptedPart,	
					CK_ULONG_PTR pusEncryptedPartLen) {
    PK11_FIPSCHECK();
    return C_EncryptUpdate(hSession,pPart,usPartLen,pEncryptedPart,
						pusEncryptedPartLen);
}


/* C_EncryptFinal finishes a multiple-part encryption operation. */
 CK_RV FC_EncryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastEncryptedPart, CK_ULONG_PTR pusLastEncryptedPartLen) {

    PK11_FIPSCHECK();
    return C_EncryptFinal(hSession,pLastEncryptedPart,pusLastEncryptedPartLen);
}

/*
 ************** Crypto Functions:     Decrypt ************************
 */


/* C_DecryptInit initializes a decryption operation. */
 CK_RV FC_DecryptInit( CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_DecryptInit(hSession,pMechanism,hKey);
}

/* C_Decrypt decrypts encrypted data in a single part. */
 CK_RV FC_Decrypt(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedData,CK_ULONG usEncryptedDataLen,CK_BYTE_PTR pData,
    						CK_ULONG_PTR pusDataLen) {
    PK11_FIPSCHECK();
    return C_Decrypt(hSession,pEncryptedData,usEncryptedDataLen,pData,
    								pusDataLen);
}


/* C_DecryptUpdate continues a multiple-part decryption operation. */
 CK_RV FC_DecryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart, CK_ULONG usEncryptedPartLen,
    				CK_BYTE_PTR pPart, CK_ULONG_PTR pusPartLen) {
    PK11_FIPSCHECK();
    return C_DecryptUpdate(hSession,pEncryptedPart,usEncryptedPartLen,
    							pPart,pusPartLen);
}


/* C_DecryptFinal finishes a multiple-part decryption operation. */
 CK_RV FC_DecryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastPart, CK_ULONG_PTR pusLastPartLen) {
    PK11_FIPSCHECK();
    return C_DecryptFinal(hSession,pLastPart,pusLastPartLen);
}


/*
 ************** Crypto Functions:     Digest (HASH)  ************************
 */

/* C_DigestInit initializes a message-digesting operation. */
 CK_RV FC_DigestInit(CK_SESSION_HANDLE hSession,
    					CK_MECHANISM_PTR pMechanism) {
    PK11_FIPSFATALCHECK();
    return C_DigestInit(hSession, pMechanism);
}


/* C_Digest digests data in a single part. */
 CK_RV FC_Digest(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData, CK_ULONG usDataLen, CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pusDigestLen) {
    PK11_FIPSFATALCHECK();
    return C_Digest(hSession,pData,usDataLen,pDigest,pusDigestLen);
}


/* C_DigestUpdate continues a multiple-part message-digesting operation. */
 CK_RV FC_DigestUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
					    CK_ULONG usPartLen) {
    PK11_FIPSFATALCHECK();
    return C_DigestUpdate(hSession,pPart,usPartLen);
}


/* C_DigestFinal finishes a multiple-part message-digesting operation. */
 CK_RV FC_DigestFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pusDigestLen) {
    PK11_FIPSFATALCHECK();
    return C_DigestFinal(hSession,pDigest,pusDigestLen);
}


/*
 ************** Crypto Functions:     Sign  ************************
 */

/* C_SignInit initializes a signature (private key encryption) operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_SignInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_SignInit(hSession,pMechanism,hKey);
}


/* C_Sign signs (encrypts with private key) data in a single part,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_Sign(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,CK_ULONG usDataLen,CK_BYTE_PTR pSignature,
    					CK_ULONG_PTR pusSignatureLen) {
    PK11_FIPSCHECK();
    return C_Sign(hSession,pData,usDataLen,pSignature,pusSignatureLen);
}


/* C_SignUpdate continues a multiple-part signature operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_SignUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
    							CK_ULONG usPartLen) {
    PK11_FIPSCHECK();
    return C_SignUpdate(hSession,pPart,usPartLen);
}


/* C_SignFinal finishes a multiple-part signature operation, 
 * returning the signature. */
 CK_RV FC_SignFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pSignature,
					    CK_ULONG_PTR pusSignatureLen) {
    PK11_FIPSCHECK();
    return C_SignFinal(hSession,pSignature,pusSignatureLen);
}

/*
 ************** Crypto Functions:     Sign Recover  ************************
 */
/* C_SignRecoverInit initializes a signature operation,
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
 CK_RV FC_SignRecoverInit(CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_SignRecoverInit(hSession,pMechanism,hKey);
}


/* C_SignRecover signs data in a single operation
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
 CK_RV FC_SignRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
  CK_ULONG usDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pusSignatureLen) {
    PK11_FIPSCHECK();
    return C_SignRecover(hSession,pData,usDataLen,pSignature,pusSignatureLen);
}

/*
 ************** Crypto Functions:     verify  ************************
 */

/* C_VerifyInit initializes a verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature (e.g. DSA) */
 CK_RV FC_VerifyInit(CK_SESSION_HANDLE hSession,
			   CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_VerifyInit(hSession,pMechanism,hKey);
}


/* C_Verify verifies a signature in a single-part operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_Verify(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    CK_ULONG usDataLen, CK_BYTE_PTR pSignature, CK_ULONG usSignatureLen) {
    /* make sure we're legal */
    PK11_FIPSCHECK();
    return C_Verify(hSession,pData,usDataLen,pSignature,usSignatureLen);
}


/* C_VerifyUpdate continues a multiple-part verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_VerifyUpdate( CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart,
						CK_ULONG usPartLen) {
    PK11_FIPSCHECK();
    return C_VerifyUpdate(hSession,pPart,usPartLen);
}


/* C_VerifyFinal finishes a multiple-part verification operation, 
 * checking the signature. */
 CK_RV FC_VerifyFinal(CK_SESSION_HANDLE hSession,
			CK_BYTE_PTR pSignature,CK_ULONG usSignatureLen) {
    PK11_FIPSCHECK();
    return C_VerifyFinal(hSession,pSignature,usSignatureLen);
}

/*
 ************** Crypto Functions:     Verify  Recover ************************
 */

/* C_VerifyRecoverInit initializes a signature verification operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
 CK_RV FC_VerifyRecoverInit(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_VerifyRecoverInit(hSession,pMechanism,hKey);
}


/* C_VerifyRecover verifies a signature in a single-part operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
 CK_RV FC_VerifyRecover(CK_SESSION_HANDLE hSession,
		 CK_BYTE_PTR pSignature,CK_ULONG usSignatureLen,
    				CK_BYTE_PTR pData,CK_ULONG_PTR pusDataLen) {
    PK11_FIPSCHECK();
    return C_VerifyRecover(hSession,pSignature,usSignatureLen,pData,
								pusDataLen);
}

/*
 **************************** Key Functions:  ************************
 */

/* C_GenerateKey generates a secret key, creating a new key object. */
 CK_RV FC_GenerateKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount,
    						CK_OBJECT_HANDLE_PTR phKey) {
    PK11_FIPSCHECK();
    return C_GenerateKey(hSession,pMechanism,pTemplate,usCount,phKey);
}


/* C_GenerateKeyPair generates a public-key/private-key pair, 
 * creating new key objects. */
 CK_RV FC_GenerateKeyPair (CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG usPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG usPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPrivateKey,
    					CK_OBJECT_HANDLE_PTR phPublicKey) {
    PK11_FIPSCHECK();
    return C_GenerateKeyPair (hSession,pMechanism,pPublicKeyTemplate,
    		usPublicKeyAttributeCount,pPrivateKeyTemplate,
		usPrivateKeyAttributeCount,phPrivateKey,phPublicKey);
}


/* C_WrapKey wraps (i.e., encrypts) a key. */
 CK_RV FC_WrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey,
					 CK_ULONG_PTR pusWrappedKeyLen) {
    PK11_FIPSCHECK();
    return C_WrapKey(hSession,pMechanism,hWrappingKey,hKey,pWrappedKey,
							pusWrappedKeyLen);
}


/* C_UnwrapKey unwraps (decrypts) a wrapped key, creating a new key object. */
 CK_RV FC_UnwrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey, CK_ULONG usWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usAttributeCount,
						 CK_OBJECT_HANDLE_PTR phKey) {
    PK11_FIPSCHECK();
    return C_UnwrapKey(hSession,pMechanism,hUnwrappingKey,pWrappedKey,
			usWrappedKeyLen,pTemplate,usAttributeCount,phKey);
}


/* C_DeriveKey derives a key from a base key, creating a new key object. */
 CK_RV FC_DeriveKey( CK_SESSION_HANDLE hSession,
	 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey,
	 CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usAttributeCount, 
						CK_OBJECT_HANDLE_PTR phKey) {
    PK11_FIPSCHECK();
    return C_DeriveKey(hSession,pMechanism,hBaseKey,pTemplate,usAttributeCount, 
								phKey);
}

/*
 **************************** Radom Functions:  ************************
 */

/* C_SeedRandom mixes additional seed material into the token's random number 
 * generator. */
 CK_RV FC_SeedRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed,
    CK_ULONG usSeedLen) {

    PK11_FIPSFATALCHECK();
    return C_SeedRandom(hSession,pSeed,usSeedLen);
}


/* C_GenerateRandom generates random data. */
 CK_RV FC_GenerateRandom(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR	pRandomData, CK_ULONG usRandomLen) {
    PK11_FIPSFATALCHECK();
    return C_GenerateRandom(hSession,pRandomData,usRandomLen);
}


/* C_GetFunctionStatus obtains an updated status of a function running 
 * in parallel with an application. */
 CK_RV FC_GetFunctionStatus(CK_SESSION_HANDLE hSession) {
    PK11_FIPSCHECK();
    return C_GetFunctionStatus(hSession);
}


/* C_CancelFunction cancels a function running in parallel */
 CK_RV FC_CancelFunction(CK_SESSION_HANDLE hSession) {
    PK11_FIPSCHECK();
    return C_CancelFunction(hSession);
}

/*
 ****************************  Version 1.1 Functions:  ************************
 */

/* C_GetOperationState saves the state of the cryptographic 
 *operation in a session. */
CK_RV FC_GetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG_PTR pulOperationStateLen) {
    PK11_FIPSFATALCHECK();
    return C_GetOperationState(hSession,pOperationState,pulOperationStateLen);
}


/* C_SetOperationState restores the state of the cryptographic operation 
 * in a session. */
CK_RV FC_SetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG  ulOperationStateLen,
        CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey) {
    PK11_FIPSFATALCHECK();
    return C_SetOperationState(hSession,pOperationState,ulOperationStateLen,
        				hEncryptionKey,hAuthenticationKey);
}

/* C_FindObjectsFinal finishes a search for token and session objects. */
CK_RV FC_FindObjectsFinal(CK_SESSION_HANDLE hSession) {
    PK11_FIPSCHECK();
    return C_FindObjectsFinal(hSession);
}


/* Dual-function cryptographic operations */

/* C_DigestEncryptUpdate continues a multiple-part digesting and encryption 
 * operation. */
CK_RV FC_DigestEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
    CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen) {
    PK11_FIPSCHECK();
    return C_DigestEncryptUpdate(hSession,pPart,ulPartLen,pEncryptedPart,
					 pulEncryptedPartLen);
}


/* C_DecryptDigestUpdate continues a multiple-part decryption and digesting 
 * operation. */
CK_RV FC_DecryptDigestUpdate(CK_SESSION_HANDLE hSession,
     CK_BYTE_PTR  pEncryptedPart, CK_ULONG  ulEncryptedPartLen,
    				CK_BYTE_PTR  pPart, CK_ULONG_PTR pulPartLen) {

    PK11_FIPSCHECK();
    return C_DecryptDigestUpdate(hSession, pEncryptedPart,ulEncryptedPartLen,
    				pPart,pulPartLen);
}

/* C_SignEncryptUpdate continues a multiple-part signing and encryption 
 * operation. */
CK_RV FC_SignEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
	 CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen) {

    PK11_FIPSCHECK();
    return C_SignEncryptUpdate(hSession,pPart,ulPartLen,pEncryptedPart,
					 pulEncryptedPartLen);
}

/* C_DecryptVerifyUpdate continues a multiple-part decryption and verify 
 * operation. */
CK_RV FC_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pEncryptedData, CK_ULONG  ulEncryptedDataLen, 
				CK_BYTE_PTR  pData, CK_ULONG_PTR pulDataLen) {

    PK11_FIPSCHECK();
    return C_DecryptVerifyUpdate(hSession,pEncryptedData,ulEncryptedDataLen, 
				pData,pulDataLen);
}


/* C_DigestKey continues a multi-part message-digesting operation,
 * by digesting the value of a secret key as part of the data already digested.
 */
CK_RV FC_DigestKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey) {
    PK11_FIPSCHECK();
    return C_DigestKey(hSession,hKey);
}


