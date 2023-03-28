
/* C_Initialize initializes the Cryptoki library. */
CK_EXTERN CK_RV CK_FUNC(C_Initialize)(
    CK_VOID_PTR 	pReserved	/* Reserved.  Should be NULL_PTR. */
);

/* C_GetInfo returns general information about Cryptoki. */
CK_EXTERN CK_RV CK_FUNC(C_GetInfo)(
    CK_INFO_PTR		pInfo	/* location that receives the information. */
);


/* C_GetSlotList obtains a list of slots in the system. */
CK_EXTERN CK_RV CK_FUNC(C_GetSlotList)(
    CK_BBOOL		tokenPresent,	/* only slots with token present */
    CK_SLOT_ID_PTR	pSlotList,	/* receives the array of slot IDs */
    CK_USHORT_PTR	pusCount	/* receives the number of slots. */
);


/* C_GetSlotInfo obtains information about a particular slot in the system. */
CK_EXTERN CK_RV CK_FUNC(C_GetSlotInfo)(
    CK_SLOT_ID 		slotID,		/* the ID of the slot */
    CK_SLOT_INFO_PTR 	pInfo		/* receives the slot information. */
);


/* C_GetTokenInfo obtains information about a particular token in the system. */
CK_EXTERN CK_RV CK_FUNC(C_GetTokenInfo)(
    CK_SLOT_ID		slotID,		/* ID of the token's slot */
    CK_TOKEN_INFO_PTR	pInfo		/* receives the token information. */
);


/* C_GetMechanismList obtains a list of mechanism types supported by a token. */
CK_EXTERN CK_RV CK_FUNC(C_GetMechanismList)(
    CK_SLOT_ID		slotID,		/* ID of the token's slot */
    CK_MECHANISM_TYPE_PTR pMechanismList, /* receives mech. types array */
    CK_USHORT_PTR	pusCount	/* receives number of mechs. */
);


/* C_GetMechanismInfo obtains information about a particular mechanism 
 * possibly supported by a token. */
CK_EXTERN CK_RV CK_FUNC(C_GetMechanismInfo)(
    CK_SLOT_ID		slotID,		/* ID of the token's slot */
    CK_MECHANISM_TYPE	type,		/* type of mechanism */
    CK_MECHANISM_INFO_PTR pInfo		/* receives mechanism information */
);


/* C_InitToken initializes a token. */
CK_EXTERN CK_RV CK_FUNC(C_InitToken)(
    CK_SLOT_ID		slotID,		/* ID of the token's slot */
    CK_CHAR_PTR		pPin,		/* the SO's initial PIN */
    CK_USHORT		usPinLen,	/* length in bytes of the PIN */
    CK_CHAR_PTR		pLabel		/* 32-byte token label (blank padded) */
);


/* C_InitPIN initializes the normal user's PIN. */
CK_EXTERN CK_RV CK_FUNC(C_InitPIN)(
    CK_SESSION_HANDLE	hSession, 	/* the session's handle */
    CK_CHAR_PTR		pPin,	  	/* the normal user's PIN */
    CK_USHORT		usPinLen  	/* length in bytes of the PIN. */
);


/* C_SetPIN modifies the PIN of user that is currently logged in. */
CK_EXTERN CK_RV CK_FUNC(C_SetPIN)(
    CK_SESSION_HANDLE	hSession, 	/* the session's handle */
    CK_CHAR_PTR		pOldPin,  	/* the old PIN 	   */
    CK_USHORT		usOldLen, 	/* length of the old PIN */
    CK_CHAR_PTR		pNewPin,  	/* the new PIN 	   */
    CK_USHORT		usNewLen  	/* length of the new PIN. */
);

/* C_OpenSession opens a session between an application and a token. */
CK_EXTERN CK_RV CK_FUNC(C_OpenSession)(
    CK_SLOT_ID		slotID,		/* the slot's ID */
    CK_FLAGS		flags,		/* defined in CK_SESSION_INFO */
    CK_VOID_PTR		pApplication,	/* pointer passed to callback */
    CK_NOTIFY		Notify,		/* notification callback function */
    CK_SESSION_HANDLE_PTR phSession	/* receives new session handle */
);


/* C_CloseSession closes a session between an application and a token. */
CK_EXTERN CK_RV CK_FUNC(C_CloseSession)(
    CK_SESSION_HANDLE	hSession 	/* the session's handle. */
);


