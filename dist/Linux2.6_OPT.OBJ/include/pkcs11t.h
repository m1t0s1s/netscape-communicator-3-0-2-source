
#ifndef _PKCS11T_H_
#define _PKCS11T_H_ 1

/* an unsigned 8-bit value */
typedef unsigned char 	CK_BYTE;

/* an unsigned 8-bit character */
typedef CK_BYTE 	CK_CHAR;

/* a BYTE-sized Boolean flag */
typedef CK_BYTE		CK_BBOOL;

/* an unsigned value, at least 32 bits long */
typedef unsigned long int CK_ULONG;

/* a signed value, the same size as a CK_ULONG */
/* CK_LONG is new for v2.0 */
typedef long int	CK_LONG;

/* at least 32 bits, each bit is a Boolean flag */
typedef CK_ULONG 	CK_FLAGS;


/* these data types are platform/implementation dependent. */
#if defined(XP_WIN) 
#if defined(_WIN32)
#define CK_ENTRY	
#define CK_PTR		*	/* definition for Win32 */ 
#define NULL_PTR 	0	/* NULL pointer */
#pragma pack(push, cryptoki, 1)
#else /* win16 */
#define CK_ENTRY
#define CK_PTR		*	/* definition for Win16 */ 
#define NULL_PTR 	0	/* NULL pointer */
#pragma pack(push, cryptoki, 1)
#endif
#else /* not windows */
#define CK_ENTRY
#define CK_PTR		*	/* definition for UNIX */ 
#define NULL_PTR 	0	/* NULL pointer */
#endif


typedef CK_BYTE   CK_PTR	CK_BYTE_PTR;   /* Pointer to a CK_BYTE */
typedef CK_CHAR   CK_PTR	CK_CHAR_PTR;   /* Pointer to a CK_CHAR */
typedef CK_ULONG  CK_PTR	CK_ULONG_PTR;  /* Pointer to a CK_ULONG */
typedef void      CK_PTR	CK_VOID_PTR;   /* Pointer to a void */


typedef struct CK_VERSION {
    CK_BYTE	major;	/* integer    portion of the version number */
    CK_BYTE	minor;  /* hundredths portion of the version number */
} CK_VERSION;


typedef struct CK_INFO {
    CK_VERSION	cryptokiVersion;	/* Cryptoki interface version number */
    CK_CHAR	manufacturerID[32];	/* blank padded */
    CK_FLAGS	flags;			/* must be zero */

    /* libraryDescription and libraryVersion are new for v2.0 */
    CK_CHAR	libraryDescription[32];	/* blank padded */
    CK_VERSION	libraryVersion;		/* version of library */
} CK_INFO;

typedef CK_INFO CK_PTR	CK_INFO_PTR;	/* points to a CK_INFO structure */


/* CK_NOTIFICATION enumerates the types of notifications 
 * that Cryptoki provides to an application.  */
/* CK_NOTIFICATION has been changed from an enum to a CK_ULONG for v2.0 */
typedef CK_ULONG CK_NOTIFICATION;
#define CKN_SURRENDER		0
#define CKN_COMPLETE		1
#define CKN_DEVICE_REMOVED	2

/* CKN_TOKEN_INSERTION is new to v2.0 */
#define CKN_TOKEN_INSERTION	3


typedef CK_ULONG	CK_SLOT_ID;

/* CK_SLOT_ID_PTR points to a CK_SLOT_ID.  */
typedef CK_SLOT_ID CK_PTR	CK_SLOT_ID_PTR;


/* CK_SLOT_INFO provides information about a slot. */
typedef struct CK_SLOT_INFO {
    CK_CHAR	slotDescription[64];	/* blank padded */
    CK_CHAR	manufacturerID[32];	/* blank padded */
    CK_FLAGS	flags;

    /* hardwareVersion and firmwareVersion are new for v2.0 */
    CK_VERSION	hardwareVersion;	/* version of hardware */
    CK_VERSION  firmwareVersion;	/* version of firmware */
} CK_SLOT_INFO;

/* flags:	bits flags that provide capabilities of the slot.
 *	Bit Flag		Mask		Meaning
 */
#define CKF_TOKEN_PRESENT	0x00000001	/* a token is present in the slot */
#define CKF_REMOVABLE_DEVICE	0x00000002	/* reader supports removable devices*/
#define CKF_HW_SLOT		0x00000004	/* a hardware slot, not a "soft token"*/

/* CK_SLOT_INFO_PTR points to a CK_SLOT_INFO. */
typedef CK_SLOT_INFO CK_PTR	CK_SLOT_INFO_PTR;


/* CK_TOKEN_INFO provides information about a token. */
typedef struct CK_TOKEN_INFO {
    CK_CHAR	label[32];		/* blank padded */
    CK_CHAR	manufacturerID[32];	/* blank padded */
    CK_CHAR	model[16];		/* blank padded */
    CK_CHAR	serialNumber[16];	/* blank padded */
    CK_FLAGS	flags;			/* see below */

    /* ulMaxSessionCount, ulSessionCount, ulMaxRwSessionCount, ulRwSessionCount, */
    /* ulMaxPinLen, and ulMinPinLen have all been changed from CK_USHORT to */
    /* CK_ULONG for v2.0 */
    CK_ULONG	ulMaxSessionCount;	/* max open sessions */
    CK_ULONG	ulSessionCount;		/* sessions currently open */
    CK_ULONG	ulMaxRwSessionCount;	/* max R/W sessions */
    CK_ULONG	ulRwSessionCount;	/* R/W sessions currently open */
    CK_ULONG	ulMaxPinLen;		/* in bytes */
    CK_ULONG	ulMinPinLen;		/* in bytes */
    CK_ULONG	ulTotalPublicMemory;	/* in bytes */
    CK_ULONG	ulFreePublicMemory;	/* in bytes */
    CK_ULONG	ulTotalPrivateMemory;	/* in bytes */
    CK_ULONG	ulFreePrivateMemory;	/* in bytes */

    /* hardwareVersion, firmwareVersion, and time are new for v2.0 */
    CK_VERSION	hardwareVersion;	/* version of hardware */
    CK_VERSION	firmwareVersion;	/* version of firmware */
    CK_CHAR	utcTime[16];		/* time */
} CK_TOKEN_INFO;

/* The flags parameter is defined as follows:
 * Table 7-2, Token Information Flags
 *	Bit Flag		Mask		Meaning 
 */
#define CKF_RNG			       0x00000001  /* has random number generator */
#define CKF_WRITE_PROTECTED	       0x00000002  /* token is write-protected */
#define CKF_LOGIN_REQUIRED	       0x00000004  /* a user must be logged in  */
#define CKF_USER_PIN_INITIALIZED       0x00000008  /* normal user's PIN is initialized */
#define CKF_EXCLUSIVE_EXISTS	       0x00000010  /* an exclusive session exists */