/* C_CloseAllSessions closes all sessions with a token. */
CK_EXTERN CK_RV CK_FUNC(C_CloseAllSessions)(
    CK_SLOT_ID		slotID		/* the token's slot. */
);


/* C_GetSessionInfo obtains information about the session. */
CK_EXTERN CK_RV CK_FUNC(C_GetSessionInfo)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_SESSION_INFO_PTR	pInfo		/* receives session information. */
);

/* C_Login logs a user into a token. */
CK_EXTERN CK_RV CK_FUNC(C_Login)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_USER_TYPE	userType,	/* the user type */
    CK_CHAR_PTR		pPin,		/* the user's PIN */
    CK_USHORT		usPinLen	/* the length of the PIN. */
);


/* C_Logout logs a user out from a token. */
CK_EXTERN CK_RV CK_FUNC(C_Logout)(
    CK_SESSION_HANDLE	hSession	/* the session's handle. */
);


/* C_CreateObject creates a new object. */
CK_EXTERN CK_RV CK_FUNC(C_CreateObject)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_ATTRIBUTE_PTR	pTemplate, 	/* the object's template */
    CK_USHORT		usCount,	/* attributes in template */
    CK_OBJECT_HANDLE_PTR phObject	/* receives new object's handle. */
);


/* C_CopyObject copies an object, creating a new object for the copy. */
CK_EXTERN CK_RV CK_FUNC(C_CopyObject)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_OBJECT_HANDLE	hObject,	/* the object's handle */
    CK_ATTRIBUTE_PTR	pTemplate,	/* template for new object */
    CK_USHORT		usCount,	/* attributes in template */
    CK_OBJECT_HANDLE_PTR phNewObject	/* receives handle for copy of object.*/
);


/* C_DestroyObject destroys an object. */
CK_EXTERN CK_RV CK_FUNC(C_DestroyObject)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_OBJECT_HANDLE	hObject		/* the object's handle.	 */
);


/* C_GetObjectSize gets the size of an object in bytes. */
CK_EXTERN CK_RV CK_FUNC(C_GetObjectSize)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_OBJECT_HANDLE	hObject,	/* the object's handle */
    CK_USHORT_PTR	pusSize		/* receives size of object. */
);


/* C_GetAttributeValue obtains the value of one or more object attributes. */
CK_EXTERN CK_RV CK_FUNC(C_GetAttributeValue)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_OBJECT_HANDLE	hObject,	/* the object's handle */
    CK_ATTRIBUTE_PTR	pTemplate,	/* specifies attributes, gets values */
    CK_USHORT		usCount		/* attributes in template. */
);


/* C_SetAttributeValue modifies the value of one or more object attributes */
CK_EXTERN CK_RV CK_FUNC(C_SetAttributeValue)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle  */
    CK_OBJECT_HANDLE	hObject,	/* the object's handle  */
    CK_ATTRIBUTE_PTR	pTemplate,	/* specifies attributes and values  */
    CK_USHORT		usCount 	/* attributes in template. */
);


/* C_FindObjectsInit initializes a search for token and session objects 
 * that match a template. */
CK_EXTERN CK_RV CK_FUNC(C_FindObjectsInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_ATTRIBUTE_PTR	pTemplate,	/* attribute values to match */
    CK_USHORT		usCount		/* attributes in search template. */
);


/* C_FindObjects continues a search for token and session objects 
 * that match a template, obtaining additional object handles. */
CK_EXTERN CK_RV CK_FUNC(C_FindObjects)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_OBJECT_HANDLE_PTR phObject,	/* receives object handle array */
    CK_USHORT		usMaxObjectCount, /* max handles to be returned */
    CK_USHORT_PTR	pusObjectCount	/* actual number returned */
);


/* C_EncryptInit initializes an encryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_EncryptInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the encryption mechanism */
    CK_OBJECT_HANDLE	hKey		/* handle of encryption key. */
);


/* C_Encrypt encrypts single-part data. */
CK_EXTERN CK_RV CK_FUNC(C_Encrypt)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pData,		/* the plaintext data */
    CK_USHORT		usDataLen,	/* bytes of plaintext data */
    CK_BYTE_PTR		pEncryptedData,	/* receives encrypted data */
    CK_USHORT_PTR	pusEncryptedDataLen /* receives encrypted byte count */
);