/* CKF_RESTORE_KEY_NOT_NEEDED is new for v2.0.  If it is set, then that means */
/* that *every* time the state of cryptographic operations of a session is */
/* successfully saved, all keys needed to continue those operations are */
/* stored in the state. */
#define CKF_RESTORE_KEY_NOT_NEEDED   0x00000020  /* key always saved in saved sessions */

/* CKF_CLOCK_ON_TOKEN is new for v2.0.  If it is set, then that means that */
/* the token has some sort of clock.  The time on that clock is returned in */
/* the token info structure. */
#define CKF_CLOCK_ON_TOKEN           0x00000040  /* token has a clock */

/* CKF_SUPPORTS_PARALLEL is new for v2.0 */
#define CKF_SUPPORTS_PARALLEL        0x00000080  /* token supports parallel sessions */

/* CKF_PROTECTED_AUTHENTICATION_PATH is new for v2.0.  If it is true, that means */
/* that there is some way for the user to login without sending a PIN through */
/* the Cryptoki library itself. */
#define CKF_PROTECTED_AUTHENTICATION_PATH 0x00000100  /* token has protected path */

/* CK_TOKEN_INFO_PTR points to a CK_TOKEN_INFO. */
typedef CK_TOKEN_INFO CK_PTR	CK_TOKEN_INFO_PTR;


/* CK_SESSION_HANDLE is a Cryptoki-assigned value that identifies a session. */
typedef CK_ULONG 	CK_SESSION_HANDLE;

/* CK_SESSION_HANDLE_PTR points to a CK_SESSION_HANDLE. */
typedef CK_SESSION_HANDLE CK_PTR CK_SESSION_HANDLE_PTR; 


/* CK_USER_TYPE enumerates the types of Cryptoki users */
/* CK_USER_TYPE has been changed from an enum to a CK_ULONG for v2.0 */
typedef CK_ULONG	CK_USER_TYPE;
/* Security Officer */
#define CKU_SO		0
/* Normal user */
#define CKU_USER	1


/* CK_STATE enumerates the session states */
/* CK_STATE has been changed from an enum to a CK_ULONG for v2.0 */
typedef CK_ULONG	CK_STATE;
#define CKS_RO_PUBLIC_SESSION	0
#define CKS_RO_USER_FUNCTIONS	1
#define CKS_RW_PUBLIC_SESSION	2
#define CKS_RW_SO_FUNCTIONS	3
#define CKS_RW_USER_FUNCTIONS	4


/* CK_SESSION_INFO provides information about a session. */
typedef struct CK_SESSION_INFO {
    CK_SLOT_ID	slotID;
    CK_STATE	state;
    CK_FLAGS	flags;		/* see below */

    /* ulDeviceError was changed from CK_USHORT to CK_ULONG for v2.0 */
    CK_ULONG	ulDeviceError;	/* device-dependent error code */
} CK_SESSION_INFO;

/* The flags are defined in the following table. */
/* Table 7-3, Session Information Flags */
/* 	Bit Flag		Mask	Meaning
 */
#define CKF_EXCLUSIVE_SESSION	0x00000001	/* session is exclusive; not shared */
#define CKF_RW_SESSION		0x00000002	/* session is read/write; not R/O */
#define CKF_SERIAL_SESSION	0x00000004	/* session doesn't support parallel */

/* CKF_INSERTION_CALLBACK is new for v2.0.  If it is set in the flags supplied to a */
/* C_OpenSession call, then instead of actually opening a session, the call */
/* is a request to get a callback when the token is inserted. */
#define CKF_INSERTION_CALLBACK	0x00000008	/* application gets notice of insertion */

/* CK_SESSION_INFO_PTR points to a CK_SESSION_INFO. */
typedef CK_SESSION_INFO CK_PTR	CK_SESSION_INFO_PTR;


/* CK_OBJECT_HANDLE is a token-specific identifier for an object.  */
typedef CK_ULONG 	CK_OBJECT_HANDLE;

/* CK_OBJECT_HANDLE_PTR points to a CK_OBJECT_HANDLE. */
typedef CK_OBJECT_HANDLE CK_PTR	CK_OBJECT_HANDLE_PTR;


/* CK_OBJECT_CLASS is a value that identifies the classes (or types) 
 * of objects that Cryptoki recognizes.  It is defined as follows: */
/* CK_OBJECT_CLASS was changed from CK_USHORT to CK_ULONG for v2.0 */
typedef CK_ULONG	CK_OBJECT_CLASS;

/* The following classes of objects are defined: */
#define CKO_DATA            0x00000000
#define CKO_CERTIFICATE     0x00000001
#define CKO_PUBLIC_KEY      0x00000002
#define CKO_PRIVATE_KEY     0x00000003
#define CKO_SECRET_KEY      0x00000004
#define CKO_VENDOR_DEFINED  0x80000000L

/* CK_OBJECT_CLASS_PTR points to a CK_OBJECT_CLASS structure. */
typedef CK_OBJECT_CLASS CK_PTR CK_OBJECT_CLASS_PTR;


/* CK_KEY_TYPE is a value that identifies a key type. */
/* CK_KEY_TYPE was changed from CK_USHORT to CK_ULONG for v2.0 */
typedef CK_ULONG 	CK_KEY_TYPE;

/* the following key types are defined: */
#define CKK_RSA			0x00000000
#define CKK_DSA			0x00000001
#define CKK_DH			0x00000002

/* CKK_ECDSA, CKK_MAYFLY, and CKK_KEA are new for v2.0 */
#define CKK_ECDSA		0x00000003
#define CKK_MAYFLY		0x00000004
#define CKK_KEA			0x00000005

#define CKK_GENERIC_SECRET	0x00000010
#define CKK_RC2			0x00000011
#define CKK_RC4			0x00000012
#define CKK_DES			0x00000013
#define CKK_DES2		0x00000014
#define CKK_DES3		0x00000015

/* all these key types are new for v2.0 */
#define CKK_CAST		0x00000016
#define CKK_CAST3		0x00000017
#define CKK_CAST5		0x00000018
#define CKK_RC5			0x00000019
#define CKK_IDEA		0x0000001A
#define CKK_SKIPJACK		0x0000001B
#define CKK_BATON		0x0000001C
#define CKK_JUNIPER		0x0000001D
#define CKK_CDMF		0x0000001E

#define CKK_VENDOR_DEFINED	0x80000000L

/* CK_CERTIFICATE_TYPE is a value that identifies a certificate type. */
/* CK_CERTIFICATE_TYPE was changed from CK_USHORT to CK_ULONG for v2.0 */
typedef CK_ULONG 	CK_CERTIFICATE_TYPE;

/* The following certificate types are defined: */
#define CKC_X_509           0x00000000
#define CKC_VENDOR_DEFINED  0x80000000L


/* CK_ATTRIBUTE_TYPE is a value that identifies an attribute type. */
/* CK_ATTRIBUTE_TYPE was changed from CK_USHORT to CK_ULONG for v2.0 */
typedef CK_ULONG	CK_ATTRIBUTE_TYPE;

/* The following attribute types are defined: */
#define CKA_CLASS              0x00000000
#define CKA_TOKEN              0x00000001
#define CKA_PRIVATE            0x00000002
#define CKA_LABEL              0x00000003
#define CKA_APPLICATION        0x00000010
#define CKA_VALUE              0x00000011
#define CKA_CERTIFICATE_TYPE   0x00000080
#define CKA_ISSUER             0x00000081
#define CKA_SERIAL_NUMBER      0x00000082
#define CKA_KEY_TYPE           0x00000100
#define CKA_SUBJECT            0x00000101
#define CKA_ID                 0x00000102
#define CKA_SENSITIVE          0x00000103
#define CKA_ENCRYPT            0x00000104
#define CKA_DECRYPT            0x00000105
#define CKA_WRAP               0x00000106
#define CKA_UNWRAP             0x00000107
#define CKA_SIGN               0x00000108
#define CKA_SIGN_RECOVER       0x00000109
#define CKA_VERIFY             0x0000010A
#define CKA_VERIFY_RECOVER     0x0000010B
#define CKA_DERIVE             0x0000010C
#define CKA_START_DATE         0x00000110
#define CKA_END_DATE           0x00000111
#define CKA_MODULUS            0x00000120
#define CKA_MODULUS_BITS       0x00000121
#define CKA_PUBLIC_EXPONENT    0x00000122
#define CKA_PRIVATE_EXPONENT   0x00000123
#define CKA_PRIME_1            0x00000124
#define CKA_PRIME_2            0x00000125
#define CKA_EXPONENT_1         0x00000126
#define CKA_EXPONENT_2         0x00000127
#define CKA_COEFFICIENT        0x00000128
#define CKA_PRIME              0x00000130
#define CKA_SUBPRIME           0x00000131
#define CKA_BASE               0x00000132
#define CKA_VALUE_BITS         0x00000160
#define CKA_VALUE_LEN          0x00000161

/* CKA_EXTRACTABLE, CKA_LOCAL, CKA_NEVER_EXTRACTABLE, CKA_ALWAYS_SENSITIVE, */
/* and CKA_MODIFIABLE are new for v2.0 */
#define CKA_EXTRACTABLE        0x00000162
#define CKA_LOCAL              0x00000163
#define CKA_NEVER_EXTRACTABLE  0x00000164
#define CKA_ALWAYS_SENSITIVE   0x00000165
#define CKA_MODIFIABLE         0x00000170

#define CKA_VENDOR_DEFINED     0x80000000L


/* CK_ATTRIBUTE is a structure that includes the type, length and value 
 * of an attribute.  */
typedef struct CK_ATTRIBUTE {
    CK_ATTRIBUTE_TYPE	type;
    CK_VOID_PTR		pValue;

    /* ulValueLen was changed from CK_USHORT to CK_ULONG for v2.0 */
    CK_ULONG		ulValueLen;	/* in bytes */
} CK_ATTRIBUTE;

/* CK_ATTRIBUTE_PTR points to a CK_ATTRIBUTE. */
typedef CK_ATTRIBUTE CK_PTR	CK_ATTRIBUTE_PTR;


/* CK_DATE is a structure that defines a date. */
typedef struct CK_DATE{
    CK_CHAR		year[4];	/* the year ("1900" - "9999")	*/
    CK_CHAR		month[2];	/* the month ("01" - "12")	*/
    CK_CHAR		day[2];		/* the day   ("01" - "31")	*/
} CK_DATE;


/* CK_MECHANISM_TYPE is a value that identifies a mechanism type. */
/* CK_MECHANISM_TYPE was changed from CK_USHORT to CK_ULONG for v2.0 */
typedef CK_ULONG	CK_MECHANISM_TYPE;

/* the following mechanism types are defined: */
#define CKM_RSA_PKCS_KEY_PAIR_GEN	0x00000000
#define CKM_RSA_PKCS			0x00000001
#define CKM_RSA_9796			0x00000002
#define CKM_RSA_X_509			0x00000003

/* CKM_MD2_RSA_PKCS, CKM_MD5_RSA_PKCS, and CKM_SHA1_RSA_PKCS are */
/* new for v2.0.  They are mechanisms which hash and sign. */
#define CKM_MD2_RSA_PKCS		0x00000004
#define CKM_MD5_RSA_PKCS		0x00000005
#define CKM_SHA1_RSA_PKCS		0x00000006

#define CKM_DSA_KEY_PAIR_GEN		0x00000010
#define CKM_DSA				0x00000011
#define CKM_DH_PKCS_KEY_PAIR_GEN 	0x00000020
#define CKM_DH_PKCS_DERIVE		0x00000021
#define CKM_RC2_KEY_GEN			0x00000100
#define CKM_RC2_ECB			0x00000101
#define CKM_RC2_CBC			0x00000102
#define CKM_RC2_MAC			0x00000103

/* CKM_RC2_MAC_GENERAL and CKM_RC2_CBC_PAD are new to v2.0 */
#define CKM_RC2_MAC_GENERAL		0x00000104
#define CKM_RC2_CBC_PAD			0x00000105

#define CKM_RC4_KEY_GEN			0x00000110
#define CKM_RC4				0x00000111
#define CKM_DES_KEY_GEN			0x00000120
#define CKM_DES_ECB			0x00000121
#define CKM_DES_CBC			0x00000122
#define CKM_DES_MAC			0x00000123

/* CKM_DES_MAC_GENERAL and CKM_DES_CBC_PAD are new to v2.0 */
#define CKM_DES_MAC_GENERAL		0x00000124
#define CKM_DES_CBC_PAD			0x00000125

#define CKM_DES2_KEY_GEN		0x00000130
#define CKM_DES3_KEY_GEN		0x00000131
#define CKM_DES3_ECB			0x00000132
#define CKM_DES3_CBC			0x00000133
#define CKM_DES3_MAC			0x00000134