/* C_EncryptUpdate continues a multiple-part encryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_EncryptUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pPart,		/* the plaintext data */
    CK_USHORT		usPartLen,	/* bytes of plaintext data */
    CK_BYTE_PTR		pEncryptedPart,	/* receives encrypted data */
    CK_USHORT_PTR	pusEncryptedPartLen /* receives encrypted byte count */
);


/* C_EncryptFinal finishes a multiple-part encryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_EncryptFinal)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pLastEncryptedPart, /* receives encrypted last part */
    CK_USHORT_PTR	pusLastEncryptedPartLen /* receives byte count */
);


/* C_DecryptInit initializes a decryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_DecryptInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the decryption mechanism */
    CK_OBJECT_HANDLE	hKey		/* handle of the decryption key */
);


/* C_Decrypt decrypts encrypted data in a single part. */
CK_EXTERN CK_RV CK_FUNC(C_Decrypt)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pEncryptedData, /* input encrypted data */
    CK_USHORT		usEncryptedDataLen, /* count of bytes of input */
    CK_BYTE_PTR		pData,		/* receives decrypted output */
    CK_USHORT_PTR	pusDataLen	/* receives decrypted byte count */
);


/* C_DecryptUpdate continues a multiple-part decryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_DecryptUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pEncryptedPart,	/* input encrypted data */
    CK_USHORT		usEncryptedPartLen, /* count of bytes of input */
    CK_BYTE_PTR		pPart,		/* receives decrypted output */
    CK_USHORT_PTR	pusPartLen	/* receives decrypted byte count */
);


/* C_DecryptFinal finishes a multiple-part decryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_DecryptFinal)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pLastPart,	/* receives decrypted output */
    CK_USHORT_PTR	pusLastPartLen	/* receives decrypted byte count */
);



/* C_DigestInit initializes a message-digesting operation. */
CK_EXTERN CK_RV CK_FUNC(C_DigestInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism	/* the digesting mechanism */
);


/* C_Digest digests data in a single part. */
CK_EXTERN CK_RV CK_FUNC(C_Digest)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pData,		/* data to be digested */
    CK_USHORT		usDataLen,	/* bytes of data to be digested */
    CK_BYTE_PTR		pDigest,	/* receives the message digest */
    CK_USHORT_PTR	pusDigestLen	/* receives byte length of digest. */
);


/* C_DigestUpdate continues a multiple-part message-digesting operation. */
CK_EXTERN CK_RV CK_FUNC(C_DigestUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pPart,		/* data to be digested */
    CK_USHORT		usPartLen	/* bytes of data to be digested */
);


/* C_DigestFinal finishes a multiple-part message-digesting operation. */
CK_EXTERN CK_RV CK_FUNC(C_DigestFinal)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pDigest,	/* receives the message digest */
    CK_USHORT_PTR	pusDigestLen	/* receives byte count of digest. */
);


/* C_SignInit initializes a signature (private key encryption) operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_EXTERN CK_RV CK_FUNC(C_SignInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the signature mechanism */
    CK_OBJECT_HANDLE	hKey		/* handle of the signature key */
);


/* C_Sign signs (encrypts with private key) data in a single part,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_EXTERN CK_RV CK_FUNC(C_Sign)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pData,		/* the data (digest) to be signed */
    CK_USHORT		usDataLen,	/* count of bytes to be signed */
    CK_BYTE_PTR		pSignature,	/* receives the signature */
    CK_USHORT_PTR	pusSignatureLen /* receives byte count of signature */
);


/* C_SignUpdate continues a multiple-part signature operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_EXTERN CK_RV CK_FUNC(C_SignUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pPart,		/* the data (digest) to be signed */
    CK_USHORT		usPartLen	/* count of bytes to be signed */
);


/* C_SignFinal finishes a multiple-part signature operation, 
 * returning the signature. */
CK_EXTERN CK_RV CK_FUNC(C_SignFinal)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pSignature,	/* receives the signature */
    CK_USHORT_PTR	pusSignatureLen /* receives byte count of signature */
);


/* C_SignRecoverInit initializes a signature operation,
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
CK_EXTERN CK_RV CK_FUNC(C_SignRecoverInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the signature mechanism */
    CK_OBJECT_HANDLE	hKey		/* handle of the signature key. */
);


/* C_SignRecover signs data in a single operation
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
CK_EXTERN CK_RV CK_FUNC(C_SignRecover)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pData,		/* the data (digest) to be signed */
    CK_USHORT		usDataLen,	/* count of bytes to be signed */
    CK_BYTE_PTR		pSignature,	/* receives the signature */
    CK_USHORT_PTR	pusSignatureLen /* receives byte count of signature */
);