/* CKM_DES3_MAC_GENERAL, CKM_DES3_CBC_PAD, CKM_CDMF_KEY_GEN, */
/* CKM_CDMF_ECB, CKM_CDMF_CBC, CKM_CDMF_MAC, CKM_CDMF_MAC_GENERAL, */
/* and CKM_CDMF_CBC_PAD are new to v2.0 */
#define CKM_DES3_MAC_GENERAL		0x00000135
#define CKM_DES3_CBC_PAD		0x00000136
#define CKM_CDMF_KEY_GEN		0x00000140
#define CKM_CDMF_ECB			0x00000141
#define CKM_CDMF_CBC			0x00000142
#define CKM_CDMF_MAC			0x00000143
#define CKM_CDMF_MAC_GENERAL		0x00000144
#define CKM_CDMF_CBC_PAD		0x00000145

#define CKM_MD2				0x00000200

/* CKM_MD2_HMAC and CKM_MD2_HMAC_GENERAL are new to v2.0 */
#define CKM_MD2_HMAC			0x00000201
#define CKM_MD2_HMAC_GENERAL		0x00000202

#define CKM_MD5				0x00000210

/* CKM_MD5_HMAC and CKM_MD5_HMAC_GENERAL are new to v2.0 */
#define CKM_MD5_HMAC			0x00000211
#define CKM_MD5_HMAC_GENERAL		0x00000212

#define CKM_SHA_1			0x00000220

/* CKM_SHA_1_HMAC and CKM_SHA_1_HMAC_GENERAL are new to v2.0 */
#define CKM_SHA_1_HMAC			0x00000221
#define CKM_SHA_1_HMAC_GENERAL		0x00000222

/* All the following mechanisms are new to v2.0 */
#define CKM_CAST_KEY_GEN		0x00000300
#define CKM_CAST_ECB			0x00000301
#define CKM_CAST_CBC			0x00000302
#define CKM_CAST_MAC			0x00000303
#define CKM_CAST_MAC_GENERAL		0x00000304
#define CKM_CAST_CBC_PAD		0x00000305
#define CKM_CAST3_KEY_GEN		0x00000310
#define CKM_CAST3_ECB			0x00000311
#define CKM_CAST3_CBC			0x00000312
#define CKM_CAST3_MAC			0x00000313
#define CKM_CAST3_MAC_GENERAL		0x00000314
#define CKM_CAST3_CBC_PAD		0x00000315
#define CKM_CAST5_KEY_GEN		0x00000320
#define CKM_CAST5_ECB			0x00000321
#define CKM_CAST5_CBC			0x00000322
#define CKM_CAST5_MAC			0x00000323
#define CKM_CAST5_MAC_GENERAL		0x00000324
#define CKM_CAST5_CBC_PAD		0x00000325
#define CKM_RC5_KEY_GEN			0x00000330
#define CKM_RC5_ECB			0x00000331
#define CKM_RC5_CBC			0x00000332
#define CKM_RC5_MAC			0x00000333
#define CKM_RC5_MAC_GENERAL		0x00000334
#define CKM_RC5_CBC_PAD			0x00000335
#define CKM_IDEA_KEY_GEN		0x00000340
#define CKM_IDEA_ECB			0x00000341
#define CKM_IDEA_CBC			0x00000342
#define CKM_IDEA_MAC			0x00000343
#define CKM_IDEA_MAC_GENERAL		0x00000344
#define CKM_IDEA_CBC_PAD		0x00000345
#define CKM_GENERIC_SECRET_KEY_GEN	0x00000350
#define CKM_CONCATENATE_BASE_AND_KEY	0x00000360
#define CKM_CONCATENATE_KEY_AND_BASE	0x00000361
#define CKM_CONCATENATE_BASE_AND_DATA	0x00000362
#define CKM_CONCATENATE_DATA_AND_BASE	0x00000363
#define CKM_XOR_BASE_AND_DATA		0x00000364
#define CKM_EXTRACT_KEY_FROM_KEY	0x00000365
#define CKM_SSL3_PRE_MASTER_KEY_GEN	0x00000370
#define CKM_SSL3_MASTER_KEY_DERIVE	0x00000371
#define CKM_SSL3_KEY_AND_MAC_DERIVE	0x00000372
#define CKM_SSL3_MD5_MAC		0x00000380
#define CKM_SSL3_SHA1_MAC		0x00000381
#define CKM_MD5_KEY_DERIVATION	0x00000390
#define CKM_MD2_KEY_DERIVATION	0x00000391
#define CKM_SHA1_KEY_DERIVATION	0x00000392
#define CKM_PBE_MD2_DES_CBC		0x000003A0
#define CKM_PBE_MD5_DES_CBC		0x000003A1
#define CKM_PBE_MD5_CAST_CBC		0x000003A2
#define CKM_PBE_MD5_CAST3_CBC		0x000003A3
#define CKM_PBE_MD5_CAST5_CBC		0x000003A4
#define CKM_KEY_WRAP_LYNKS		0x00000400
#define CKM_KEY_WRAP_SET_OAEP		0x00000401

/* Fortezza mechanisms */
#define CKM_SKIPJACK_KEY_GEN		0x00001000
#define CKM_SKIPJACK_ECB64		0x00001001
#define CKM_SKIPJACK_CBC64		0x00001002
#define CKM_SKIPJACK_OFB64		0x00001003
#define CKM_SKIPJACK_CFB64		0x00001004
#define CKM_SKIPJACK_CFB32		0x00001005
#define CKM_SKIPJACK_CFB16		0x00001006
#define CKM_SKIPJACK_CFB8		0x00001007
#define CKM_SKIPJACK_WRAP		0x00001008
#define CKM_KEA_KEY_PAIR_GEN		0x00001010
#define CKM_KEA_KEY_DERIVE		0x00001011
#define CKM_FORTEZZA_TIMESTAMP	0x00001020
#define CKM_BATON_KEY_GEN		0x00001030
#define CKM_BATON_ECB128		0x00001031
#define CKM_BATON_ECB96			0x00001032
#define CKM_BATON_CBC128		0x00001033
#define CKM_BATON_COUNTER		0x00001034
#define CKM_BATON_SHUFFLE		0x00001035
#define CKM_BATON_WRAP			0x00001036
#define CKM_ECDSA_KEY_PAIR_GEN	0x00001040
#define CKM_ECDSA				0x00001041
#define CKM_MAYFLY_KEY_PAIR_GEN	0x00001050
#define CKM_MAYFLY_KEY_DERIVE		0x00001051
#define CKM_JUNIPER_KEY_GEN		0x00001060
#define CKM_JUNIPER_ECB128		0x00001061
#define CKM_JUNIPER_CBC128		0x00001062
#define CKM_JUNIPER_COUNTER		0x00001063
#define CKM_JUNIPER_SHUFFLE		0x00001064
#define CKM_JUNIPER_WRAP		0x00001065
#define CKM_FASTHASH			0x00001070

#define CKM_VENDOR_DEFINED		0x80000000L


/* CK_MECHANISM_TYPE_PTR points to a CK_MECHANISM_TYPE structure. */
typedef CK_MECHANISM_TYPE CK_PTR CK_MECHANISM_TYPE_PTR;