/* C_VerifyInit initializes a verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature (e.g. DSA) */
CK_EXTERN CK_RV CK_FUNC(C_VerifyInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the verification mechanism */
    CK_OBJECT_HANDLE	hKey		/* handle of the verification key */ 
);


/* C_Verify verifies a signature in a single-part operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_EXTERN CK_RV CK_FUNC(C_Verify)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pData,		/* plaintext data (digest) to compare */
    CK_USHORT		usDataLen,	/* length of data (digest) in bytes */
    CK_BYTE_PTR		pSignature,	/* the signature to be verified */
    CK_USHORT		usSignatureLen	/* count of bytes of signature */
);


/* C_VerifyUpdate continues a multiple-part verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_EXTERN CK_RV CK_FUNC(C_VerifyUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pPart,		/* plaintext data (digest) to compare */
    CK_USHORT		usPartLen	/* length of data (digest) in bytes */
);


/* C_VerifyFinal finishes a multiple-part verification operation, 
 * checking the signature. */
CK_EXTERN CK_RV CK_FUNC(C_VerifyFinal)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pSignature,	/* the signature to be verified */
    CK_USHORT		usSignatureLen	/* count of bytes of signature */
);


/* C_VerifyRecoverInit initializes a signature verification operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
CK_EXTERN CK_RV CK_FUNC(C_VerifyRecoverInit)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the verification mechanism */
    CK_OBJECT_HANDLE	hKey		/* handle of the verification key */
);


/* C_VerifyRecover verifies a signature in a single-part operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
CK_EXTERN CK_RV CK_FUNC(C_VerifyRecover)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pSignature,	/* the signature to be verified */
    CK_USHORT		usSignatureLen, /* count of bytes of signature */
    CK_BYTE_PTR		pData,		/* receives decrypted data (digest) */
    CK_USHORT_PTR	pusDataLen	/* receives byte count of data */
);


/* C_GenerateKey generates a secret key, creating a new key object. */
CK_EXTERN CK_RV CK_FUNC(C_GenerateKey)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the key generation mechanism */
    CK_ATTRIBUTE_PTR	pTemplate,	/* template for the new key */
    CK_USHORT		usCount,	/* number of attributes in template */
    CK_OBJECT_HANDLE_PTR phKey		/* receives handle of new key */
);


/* C_GenerateKeyPair generates a public-key/private-key pair, 
 * creating new key objects. */
CK_EXTERN CK_RV CK_FUNC(C_GenerateKeyPair)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the key generation mechanism */
    CK_ATTRIBUTE_PTR	pPublicKeyTemplate,	/* public attribute template */
    CK_USHORT		usPublicKeyAttributeCount, /* no of public attributes*/
    CK_ATTRIBUTE_PTR	pPrivateKeyTemplate,	/* private attribute template */
    CK_USHORT		usPrivateKeyAttributeCount,/* no of private attributes*/
    CK_OBJECT_HANDLE_PTR phPrivateKey,	/* receives handle of new private key */
    CK_OBJECT_HANDLE_PTR phPublicKey	/* receives handle of new public key */
);


/* C_WrapKey wraps (i.e., encrypts) a key. */
CK_EXTERN CK_RV CK_FUNC(C_WrapKey)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the wrapping mechanism */
    CK_OBJECT_HANDLE	hWrappingKey,	/* handle of the wrapping key */
    CK_OBJECT_HANDLE	hKey,		/* handle of the key to be wrapped */
    CK_BYTE_PTR		pWrappedKey,	/* receives the wrapped key */
    CK_USHORT_PTR	pusWrappedKeyLen /* receives byte size of wrapped key */
);


/* C_UnwrapKey unwraps (decrypts) a wrapped key, creating a new key object. */
CK_EXTERN CK_RV CK_FUNC(C_UnwrapKey)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the unwrapping mechanism */
    CK_OBJECT_HANDLE	hUnwrappingKey, /* handle of the unwrapping key */
    CK_BYTE_PTR		pWrappedKey,	/* the wrapped key */
    CK_USHORT		usWrappedKeyLen,/* bytes length of wrapped key */
    CK_ATTRIBUTE_PTR	pTemplate,	/* template for the new key */
    CK_USHORT		usAttributeCount,/* number of attributes in template */
    CK_OBJECT_HANDLE_PTR phKey		/* receives handle of recovered key */
);