/* CK_MECHANISM is a structure that specifies a particular mechanism.  */
typedef struct CK_MECHANISM {
    CK_MECHANISM_TYPE	mechanism;
    CK_VOID_PTR		pParameter;

    /* ulParameterLen was changed from CK_USHORT to CK_ULONG for v2.0 */
    CK_ULONG		ulParameterLen;	/* in bytes */
} CK_MECHANISM;

/* CK_MECHANISM_PTR points to a CK_MECHANISM structure. */
typedef CK_MECHANISM CK_PTR CK_MECHANISM_PTR;


/* CK_MECHANISM_INFO provides information about a particular mechanism. */
typedef struct CK_MECHANISM_INFO {
    CK_ULONG		ulMinKeySize;
    CK_ULONG		ulMaxKeySize;
    CK_FLAGS		flags;
} CK_MECHANISM_INFO;

/* The flags are defined as follows.
 *	Table 7-4, Mechanism Information FLags
 *	Bit Flag	Mask	Meaning */
#define CKF_HW			0x00000001 /* performed by HW device; not SW */

/* The flags CKF_ENCRYPT, CKF_DECRYPT, CKF_DIGEST, CKF_SIGN, CKG_SIGN_RECOVER, */
/* CKF_VERIFY, CKF_VERIFY_RECOVER, CKF_GENERATE, CKF_GENERATE_KEY_PAIR, CKF_WRAP, */
/* CKF_UNWRAP, and CKF_DERIVE are new for v2.0 */
#define CKF_ENCRYPT		0x00000100 /* can be used with C_EncryptInit */
#define CKF_DECRYPT		0x00000200 /* can be used with C_DecryptInit */
#define CKF_DIGEST		0x00000400 /* can be used with C_DigestInit */
#define CKF_SIGN 		0x00000800 /* can be used with C_SignInit */
#define CKF_SIGN_RECOVER	0x00001000 /* can use with C_SignRecoverInit */
#define CKF_VERIFY		0x00002000 /* can be used with C_VerifyInit */
#define CKF_VERIFY_RECOVER	0x00004000 /* can use w/ C_VerifyRecoverInit */
#define CKF_GENERATE		0x00008000L /* can be used with C_GenerateKey */
#define CKF_GENERATE_KEY_PAIR	0x00010000L /* can use with C_GenerateKeyPair */
#define CKF_WRAP		0x00020000L /* can be used with C_WrapKey */
#define CKF_UNWRAP		0x00040000L /* can be used with C_UnwrapKey */
#define CKF_DERIVE		0x00080000L /* can be used with C_DeriveKey */

#define CKF_EXTENSION		0x80000000L /* Must be FALSE for this version */

/* CK_MECHANISM_INFO_PTR points to a CK_MECHANISM_INFO structure. */
typedef CK_MECHANISM_INFO CK_PTR	CK_MECHANISM_INFO_PTR;


/* CK_RC2_PARAMS provides the parameters to the CKM_RC2_ECB and CKM_RC2_MAC */
/* mechanisms.  An instance of CK_RC2_PARAMS just holds the effective keysize. */
typedef CK_ULONG	CK_RC2_PARAMS;

/* CK_RC2_PARAMS_PTR points to a CK_RC2_PARAMS. */
typedef CK_RC2_PARAMS CK_PTR	CK_RC2_PARAMS_PTR;


/* CK_RC2_CBC_PARAMS provides the parameters to the CKM_RC2_CBC mechanism. */
typedef struct CK_RC2_CBC_PARAMS {
    /* ulEffectiveBits was changed from CK_USHORT to CK_ULONG for v2.0 */
    CK_ULONG	ulEffectiveBits;	/* effective bits (1-1024) */

    CK_BYTE	iv[8];			/* IV for CBC mode */
} CK_RC2_CBC_PARAMS;

/* CK_RC2_CBC_PARAMS_PTR points to a CK_RC2_CBC_PARAMS. */
typedef CK_RC2_CBC_PARAMS CK_PTR	CK_RC2_CBC_PARAMS_PTR;


/* CK_RC5_PARAMS provides the parameters to the CKM_RC5_ECB and CKM_RC5_MAC */
/* mechanisms. */
/* CK_RC5_PARAMS is new for v2.0 */
typedef struct CK_RC5_PARAMS {
    CK_ULONG	ulWordsize;		/* wordsize in bits */
    CK_ULONG	ulRounds;		/* number of rounds */
} CK_RC5_PARAMS;

/* CK_RC5_PARAMS_PTR points to a CK_RC5_PARAMS. */
typedef CK_RC5_PARAMS CK_PTR	CK_RC5_PARAMS_PTR;


/* CK_RC5_CBC_PARAMS provides the parameters to the CKM_RC5_CBC mechanism. */
/* CK_RC5_CBC_PARAMS is new for v2.0 */
typedef struct CK_RC5_CBC_PARAMS {
    CK_ULONG	ulWordsize;		/* wordsize in bits */
    CK_ULONG	ulRounds;		/* number of rounds */
    CK_BYTE_PTR	pIv;			/* pointer to IV */
    CK_ULONG	ulIvLen;		/* length of IV in bytes */
} CK_RC5_CBC_PARAMS;

/* CK_RC5_CBC_PARAMS_PTR points to a CK_RC5_CBC_PARAMS. */
typedef CK_RC5_CBC_PARAMS CK_PTR	CK_RC5_CBC_PARAMS_PTR;


/* CK_MAC_GENERAL_PARAMS provides the parameters to most block ciphers' */
/* MAC_GENERAL mechanisms.  Its value is the length of the MAC. */
/* CK_MAC_GENERAL_PARAMS is new for v2.0 */
typedef CK_ULONG		CK_MAC_GENERAL_PARAMS;

typedef CK_MAC_GENERAL_PARAMS CK_PTR	CK_MAC_GENERAL_PARAMS_PTR;


/* CK_RC2_MAC_GENERAL_PARAMS provides the parameters for the */
/* CKM_RC2_MAC_GENERAL mechanism. */
/* CK_RC2_MAC_GENERAL_PARAMS is new for v2.0 */
typedef struct CK_RC2_MAC_GENERAL_PARAMS {
    CK_ULONG	ulEffectiveBits;	/* effective bits (1-1024) */
    CK_ULONG	ulMacLength;	/* Length of MAC in bytes */
} CK_RC2_MAC_GENERAL_PARAMS;

typedef CK_RC2_MAC_GENERAL_PARAMS CK_PTR	CK_RC2_MAC_GENERAL_PARAMS_PTR;


/* CK_RC5_MAC_GENERAL_PARAMS provides the parameters for the */
/* CKM_RC5_MAC_GENERAL mechanism. */
/* CK_RC5_MAC_GENERAL_PARAMS is new for v2.0 */
typedef struct CK_RC5_MAC_GENERAL_PARAMS {
    CK_ULONG	ulWordsize;		/* wordsize in bits */
    CK_ULONG	ulRounds;		/* number of rounds */
    CK_ULONG	ulMacLength;	/* Length of MAC in bytes */
} CK_RC5_MAC_GENERAL_PARAMS;

typedef CK_RC5_MAC_GENERAL_PARAMS CK_PTR	CK_RC5_MAC_GENERAL_PARAMS_PTR;


/* CK_KEY_WRAP_SET_OAEP_PARAMS provides the parameters to the */
/* CKM_KEY_WRAP_SET_OAEP mechanism. */
/* CK_KEY_WRAP_SET_OAEP_PARAMS is new for v2.0 */
typedef struct CK_KEY_WRAP_SET_OAEP_PARAMS {
    CK_BYTE		bBC;			/* block contents byte */
    CK_BYTE_PTR	pX;			/* extra data */
    CK_ULONG	ulXLen;		/* length of extra data in bytes */
} CK_KEY_WRAP_SET_OAEP_PARAMS;

/* CK_KEY_WRAP_SET_OAEP_PARAMS_PTR points to a CK_KEY_WRAP_SET_OAEP_PARAMS. */
typedef CK_KEY_WRAP_SET_OAEP_PARAMS CK_PTR	CK_KEY_WRAP_SET_OAEP_PARAMS_PTR;


/* CK_SKIPJACK_PARAMS provides the parameters to the CKM_SKIPJACK_ECB, */
/* CKM_SKIPJACK_CBC64, CKM_SKIPJACK_OFB64, CKM_SKIPJACK_CFB64, */
/* CKM_SKIPJACK_CFB32, CKM_SKIPJACK_CFB16, and CKM_SKIPJACK_CFB8 */
/* mechanisms. */
/* CK_SKIPJACK_PARAMS is new for v2.0 */
typedef struct CK_SKIPJACK_PARAMS {
    CK_BYTE	iv[24];
} CK_SKIPJACK_PARAMS;

/* CK_SKIPJACK_PARAMS_PTR points to a CK_SKIPJACK_PARAMS. */
typedef CK_SKIPJACK_PARAMS CK_PTR	CK_SKIPJACK_PARAMS_PTR;

/* CK_BATON_PARAMS provides the parameters to the CKM_BATON_ECB128, */
/* CKM_BATON_ECB96, CKM_BATON_CBC128, CKM_BATON_COUNTER, and */
/* CKM_BATON_SHUFFLE mechanisms. */
/* CK_BATON_PARAMS is new for v2.0 */
typedef struct CK_BATON_PARAMS {
    CK_BYTE	iv[24];
} CK_BATON_PARAMS;

/* CK_BATON_PARAMS_PTR points to a CK_BATON_PARAMS. */
typedef CK_BATON_PARAMS CK_PTR		CK_BATON_PARAMS_PTR;


/* CK_JUNIPER_PARAMS provides the parameters to the CKM_JUNIPER_ECB128, */
/* CKM_JUNIPER_CBC128, CKM_JUNIPER_COUNTER, and CKM_JUNIPER_SHUFFLE */
/* mechanisms. */
/* CK_JUNIPER_PARAMS is new for v2.0 */
typedef struct CK_JUNIPER_PARAMS {
    CK_BYTE	iv[24];
} CK_JUNIPER_PARAMS;

/* CK_JUNIPER_PARAMS_PTR points to a CK_JUNIPER_PARAMS. */
typedef CK_JUNIPER_PARAMS CK_PTR	CK_JUNIPER_PARAMS_PTR;


/* CK_KEA_DERIVE_PARAMS provides the parameters to the CKM_KEA_DERIVE mechanism. */
/* CK_KEA_DERIVE_PARAMS is new for v2.0 */
typedef struct CK_KEA_DERIVE_PARAMS {
    CK_BBOOL	isSender;
    CK_ULONG	ulRandomLen;
    CK_BYTE_PTR	pRandomA;
    CK_BYTE_PTR	pRandomB;
    CK_ULONG	ulPublicDataLen;
    CK_BYTE_PTR	pPublicData;
} CK_KEA_DERIVE_PARAMS;

/* CK_KEA_DERIVE_PARAMS_PTR points to a CK_KEA_DERIVE_PARAMS. */
typedef CK_KEA_DERIVE_PARAMS CK_PTR	CK_KEA_DERIVE_PARAMS_PTR;


/* CK_MAYFLY_DERIVE_PARAMS provides the parameters to the CKM_MAYFLY_DERIVE mechanism. */
/* CK_MAYFLY_DERIVE_PARAMS is new for v2.0 */
typedef struct CK_MAYFLY_DERIVE_PARAMS {
    CK_BBOOL	isSender;
    CK_ULONG	ulRandomLen;
    CK_BYTE_PTR	pRandomA;
    CK_BYTE_PTR	pRandomB;
    CK_ULONG	ulPublicDataLen;
    CK_BYTE_PTR	pPublicData;
} CK_MAYFLY_DERIVE_PARAMS;

/* CK_MAYFLY_DERIVE_PARAMS_PTR points to a CK_MAYFLY_DERIVE_PARAMS. */
typedef CK_MAYFLY_DERIVE_PARAMS CK_PTR	CK_MAYFLY_DERIVE_PARAMS_PTR;


/* CK_EXTRACTX_PARAMS provides the parameters to the CKM_SKIPJACK_WRAP mechanism. */
/* CK_EXTRACTX_PARAMS is new for v2.0 */
typedef struct CK_EXTRACTX_PARAMS {
    CK_BBOOL	bKeaKey;
    CK_ULONG	ulPasswordLen;
    CK_BYTE_PTR	pPassword;
    CK_ULONG	ulPublicDataLen;
    CK_BYTE_PTR	pPublicData;
    CK_ULONG	ulWrappedXLen;
    CK_BYTE_PTR	pWrappedX;
    CK_ULONG	ulPAndGLen;
    CK_ULONG	ulQLen;
    CK_ULONG	ulRandomLen;
    CK_BYTE_PTR	pRandomA;
    CK_BYTE_PTR	pPrimeP;
    CK_BYTE_PTR	pBaseG;
    CK_BYTE_PTR	pSubprimeQ;
} CK_EXTRACTX_PARAMS;

/* CK_EXTRACTX_PARAMS_PTR points to a CK_EXTRACTX_PARAMS. */
typedef CK_EXTRACTX_PARAMS CK_PTR	CK_EXTRACTX_PARAMS_PTR;