/* C_DeriveKey derives a key from a base key, creating a new key object. */
CK_EXTERN CK_RV CK_FUNC(C_DeriveKey)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_MECHANISM_PTR	pMechanism,	/* the key derivation mechanism */
    CK_OBJECT_HANDLE	hBaseKey,	/* handle of the base key */
    CK_ATTRIBUTE_PTR	pTemplate,	/* template for the new key */
    CK_USHORT		usAttributeCount, /* number of attributes in template */
    CK_OBJECT_HANDLE_PTR phKey		/* receives handle of derived key */
);


/* C_SeedRandom mixes additional seed material into the token's random number 
 * generator. */
CK_EXTERN CK_RV CK_FUNC(C_SeedRandom)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pSeed,		/* the seed material */
    CK_USHORT		usSeedLen	/* count of bytes of seed material */
);


/* C_GenerateRandom generates random data. */
CK_EXTERN CK_RV CK_FUNC(C_GenerateRandom)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pRandomData,	/* receives the random data */
    CK_USHORT		usRandomLen	/* number of bytes to be generated */
);


/* C_GetFunctionStatus obtains an updated status of a function running 
 * in parallel with an application. */
CK_EXTERN CK_RV CK_FUNC(C_GetFunctionStatus)(
    CK_SESSION_HANDLE	hSession	/* the session's handle */
);


/* C_CancelFunction cancels a function running in parallel */
CK_EXTERN CK_RV CK_FUNC(C_CancelFunction)(
    CK_SESSION_HANDLE	hSession	/* the session's handle */
);


#if defined(CRYSTOKI)
/* extensions from Chrysalis-ITS proposal of September 15, 1996 */
CK_EXTERN CK_RV CK_FUNC(C_CreateUser)(
    CK_SESSION_HANDLE	hSession, 	/* the session's handle */
    CK_CHAR_PTR		pUserName,  	/* the new user's name 	   */
    CK_USHORT		usUserNameLen  	/* length of the new user's name */
);

CK_EXTERN CK_RV CK_FUNC(C_DestroyUser)(
    CK_SESSION_HANDLE	hSession, 	/* the session's handle */
    CK_USER_HANDLE	hUser		/* handle of user to be destroyed */
);

CK_EXTERN CK_RV CK_FUNC(C_GetUserList)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_USER_HANDLE_PTR	phUserList,	/* receives array of user handles */
    CK_USHORT_PTR	pusUserCount	/* receives count of user handles */
);

CK_EXTERN CK_RV CK_FUNC(C_GetUserName)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_USER_HANDLE	hUser		/* handle of user to be destroyed */
    CK_CHAR_PTR		pUserName,	/* receives the user name */
    CK_USHORT_PTR	pusUserNameLen	/* receives byte count of user name */
);

/* C_LoginUser logs a Specific user into a token. */
CK_EXTERN CK_RV CK_FUNC(C_LoginUser)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_USER_HANDLE	hUser		/* handle of user to be destroyed */
    CK_CHAR_PTR		pPin,		/* the user's PIN */
    CK_USHORT		usPinLen	/* the length of the PIN. */
);

/* C_DecryptDigestUpdate continues a multiple-part decryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_DecryptDigestUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pEncryptedPart,	/* input encrypted data */
    CK_USHORT		usEncryptedPartLen, /* count of bytes of input */
    CK_BYTE_PTR		pPart,		/* receives decrypted output */
    CK_USHORT_PTR	pusPartLen	/* receives decrypted byte count */
);

/* C_DigestEncryptUpdate continues a multiple-part encryption operation. */
CK_EXTERN CK_RV CK_FUNC(C_DigestEncryptUpdate)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_BYTE_PTR		pPart,		/* the plaintext data */
    CK_USHORT		usPartLen,	/* bytes of plaintext data */
    CK_BYTE_PTR		pEncryptedPart,	/* receives encrypted data */
    CK_USHORT_PTR	pusEncryptedPartLen /* receives encrypted byte count */
);

/* C_DigestKey continues a multi-part message-digesting operation,
 * by digesting the value of a secret key as part of the data already digested.
 */
CK_EXTERN CK_RV CK_FUNC(C_DigestKey)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_OBJECT_HANDLE	hKey		/* handle of secret key to digest */
);

#endif /* CRYSTOKI */