/* CK_RELAYX_PARAMS provides the parameters to the CKM_SKIPJACK_WRAP mechanism. */
/* CK_RELAYX_PARAMS is new for v2.0 */
typedef struct CK_RELAYX_PARAMS {
    CK_ULONG	ulOldPasswordLen;
    CK_BYTE_PTR	pOldPassword;
    CK_ULONG	ulOldPublicDataLen;
    CK_BYTE_PTR	pOldPublicData;
    CK_ULONG	ulOldRandomLen;
    CK_BYTE_PTR	pOldRandomA;
    CK_ULONG	ulWrappedXLen;
    CK_BYTE_PTR	pOldWrappedX;
    CK_ULONG	ulNewRandomLen;
    CK_BYTE_PTR	pNewRandomA;
    CK_ULONG	ulNewPasswordLen;
    CK_BYTE_PTR	pNewPassword;
    CK_ULONG	ulNewPublicDataLen;
    CK_BYTE_PTR	pNewPublicData;
    CK_BYTE_PTR	pNewWrappedX;
} CK_RELAYX_PARAMS;

/* CK_RELAYX_PARAMS_PTR points to a CK_RELAYX_PARAMS. */
typedef CK_RELAYX_PARAMS CK_PTR	CK_RELAYX_PARAMS_PTR;


/* CK_INSTALLX_PARAMS provides the parameters to the CKM_SKIPJACK_WRAP mechanism. */
/* CK_INSTALLX_PARAMS is new for v2.0 */
typedef struct CK_INSTALLX_PARAMS {
    CK_BBOOL	bKeaKey;
    CK_ULONG	ulPasswordLen;
    CK_BYTE_PTR	pPassword;
    CK_ULONG	ulPublicDataLen;
    CK_BYTE_PTR	pPublicData;
    CK_ULONG	ulWrappedXLen;
    CK_BYTE_PTR	pWrappedX;
    CK_ULONG	ulPAndGLen;
    CK_ULONG	ulQLen;
    CK_ULONG	ulRandomLen;
    CK_BYTE_PTR	pRandomA;
    CK_BYTE_PTR	pPrimeP;
    CK_BYTE_PTR	pBaseG;
    CK_BYTE_PTR	pSubprimeQ;
} CK_INSTALLX_PARAMS;

/* CK_INSTALLX_PARAMS_PTR points to a CK_INSTALLX_PARAMS. */
typedef CK_INSTALLX_PARAMS CK_PTR	CK_INSTALLX_PARAMS_PTR;


/* The CK_EXTRACT_PARAMS is used for the CKM_EXTRACT_KEY_FROM_KEY mechanism. */
/* CK_EXTRACT_PARAMS is new for v2.0 */
typedef struct CK_EXTRACT_PARAMS {
    CK_ULONG		ulLocationOfFirstBit;
} CK_EXTRACT_PARAMS;

/* CK_EXTRACT_PARAMS_PTR points to a CK_EXTRACT_PARAMS. */
typedef CK_EXTRACT_PARAMS CK_PTR CK_EXTRACT_PARAMS_PTR;


typedef struct CK_PBE_PARAMS {
    CK_CHAR_PTR		pInitVector;
    CK_CHAR_PTR		pPassword;
    CK_ULONG		ulPasswordLen;
    CK_CHAR_PTR		pSalt;
    CK_ULONG		ulSaltLen;
    CK_ULONG		ulIteration;
} CK_PBE_PARAMS;

typedef CK_PBE_PARAMS CK_PTR CK_PBE_PARAMS_PTR;


typedef struct CK_SSL3_RANDOM_DATA {
    CK_BYTE_PTR		pClientRandom;
    CK_ULONG		ulClientRandomLen;
    CK_BYTE_PTR		pServerRandom;
    CK_ULONG		ulServerRandomLen;
} CK_SSL3_RANDOM_DATA;


typedef struct CK_SSL3_MASTER_KEY_DERIVE_PARAMS {
    CK_SSL3_RANDOM_DATA	RandomInfo;
    CK_BYTE_PTR		pVersionMajor;
    CK_BYTE_PTR		pVersionMinor;
} CK_SSL3_MASTER_KEY_DERIVE_PARAMS;

typedef struct CK_SSL3_MASTER_KEY_DERIVE_PARAMS CK_PTR CK_SSL3_MASTER_KEY_DERIVE_PARAMS_PTR;


typedef struct CK_SSL3_KEY_MAT_OUT {
    CK_OBJECT_HANDLE	hClientMacSecret;
    CK_OBJECT_HANDLE	hServerMacSecret;
    CK_OBJECT_HANDLE	hClientKey;
    CK_OBJECT_HANDLE	hServerKey;
    CK_BYTE_PTR		pIVClient;
    CK_BYTE_PTR		pIVServer;
} CK_SSL3_KEY_MAT_OUT;

typedef CK_SSL3_KEY_MAT_OUT CK_PTR CK_SSL3_KEY_MAT_OUT_PTR;


typedef struct CK_SSL3_KEY_MAT_PARAMS {
    CK_ULONG		ulMacSizeInBits;
    CK_ULONG		ulKeySizeInBits;
    CK_ULONG		ulIVSizeInBits;
    CK_BBOOL		bIsExport;
    CK_SSL3_RANDOM_DATA	RandomInfo;
    CK_SSL3_KEY_MAT_OUT_PTR pReturnedKeyMaterial;
} CK_SSL3_KEY_MAT_PARAMS;

typedef CK_SSL3_KEY_MAT_PARAMS CK_PTR CK_SSL3_KEY_MAT_PARAMS_PTR;


/* CK_RV is a value that identifies the return value of a Cryptoki function. */
/* CK_RV was changed from CK_USHORT to CK_ULONG for v2.0 */
typedef CK_ULONG 	CK_RV;

#define CKR_OK                                0x00000000
#define CKR_CANCEL                            0x00000001
#define CKR_HOST_MEMORY                       0x00000002
#define CKR_SLOT_ID_INVALID                   0x00000003
#define CKR_FLAGS_INVALID                     0x00000004
#define CKR_ATTRIBUTE_READ_ONLY               0x00000010
#define CKR_ATTRIBUTE_SENSITIVE               0x00000011
#define CKR_ATTRIBUTE_TYPE_INVALID            0x00000012
#define CKR_ATTRIBUTE_VALUE_INVALID           0x00000013
#define CKR_DATA_INVALID                      0x00000020
#define CKR_DATA_LEN_RANGE                    0x00000021
#define CKR_DEVICE_ERROR                      0x00000030
#define CKR_DEVICE_MEMORY                     0x00000031
#define CKR_DEVICE_REMOVED                    0x00000032
#define CKR_ENCRYPTED_DATA_INVALID            0x00000040
#define CKR_ENCRYPTED_DATA_LEN_RANGE          0x00000041
#define CKR_FUNCTION_CANCELED                 0x00000050
#define CKR_FUNCTION_NOT_PARALLEL             0x00000051
#define CKR_FUNCTION_PARALLEL                 0x00000052

/* CKR_FUNCTION_PARALLEL_ACTIVE is new for v2.0 */
#define CKR_FUNCTION_PARALLEL_ACTIVE          0x00000053

#define CKR_KEY_HANDLE_INVALID                0x00000060
#define CKR_KEY_SENSITIVE                     0x00000061
#define CKR_KEY_SIZE_RANGE                    0x00000062
#define CKR_KEY_TYPE_INCONSISTENT             0x00000063

/* CKR_KEY_NOT_NEEDED and CKR_KEY_CHANGED are new for v2.0 */
#define CKR_KEY_NOT_NEEDED                    0x00000064
#define CKR_KEY_CHANGED                       0x00000065

#define CKR_MECHANISM_INVALID                 0x00000070
#define CKR_MECHANISM_PARAM_INVALID           0x00000071
#define CKR_OBJECT_CLASS_INCONSISTENT         0x00000080
#define CKR_OBJECT_CLASS_INVALID              0x00000081
#define CKR_OBJECT_HANDLE_INVALID             0x00000082
#define CKR_OPERATION_ACTIVE                  0x00000090
#define CKR_OPERATION_NOT_INITIALIZED         0x00000091
#define CKR_PIN_INCORRECT                     0x000000A0
#define CKR_PIN_INVALID                       0x000000A1
#define CKR_PIN_LEN_RANGE                     0x000000A2
#define CKR_SESSION_CLOSED                    0x000000B0
#define CKR_SESSION_COUNT                     0x000000B1
#define CKR_SESSION_EXCLUSIVE_EXISTS          0x000000B2
#define CKR_SESSION_HANDLE_INVALID            0x000000B3
#define CKR_SESSION_PARALLEL_NOT_SUPPORTED    0x000000B4
#define CKR_SESSION_READ_ONLY                 0x000000B5
#define CKR_SESSION_EXISTS                    0x000000B6
#define CKR_SIGNATURE_INVALID                 0x000000C0
#define CKR_SIGNATURE_LEN_RANGE               0x000000C1
#define CKR_TEMPLATE_INCOMPLETE               0x000000D0
#define CKR_TEMPLATE_INCONSISTENT             0x000000D1
#define CKR_TOKEN_NOT_PRESENT                 0x000000E0
#define CKR_TOKEN_NOT_RECOGNIZED              0x000000E1
#define CKR_TOKEN_WRITE_PROTECTED             0x000000E2
#define CKR_UNWRAPPING_KEY_HANDLE_INVALID     0x000000F0
#define CKR_UNWRAPPING_KEY_SIZE_RANGE         0x000000F1
#define CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT  0x000000F2
#define CKR_USER_ALREADY_LOGGED_IN            0x00000100
#define CKR_USER_NOT_LOGGED_IN                0x00000101
#define CKR_USER_PIN_NOT_INITIALIZED          0x00000102
#define CKR_USER_TYPE_INVALID                 0x00000103
#define CKR_WRAPPED_KEY_INVALID               0x00000110
#define CKR_WRAPPED_KEY_LEN_RANGE             0x00000112
#define CKR_WRAPPING_KEY_HANDLE_INVALID       0x00000113
#define CKR_WRAPPING_KEY_SIZE_RANGE           0x00000114
#define CKR_WRAPPING_KEY_TYPE_INCONSISTENT    0x00000115

/* These are new to v2.0 */
#define CKR_RANDOM_SEED_NOT_SUPPORTED         0x00000120
#define CKR_INVALID_PADDING_TYPE              0x00000130
#define CKR_INVALID_OID                       0x00000131
#define CKR_INSERTION_CALLBACK_SET            0x00000140
#define CKR_INSERTION_CALLBACK_NOT_SUPPORTED  0x00000141
#define CKR_BUFFER_TOO_SMALL                  0x00000150
#define CKR_SAVED_STATE_INVALID               0x00000160
#define CKR_GENERAL_ERROR                     0x00000170
#define CKR_FUNCTION_NOT_SUPPORTED            0x00000171

#define CKR_VENDOR_DEFINED                    0x80000000L


/* CK_NOTIFY is an application callback that processes events. */
typedef CK_RV (CK_ENTRY CK_PTR CK_NOTIFY)(
    CK_SESSION_HANDLE	hSession,	/* the session's handle */
    CK_NOTIFICATION	event,
    CK_VOID_PTR		pApplication	/* same as passed to C_OpenSession. */
);


/* CK_FUNCTION_LIST is going to be a structure holding a Cryptoki spec */
/* version and pointers of appropriate types to all the Cryptoki functions. */
/* CK_FUNCTION_LIST is new for v2.0 */
typedef struct CK_FUNCTION_LIST CK_FUNCTION_LIST;

typedef CK_FUNCTION_LIST CK_PTR	CK_FUNCTION_LIST_PTR;

typedef CK_FUNCTION_LIST_PTR CK_PTR CK_FUNCTION_LIST_PTR_PTR;


/* Do not attempt to use these. They are only used by NETSCAPE's internal
 * PKCS #11 interface. Most of these are place holders for other mechanism
 * and will change in the future.
 */
#define CKM_NETSCAPE_PBE_KEY_GEN		0x80000001L
#define CKM_NETSCAPE_PBE_SHA1_DES_CBC		0x80000002L
#define CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC	0x80000003L
#define CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC	0x80000004L
#define CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC	0x80000005L
#define CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4	0x80000006L
#define CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4	0x80000007L
#define CKM_NETSCAPE_PBE_SHA1_RC2_CBC		0x80000008L
#define CKM_NETSCAPE_PBE_SHA1_RC4		0x80000009L
#define CKM_NETSCAPE_NULL_ENCRYPT		0x8000000AL

/* XXX added for PKCS 12 -- PBE algorithms */
typedef struct CK_NETSCAPE_PBE_PARAMS {
    CK_CHAR_PTR		pInitVector;
    CK_CHAR_PTR		pPassword;
    CK_ULONG		ulPasswordLen;
    CK_CHAR_PTR		pSalt;
    CK_ULONG		ulSaltLen;
    CK_ULONG		ulIteration;
    CK_ULONG		ulEffectiveBits;	/* for variable key len algs */
} CK_NETSCAPE_PBE_PARAMS;

typedef CK_NETSCAPE_PBE_PARAMS CK_PTR CK_NETSCAPE_PBE_PARAMS_PTR;

#if defined(XP_WIN)
#pragma pack(pop, cryptoki)
#endif

#endif
