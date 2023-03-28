/*
 * This file implements PKCS 11 on top of our existing security modules
 *
 * For more information about PKCS 11 See PKCS 11 Token Inteface Standard.
 *   This implementation has two slots:
 *	slot 1 is our generic crypto support. It does not require login.
 *   It supports Public Key ops, and all they bulk ciphers and hashes. 
 *   It can also support Private Key ops for imported Private keys. It does 
 *   not have any token storage.
 *	slot 2 is our private key support. It requires a login before use. It
 *   can store Private Keys and Certs as token objects. Currently only private
 *   keys and their associated Certificates are saved on the token.
 *
 *   In this implementation, session objects are only visible to the session
 *   that created or generated them.
 */
#include "seccomon.h"
#include "crypto.h"
#include "pkcs11.h"
#include "pkcs11i.h"
#include "key.h"
#include "secitem.h"
#include "cert.h"
#include "secrng.h"
#include "sechash.h"
#include "secder.h"
#include "secpkcs5.h"
#include "pk11func.h"
#include "pkcs11t.h"
#include "secport.h"
#include "secoid.h"

#ifdef XP_MAC
#include "certdb.h"
#else
#include "../cert/certdb.h"
#endif
 
/*
 * ******************** Static data *******************************
 */

/* The next three strings must be exactly 32 characters long */
static char *manufacturerID      = "Netscape Communications Corp    ";
static char *libraryDescription  = "Communicator Internal Crypto Svc";
static char *tokDescription      = "Communicator Generic Crypto Svcs";
static char *privTokDescription  = "Communicator Certificate DB     ";
/* The next two strings must be exactly 64 characters long, with the
   first 32 characters meaningful  */
static char *slotDescription     = 
	"Communicator Internal Cryptographic Services Version 4.0        ";
static char *privSlotDescription = 
	"Communicator User Private Key and Certificate Services          ";


#define NETSCAPE_SLOT_ID 1
#define PRIVATE_KEY_SLOT_ID 2

static PK11Slot pk11_slot[2];
#define __PASTE(x,y)    x##y
 
 
#undef CK_FUNC
#undef CK_EXTERN
#undef CK_NEED_ARG_LIST
#undef _CK_RV
 
 
/* build the crypto module table */
static CK_FUNCTION_LIST pk11_funcList = {
    { 1, 10 },
 
#undef CK_FUNC
#undef CK_EXTERN
#undef CK_NEED_ARG_LIST
#undef _CK_RV
 
#define CK_EXTERN
#define CK_FUNC(name) name,
#define _CK_RV
 
#include "pkcs11f.h"
 
};
 
#undef CK_FUNC
#undef CK_EXTERN
#undef _CK_RV
 
 
#undef __PASTE

/* List of DES Weak Keys */ 
typedef unsigned char desKey[8];
static desKey  pk11_desWeakTable[] = {
#ifdef noParity
    /* weak */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1e, 0x1e, 0x1e, 0x1e, 0x0e, 0x0e, 0x0e, 0x0e },
    { 0xe0, 0xe0, 0xe0, 0xe0, 0xf0, 0xf0, 0xf0, 0xf0 },
    { 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe },
    /* semi-weak */
    { 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe },
    { 0xfe, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0xfe },

    { 0x1e, 0xe0, 0x1e, 0xe0, 0x0e, 0xf0, 0x0e, 0xf0 },
    { 0xe0, 0x1e, 0xe0, 0x1e, 0xf0, 0x0e, 0xf0, 0x0e },

    { 0x00, 0xe0, 0x00, 0xe0, 0x00, 0x0f, 0x00, 0x0f },
    { 0xe0, 0x00, 0xe0, 0x00, 0xf0, 0x00, 0xf0, 0x00 },

    { 0x1e, 0xfe, 0x1e, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe },
    { 0xfe, 0x1e, 0xfe, 0x1e, 0xfe, 0x0e, 0xfe, 0x0e },

    { 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x0e },
    { 0x1e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x0e, 0x00 },

    { 0xe0, 0xfe, 0xe0, 0xfe, 0xf0, 0xfe, 0xf0, 0xfe },
    { 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf0, 0xfe, 0xf0 },
#else
    /* weak */
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0x1f, 0x1f, 0x1f, 0x1f, 0x0e, 0x0e, 0x0e, 0x0e },
    { 0xe0, 0xe0, 0xe0, 0xe0, 0xf1, 0xf1, 0xf1, 0xf1 },
    { 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe },

    /* semi-weak */
    { 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe },
    { 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01 },

    { 0x1f, 0xe0, 0x1f, 0xe0, 0x0e, 0xf1, 0x0e, 0xf1 },
    { 0xe0, 0x1f, 0xe0, 0x1f, 0xf1, 0x0e, 0xf1, 0x0e },

    { 0x01, 0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1 },
    { 0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1, 0x01 },

    { 0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe },
    { 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e },

    { 0x01, 0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e },
    { 0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e, 0x01 },

    { 0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1, 0xfe },
    { 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1 }
#endif
};

    
static int pk11_desWeakTableSize = sizeof(pk11_desWeakTable)/
						sizeof(pk11_desWeakTable[0]);

/* DES KEY Parity conversion table. Takes each byte/2 as an index, returns
 * that byte with the proper parity bit set */
static unsigned char parityTable[256] = {
/* Even...0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e */
/* E */   0x01,0x02,0x04,0x07,0x08,0x0b,0x0d,0x0e,
/* Odd....0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e */
/* O */   0x10,0x13,0x15,0x16,0x19,0x1a,0x1c,0x1f,
/* Odd....0x20,0x22,0x24,0x26,0x28,0x2a,0x2c,0x2e */
/* O */   0x20,0x23,0x25,0x26,0x29,0x2a,0x2c,0x2f,
/* Even...0x30,0x32,0x34,0x36,0x38,0x3a,0x3c,0x3e */
/* E */   0x31,0x32,0x34,0x37,0x38,0x3b,0x3d,0x3e,
/* Odd....0x40,0x42,0x44,0x46,0x48,0x4a,0x4c,0x4e */
/* O */   0x40,0x43,0x45,0x46,0x49,0x4a,0x4c,0x4f,
/* Even...0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,0x5e */
/* E */   0x51,0x52,0x54,0x57,0x58,0x5b,0x5d,0x5e,
/* Even...0x60,0x62,0x64,0x66,0x68,0x6a,0x6c,0x6e */
/* E */   0x61,0x62,0x64,0x67,0x68,0x6b,0x6d,0x6e,
/* Odd....0x70,0x72,0x74,0x76,0x78,0x7a,0x7c,0x7e */
/* O */   0x70,0x73,0x75,0x76,0x79,0x7a,0x7c,0x7f,
/* Odd....0x80,0x82,0x84,0x86,0x88,0x8a,0x8c,0x8e */
/* O */   0x80,0x83,0x85,0x86,0x89,0x8a,0x8c,0x8f,
/* Even...0x90,0x92,0x94,0x96,0x98,0x9a,0x9c,0x9e */
/* E */   0x91,0x92,0x94,0x97,0x98,0x9b,0x9d,0x9e,
/* Even...0xa0,0xa2,0xa4,0xa6,0xa8,0xaa,0xac,0xae */
/* E */   0xa1,0xa2,0xa4,0xa7,0xa8,0xab,0xad,0xae,
/* Odd....0xb0,0xb2,0xb4,0xb6,0xb8,0xba,0xbc,0xbe */
/* O */   0xb0,0xb3,0xb5,0xb6,0xb9,0xba,0xbc,0xbf,
/* Even...0xc0,0xc2,0xc4,0xc6,0xc8,0xca,0xcc,0xce */
/* E */   0xc1,0xc2,0xc4,0xc7,0xc8,0xcb,0xcd,0xce,
/* Odd....0xd0,0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde */
/* O */   0xd0,0xd3,0xd5,0xd6,0xd9,0xda,0xdc,0xdf,
/* Odd....0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xee */
/* O */   0xe0,0xe3,0xe5,0xe6,0xe9,0xea,0xec,0xef,
/* Even...0xf0,0xf2,0xf4,0xf6,0xf8,0xfa,0xfc,0xfe */
/* E */   0xf1,0xf2,0xf4,0xf7,0xf8,0xfb,0xfd,0xfe,
};

/* Mechanisms */
struct mechanismList {
    CK_MECHANISM_TYPE	type;
    CK_MECHANISM_INFO	domestic;
    PRBool		privkey;
};

static struct mechanismList mechanisms[] = {
     {CKM_RSA_PKCS_KEY_PAIR_GEN,{128,2048,CKF_GENERATE_KEY_PAIR}, PR_TRUE},
     {CKM_RSA_PKCS,{128,2048,CKF_ENCRYPT|CKF_DECRYPT|
		CKF_SIGN|CKF_SIGN_RECOVER|CKF_VERIFY|
		CKF_VERIFY_RECOVER|CKF_UNWRAP|CKF_WRAP}, PR_TRUE},
     /*{CKM_RSA_9796,{128,2048,CKF_ENCRYPT|CKF_DECRYPT|
		CKF_SIGN|CKF_SIGN_RECOVER|CKF_VERIFY|
		CKF_VERIFY_RECOVER|CKF_UNWRAP|CKF_WRAP}, PR_TRUE}, */
     {CKM_RSA_X_509,{128,2048,CKF_ENCRYPT|CKF_DECRYPT|
		CKF_SIGN|CKF_SIGN_RECOVER|CKF_VERIFY|
		CKF_VERIFY_RECOVER|CKF_UNWRAP|CKF_WRAP}, PR_TRUE}, 
     {CKM_DSA,{512,1024,CKF_SIGN|CKF_VERIFY}, PR_TRUE},
     {CKM_DSA_KEY_PAIR_GEN,{512,1024,CKF_GENERATE_KEY_PAIR}, PR_TRUE},
     /* no diffie hellman yet */
     /*{CKM_DH,{128,1024,CKF_DERIVE}, PR_TRUE}, */
     {CKM_RC2_KEY_GEN,{1,128,CKF_GENERATE}, PR_FALSE},
     {CKM_RC2_ECB,{1,128,CKF_ENCRYPT|CKF_DECRYPT|CKF_WRAP|CKF_UNWRAP},
								 PR_FALSE},
     {CKM_RC2_CBC,{1,128,CKF_ENCRYPT|CKF_DECRYPT}, PR_FALSE},
     {CKM_RC4_KEY_GEN,{1,256,CKF_GENERATE}, PR_FALSE},
     {CKM_RC4,{1,256,CKF_ENCRYPT|CKF_DECRYPT|CKF_WRAP|CKF_UNWRAP}, PR_FALSE},
     {CKM_DES_KEY_GEN,{64,64,CKF_GENERATE}, PR_FALSE},
     {CKM_DES_ECB,{64,64,CKF_ENCRYPT|CKF_DECRYPT|CKF_WRAP|CKF_UNWRAP},
								   PR_FALSE},
     {CKM_DES_CBC,{64,64,CKF_ENCRYPT|CKF_DECRYPT}, PR_FALSE},
     {CKM_DES2_KEY_GEN,{192,192,CKF_GENERATE}, PR_FALSE},
     {CKM_DES3_KEY_GEN,{192,192,CKF_GENERATE}, PR_FALSE},
     {CKM_DES3_ECB,{192,192,CKF_ENCRYPT|CKF_DECRYPT|CKF_WRAP|CKF_UNWRAP},
								   PR_FALSE},
     {CKM_DES3_CBC,{192,192,CKF_ENCRYPT|CKF_DECRYPT}, PR_FALSE},
     /* no Block encryption mac'ing */
     /*{CKM_RC2_MAC,{1,128,0}, PR_FALSE},*/
     /*{CKM_DES_MAC,{64,64,0}, PR_FALSE},*/
     /*{CKM_DES3_MAC,{192,192,0}, PR_FALSE},*/
     {CKM_MD2,{0,0,CKF_DIGEST}, PR_FALSE},
     {CKM_MD5,{0,0,CKF_DIGEST}, PR_FALSE},
     {CKM_SHA_1,{0,0,CKF_DIGEST}, PR_FALSE},
     /* SSL specific */
     /* Netscape specific */
     {CKM_PBE_MD2_DES_CBC,{64,64,CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_MD5_DES_CBC,{64,64,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_DES_CBC,{64,64,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC,{192,192,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC,{40,40,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC,{128,128,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_RC2_CBC,{1,128,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4,{40,40,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4,{128,128,CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_RC4,{1,128,CKF_GENERATE}, PR_TRUE},
};
static CK_ULONG mechanismCount = sizeof(mechanisms)/sizeof(mechanisms[0]);


/*
 * Configuration utils
 */
void
PK11_ConfigurePKCS11(char *man, char *libdes, char *tokdes, char *ptokdes,
	char *slotdes, char *pslotdes, char *fslotdes, char *fpslotdes) 
{

    /* make sure the internationalization was done correctly... */
    if (man && (PORT_Strlen(man) == 33)) {
	manufacturerID = man;
    }
    if (libdes && (PORT_Strlen(libdes) == 33)) {
	libraryDescription = libdes;
    }
    if (tokdes && (PORT_Strlen(tokdes) == 33)) {
	tokDescription = tokdes;
    }
    if (ptokdes && (PORT_Strlen(ptokdes) == 33)) {
	privTokDescription = ptokdes;
    }
    if (slotdes && (PORT_Strlen(slotdes) == 65)) {
	slotDescription = slotdes;
    }
    if (pslotdes && (PORT_Strlen(pslotdes) == 65)) {
	privSlotDescription = pslotdes;
    }

    PK11_ConfigureFIPS(fslotdes,fpslotdes);

    return;
}

/*
 * ******************** Password Utilities *******************************
 */

/* Handle to give the password to the database. user arg should be a pointer
 * to the slot. */
static SECItem *pk11_givePass(void *sp,SECKEYKeyDBHandle *handle) {
    PK11Slot *slot = (PK11Slot *)sp;

    if (slot->password == NULL) return NULL;

    return SECITEM_DupItem(slot->password);
}

static void pk11_FreePrivKey(SECKEYLowPrivateKey *key, PRBool freeit) {
    SECKEY_LowDestroyPrivateKey(key);
}

static void pk11_Null(void *data, PRBool freeit) {
    return;
}

/*
 * see if the key DB password is enabled
 */
PRBool
pk11_hasNullPassword(SECItem **pwitem)
{
    PRBool pwenabled;
    SECKEYKeyDBHandle *keydb;
    
    keydb = SECKEY_GetDefaultKeyDB();
    pwenabled = PR_FALSE;
    *pwitem = NULL;
    if (SECKEY_HasKeyDBPassword (keydb) == SECSuccess) {
	*pwitem = SECKEY_HashPassword("", keydb->global_salt);
	if ( *pwitem ) {
	    if (SECKEY_CheckKeyDBPassword (keydb, *pwitem) == SECSuccess) {
		pwenabled = PR_TRUE;
	    } else {
	    	SECITEM_ZfreeItem(*pwitem, PR_TRUE);
		*pwitem = NULL;
	    }
	}
    }

    return pwenabled;
}

/*
 * ******************** Attribute Utilities *******************************
 */

/*
 * create a new attribute with type, value, and length. Space is allocated
 * to hold value.
 */
static PK11Attribute *
pk11_NewAttribute(CK_ATTRIBUTE_TYPE type, CK_VOID_PTR *value, CK_ULONG len) {
    PK11Attribute *attribute;

    attribute = PORT_Alloc(sizeof(PK11Attribute));
    if (attribute == NULL) return NULL;

    attribute->attrib.type = type;
    if (value) {
	attribute->attrib.pValue = PORT_Alloc(len);
	if (attribute->attrib.pValue == NULL) {
	    PORT_Free(attribute);
	    return NULL;
	}
	PORT_Memcpy(attribute->attrib.pValue,value,len);
	attribute->attrib.ulValueLen = len;
    } else {
	attribute->attrib.pValue = NULL;
	attribute->attrib.ulValueLen = 0;
    }
    attribute->handle = type;
    attribute->next = attribute->prev = NULL;
    attribute->refCount = 1;
#ifdef PKCS11_USE_THREADS
    attribute->refLock = PR_NewLock(0);
    if (attribute->refLock == NULL) {
	if (attribute->attrib.pValue) PORT_Free(attribute->attrib.pValue);
	PORT_Free(attribute);
	return NULL;
    }
#else
    attribute->refLock = NULL;
#endif
    return attribute;
}

/*
 * Free up all the memory associated with an attribute. Reference count
 * must be zero to call this.
 */
static void
pk11_DestroyAttribute(PK11Attribute *attribute) {
    PORT_Assert(attribute->refCount == 0);
    PK11_USE_THREADS(USE_THREADS(PR_DestroyLock(attribute->refLock);))
    if (attribute->attrib.pValue) {
	 /* clear out the data in the attribute value... it may have been
	  * sensitive data */
	 PORT_Memset(attribute->attrib.pValue,0,attribute->attrib.ulValueLen);
	 PORT_Free(attribute->attrib.pValue);
    }
    PORT_Free(attribute);
}
    
    
/*
 * look up and attribute structure from a type and Object structure.
 * The returned attribute is referenced and needs to be freed when 
 * it is no longer needed.
 */
static PK11Attribute *
pk11_FindAttribute(PK11Object *object,CK_ATTRIBUTE_TYPE type) {
    PK11Attribute *attribute;

    PK11_USE_THREADS(PR_Lock(object->attributeLock);)
    pk11queue_find(attribute,type,object->head,HASH_SIZE);
    if (attribute) {
	/* atomic increment would be nice here */
	PK11_USE_THREADS(PR_Lock(attribute->refLock);)
	attribute->refCount++;
	PK11_USE_THREADS(PR_Unlock(attribute->refLock);)
    }
    PK11_USE_THREADS(PR_Unlock(object->attributeLock);)

    return(attribute);
}


/*
 * release a reference to an attribute structure
 */
static void
pk11_FreeAttribute(PK11Attribute *attribute) {
    PRBool destroy = PR_FALSE;

    PK11_USE_THREADS(PR_Lock(attribute->refLock);)
    if (attribute->refCount == 1) destroy = PR_TRUE;
    attribute->refCount--;
    PK11_USE_THREADS(PR_Unlock(attribute->refLock);)

    if (destroy) pk11_DestroyAttribute(attribute);
}


/*
 * return true if object has attribute
 */
static PRBool
pk11_hasAttribute(PK11Object *object,CK_ATTRIBUTE_TYPE type) {
    PK11Attribute *attribute;

    PK11_USE_THREADS(PR_Lock(object->attributeLock);)
    pk11queue_find(attribute,type,object->head,HASH_SIZE);
    PK11_USE_THREADS(PR_Unlock(object->attributeLock);)

    return (attribute != NULL);
}

/*
 * add an attribute to an object
 */
static
void pk11_AddAttribute(PK11Object *object,PK11Attribute *attribute) {
    PK11_USE_THREADS(PR_Lock(object->attributeLock);)
    pk11queue_add(attribute,attribute->handle,object->head,HASH_SIZE);
    PK11_USE_THREADS(PR_Unlock(object->attributeLock);)
}

/*
 * delete an attribute from an object
 */
static void
pk11_DeleteAttribute(PK11Object *object, PK11Attribute *attribute) {
    PK11_USE_THREADS(PR_Lock(object->attributeLock);)
    if (attribute->next || attribute->prev) {
	pk11queue_delete(attribute,attribute->handle,
						object->head,HASH_SIZE);
    }
    PK11_USE_THREADS(PR_Unlock(object->attributeLock);)
    pk11_FreeAttribute(attribute);
}

/*
 * this is only valid for CK_BBOOL type attributes. Return the state
 * of that attribute.
 */
static PRBool
pk11_isTrue(PK11Object *object,CK_ATTRIBUTE_TYPE type) {
    PK11Attribute *attribute;
    PRBool tok = PR_FALSE;

    attribute=pk11_FindAttribute(object,type);
    if (attribute == NULL) { return PR_FALSE; }
    tok = *(CK_BBOOL *)attribute->attrib.pValue;
    pk11_FreeAttribute(attribute);

    return tok;
}

/*
 * force an attribute to null.
 * this is for sensitive keys which are stored in the database, we don't
 * want to keep this info around in memory in the clear.
 */
static void
pk11_nullAttribute(PK11Object *object,CK_ATTRIBUTE_TYPE type) {
    PK11Attribute *attribute;

    attribute=pk11_FindAttribute(object,type);
    if (attribute == NULL) return;

    if (attribute->attrib.pValue != NULL) {
	PORT_Memset(attribute->attrib.pValue,0,attribute->attrib.ulValueLen);
	PORT_Free(attribute->attrib.pValue);
	attribute->attrib.pValue = NULL;
	attribute->attrib.ulValueLen = 0;
    }
    pk11_FreeAttribute(attribute);
}

/*
 * return a null terminated string from attribute 'type'. This string
 * is allocated and needs to be freed with PORT_Free() When complete.
 */
static char *
pk11_getString(PK11Object *object,CK_ATTRIBUTE_TYPE type) {
    PK11Attribute *attribute;
    char *label = NULL;

    attribute=pk11_FindAttribute(object,type);
    if (attribute == NULL) return NULL;

    if (attribute->attrib.pValue != NULL) {
	label = (char *) PORT_Alloc(attribute->attrib.ulValueLen+1);
	if (label == NULL) {
    	    pk11_FreeAttribute(attribute);
	    return NULL;
	}

	PORT_Memcpy(label,attribute->attrib.pValue,
						attribute->attrib.ulValueLen);
	label[attribute->attrib.ulValueLen] = 0;
    }
    pk11_FreeAttribute(attribute);
    return label;
}

/*
 * decode when a particular attribute may be modified
 * 	PK11_NEVER: This attribute must be set at object creation time and
 *  can never be modified.
 *	PK11_ONCOPY: This attribute may be modified only when you copy the
 *  object.
 *	PK11_SENSITIVE: The CKA_SENSITIVE attribute can only be changed from
 *  FALSE to TRUE.
 *	PK11_ALWAYS: This attribute can always be modified.
 * Some attributes vary their modification type based on the class of the 
 *   object.
 */
PK11ModifyType
pk11_modifyType(CK_ATTRIBUTE_TYPE type, CK_OBJECT_CLASS class) {
    /* if we don't know about it, user user defined, always allow modify */
    PK11ModifyType mtype = PK11_ALWAYS; 

    switch(type) {
    /* NEVER */
    case CKA_CLASS:
    case CKA_CERTIFICATE_TYPE:
    case CKA_KEY_TYPE:
    case CKA_MODULUS:
    case CKA_MODULUS_BITS:
    case CKA_PUBLIC_EXPONENT:
    case CKA_PRIVATE_EXPONENT:
    case CKA_PRIME:
    case CKA_SUBPRIME:
    case CKA_BASE:
    case CKA_PRIME_1:
    case CKA_PRIME_2:
    case CKA_EXPONENT_1:
    case CKA_EXPONENT_2:
    case CKA_COEFFICIENT:
    case CKA_VALUE_LEN:
	mtype = PK11_NEVER;
	break;

    /* ONCOPY */
    case CKA_TOKEN:
    case CKA_PRIVATE:
	mtype = PK11_ONCOPY;
	break;

    /* SENSITIVE */
    case CKA_SENSITIVE:
	mtype = PK11_SENSITIVE;
	break;

    /* ALWAYS */
    case CKA_LABEL:
    case CKA_APPLICATION:
    case CKA_ID:
    case CKA_SERIAL_NUMBER:
    case CKA_START_DATE:
    case CKA_END_DATE:
    case CKA_DERIVE:
    case CKA_ENCRYPT:
    case CKA_DECRYPT:
    case CKA_SIGN:
    case CKA_VERIFY:
    case CKA_SIGN_RECOVER:
    case CKA_VERIFY_RECOVER:
    case CKA_WRAP:
    case CKA_UNWRAP:
	mtype = PK11_ALWAYS;
	break;

    /* DEPENDS ON CLASS */
    case CKA_VALUE:
	mtype = (class == CKO_DATA) ? PK11_ALWAYS : PK11_NEVER;
	break;

    case CKA_SUBJECT:
	mtype = (class == CKO_CERTIFICATE) ? PK11_NEVER : PK11_ALWAYS;
	break;
    default:
	break;
    }
    return mtype;
}

/* decode if a particular attribute is sensitive (cannot be read
 * back to the user of if the object is set to SENSITIVE) */
PRBool
pk11_isSensitive(CK_ATTRIBUTE_TYPE type, CK_OBJECT_CLASS class) {
    switch(type) {
    /* ALWAYS */
    case CKA_PRIVATE_EXPONENT:
    case CKA_PRIME_1:
    case CKA_PRIME_2:
    case CKA_EXPONENT_1:
    case CKA_EXPONENT_2:
    case CKA_COEFFICIENT:
	return PR_TRUE;

    /* DEPENDS ON CLASS */
    case CKA_VALUE:
	/* PRIVATE and SECRET KEYS have SENSITIVE values */
	return ((class == CKO_PRIVATE_KEY) || (class == CKO_SECRET_KEY));

    default:
	break;
    }
    return PR_FALSE;
}

/* 
 * copy an attribute into a SECItem. Secitem is allocated in the specified
 * arena.
 */
static CK_RV
pk11_Attribute2SecItem(PRArenaPool *arena,SECItem *item,PK11Object *object,
					CK_ATTRIBUTE_TYPE type) {
    int len;
    PK11Attribute *attribute;

    attribute = pk11_FindAttribute(object, type);
    if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;
    len = attribute->attrib.ulValueLen;

    if (arena) {
    	item->data = (unsigned char *) PORT_ArenaAlloc(arena,len);
    } else {
    	item->data = (unsigned char *) PORT_Alloc(len);
    }
    if (item->data == NULL) {
	pk11_FreeAttribute(attribute);
	return CKR_HOST_MEMORY;
    }
    item->len = len;
    PORT_Memcpy(item->data,attribute->attrib.pValue, len);
    pk11_FreeAttribute(attribute);
    return CKR_OK;
}

static void
pk11_DeleteAttributeType(PK11Object *object,CK_ATTRIBUTE_TYPE type) {
    PK11Attribute *attribute;
    attribute = pk11_FindAttribute(object, type);
    if (attribute == NULL) return ;
    pk11_DeleteAttribute(object,attribute);
}

static CK_RV
pk11_AddAttributeType(PK11Object *object,CK_ATTRIBUTE_TYPE type,void *valPtr,
							CK_ULONG length) {
    PK11Attribute *attribute;
    attribute = pk11_NewAttribute(type,valPtr,length);
    if (attribute == NULL) { return CKR_HOST_MEMORY; }
    pk11_AddAttribute(object,attribute);
    return CKR_OK;
}

/*
 * ******************** Object Utilities *******************************
 */

/*
 * Create a new object
 */
static PK11Object *
pk11_NewObject(PK11Slot *slot) {
    PK11Object *object;
    int i;

    object = PORT_Alloc(sizeof(PK11Object));
    if (object == NULL) return NULL;

    object->handle = 0;
    object->next = object->prev = NULL;
    object->sessionList.next = NULL;
    object->sessionList.prev = NULL;
    object->sessionList.parent = object;
    object->inDB = PR_FALSE;
    object->label = NULL;
    object->refCount = 1;
    object->session = NULL;
    object->slot = slot;
    object->class = 0xffff;
#ifdef PKCS11_USE_THREADS
    object->refLock = PR_NewLock(0);
    if (object->refLock == NULL) {
	PORT_Free(object);
	return NULL;
    }
    object->attributeLock = PR_NewLock(0);
    if (object->attributeLock == NULL) {
	PK11_USE_THREADS(PR_DestroyLock(object->refLock);)
	PORT_Free(object);
	return NULL;
    }
#else
    object->attributeLock = NULL;
    object->refLock = NULL;
#endif
    for (i=0; i < HASH_SIZE; i++) {
	object->head[i] = NULL;
    }
    object->objectInfo = NULL;
    object->infoFree = NULL;
    return object;
}

/*
 * free all the data associated with an object. Object reference count must
 * be 'zero'.
 */
static CK_RV
pk11_DestroyObject(PK11Object *object) {
    int i;
    SECItem pubKey;
    CK_RV crv = CKR_OK;
    SECStatus rv;
    PORT_Assert(object->refCount == 0);

    /* delete the database value */
    if (object->inDB) {
	if (pk11_isToken(object->handle)) {
	/* remove the objects from the real data base */
	    switch (object->handle & PK11_TOKEN_TYPE_MASK) {
	      case PK11_TOKEN_TYPE_PRIV:
		/* KEYID is the public KEY for DSA and DH, and the MODULUS for
		 *  RSA */
		crv=pk11_Attribute2SecItem(NULL,&pubKey,object,CKA_ID);
		if (crv != CKR_OK) break;
		rv = SECKEY_DeleteKey(SECKEY_GetDefaultKeyDB(), &pubKey);
		if (rv != SECSuccess) crv= CKR_DEVICE_ERROR;
		break;
	      case PK11_TOKEN_TYPE_CERT:
		rv = SEC_DeletePermCertificate((CERTCertificate *)object->objectInfo);
		if (rv != SECSuccess) crv = CKR_DEVICE_ERROR;
		break;
	    }
	}
    } 
    if (object->label) PORT_Free(object->label);

    /* clean out the attributes */
    /* since no one is referencing us, it's safe to walk the chain
     * without a lock */
    for (i=0; i < HASH_SIZE; i++) {
	PK11Attribute *ap,*next;
	for (ap = object->head[i]; ap != NULL; ap = next) {
	    next = ap->next;
	    /* paranoia */
	    ap->next = ap->prev = NULL;
	    pk11_FreeAttribute(ap);
	}
	object->head[i] = NULL;
    }
    PK11_USE_THREADS(PR_DestroyLock(object->attributeLock);)
    PK11_USE_THREADS(PR_DestroyLock(object->refLock);)
    if (object->objectInfo) {
	(*object->infoFree)(object->objectInfo);
    }
    PORT_Free(object);
    return crv;
}

    
/*
 * look up and object structure from a handle. OBJECT_Handles only make
 * sense in terms of a given session.  make a reference to that object
 * structure returned.
 */
static PK11Object * pk11_ObjectFromHandle(CK_OBJECT_HANDLE handle, 
						PK11Session *session) {
    PK11Object **head;
    PRLock *lock;
    PK11Slot *slot = pk11_SlotFromSession(session);
    PK11Object *object;

    /*
     * Token objects are stored in the slot. Session objects are stored
     * with the session.
     */
    head = slot->tokObjects;
    lock = slot->objectLock;

    PK11_USE_THREADS(PR_Lock(lock);)
    pk11queue_find(object,handle,head,HASH_SIZE);
    if (object) {
	PK11_USE_THREADS(PR_Lock(object->refLock);)
	object->refCount++;
	PK11_USE_THREADS(PR_Unlock(object->refLock);)
    }
    PK11_USE_THREADS(PR_Unlock(lock);)

    return(object);
}

/*
 * release a reference to an object handle
 */
static PK11FreeStatus
pk11_FreeObject(PK11Object *object) {
    PRBool destroy = PR_FALSE;
    CK_RV crv;

    PK11_USE_THREADS(PR_Lock(object->refLock);)
    if (object->refCount == 1) destroy = PR_TRUE;
    object->refCount--;
    PK11_USE_THREADS(PR_Unlock(object->refLock);)

    if (destroy) {
	crv = pk11_DestroyObject(object);
	if (crv != CKR_OK) {
	   return PK11_DestroyFailure;
	} 
	return PK11_Destroyed;
    }
    return PK11_Busy;
}
 
/*
 * add an object to a slot and session queue
 */
static
void pk11_AddSlotObject(PK11Slot *slot, PK11Object *object) {
    PK11_USE_THREADS(PR_Lock(slot->objectLock);)
    pk11queue_add(object,object->handle,slot->tokObjects,HASH_SIZE);
    PK11_USE_THREADS(PR_Unlock(slot->objectLock);)
}

static
void pk11_AddObject(PK11Session *session, PK11Object *object) {
    PK11Slot *slot = pk11_SlotFromSession(session);

    if (!pk11_isToken(object->handle)) {
	PK11_USE_THREADS(PR_Lock(session->objectLock);)
	pk11queue_add(&object->sessionList,0,session->objects,0);
	PK11_USE_THREADS(PR_Unlock(session->objectLock);)
    }
    pk11_AddSlotObject(slot,object);
} 

/*
 * add an object to a slot andsession queue
 */
static
void pk11_DeleteObject(PK11Session *session, PK11Object *object) {
    PK11Slot *slot = pk11_SlotFromSession(session);

    if (!pk11_isToken(object->handle)) {
	PK11_USE_THREADS(PR_Lock(session->objectLock);)
	pk11queue_delete(&object->sessionList,0,session->objects,0);
	PK11_USE_THREADS(PR_Unlock(session->objectLock);)
    }
    PK11_USE_THREADS(PR_Lock(slot->objectLock);)
    pk11queue_delete(object,object->handle,slot->tokObjects,HASH_SIZE);
    PK11_USE_THREADS(PR_Unlock(slot->objectLock);)
    pk11_FreeObject(object);
}

/*
 * copy the attributes from one object to another. Don't overwrite existing
 * attributes. NOTE: This is a pretty expensive operation since it
 * grabs the attribute locks for the src object for a *long* time.
 */
static CK_RV
pk11_CopyObject(PK11Object *destObject,PK11Object *srcObject) {
    PK11Attribute *attribute;
    int i;

    PK11_USE_THREADS(PR_Lock(srcObject->attributeLock);)
    for(i=0; i < HASH_SIZE; i++) {
	attribute = srcObject->head[i];
	do {
	    if (attribute) {
		if (!pk11_hasAttribute(destObject,attribute->handle)) {
		    /* we need to copy the attribute since each attribute
		     * only has one set of link list pointers */
		    PK11Attribute *newAttribute = pk11_NewAttribute(
				pk11_attr_expand(&attribute->attrib));
		    if (newAttribute == NULL) {
			PK11_USE_THREADS(PR_Unlock(srcObject->attributeLock);)
			return CKR_HOST_MEMORY;
		    }
		    pk11_AddAttribute(destObject,newAttribute);
		}
		attribute=attribute->next;
	    }
	} while (attribute != NULL);
    }
    PK11_USE_THREADS(PR_Unlock(srcObject->attributeLock);)
    return CKR_OK;
}

/*
 * ******************** Search Utilities *******************************
 */

/* add an object to a search list */
CK_RV
AddToList(PK11ObjectListElement **list,PK11Object *object) {
     PK11ObjectListElement *new = 
	(PK11ObjectListElement *)PORT_Alloc(sizeof(PK11ObjectListElement));

     if (new == NULL) return CKR_HOST_MEMORY;

     new->next = *list;
     new->object = object;
     PK11_USE_THREADS(PR_Lock(object->refLock);)
     object->refCount++;
     PK11_USE_THREADS(PR_Unlock(object->refLock);)

    *list = new;
    return CKR_OK;
}


/* return true if the object matches the template */
PRBool
pk11_objectMatch(PK11Object *object,CK_ATTRIBUTE_PTR template,int count) {
    int i;

    for (i=0; i < count; i++) {
	PK11Attribute *attribute = pk11_FindAttribute(object,template[i].type);
	if (attribute == NULL) {
	    return PR_FALSE;
	}
	if (attribute->attrib.ulValueLen == template[i].ulValueLen) {
	    if (PORT_Memcmp(attribute->attrib.pValue,template[i].pValue,
					template[i].ulValueLen) == 0) {
		continue;
	    }
	}
        pk11_FreeAttribute(attribute);
	return PR_FALSE;
    }
    return PR_TRUE;
}

/* search through all the objects in the queue and return the template matches
 * in the object list.
 */
CK_RV
pk11_searchObjectList(PK11ObjectListElement **objectList,PK11Object **head,
			 PRLock *lock, CK_ATTRIBUTE_PTR template, int count) {
    int i;
    PK11Object *object;
    CK_RV rv;

    for(i=0; i < HASH_SIZE; i++) {
        /* sigh we need to hold the lock to copy a consistant version of
         * the linked list. */
        PK11_USE_THREADS(PR_Lock(lock);)
	for (object = head[i]; object != NULL; object= object->next) {
	    if (pk11_objectMatch(object,template,count)) {
	        rv = AddToList(objectList,object);
		if (rv != CKR_OK) {
		    return rv;
		}
	    }
	}
        PK11_USE_THREADS(PR_Unlock(lock);)
    }
    return CKR_OK;
}

/*
 * free a single list element. Return the Next object in the list.
 */
PK11ObjectListElement *
pk11_FreeObjectListElement(PK11ObjectListElement *objectList) {
    PK11ObjectListElement *ol = objectList->next;

    pk11_FreeObject(objectList->object);
    PORT_Free(objectList);
    return ol;
}

/* free an entire object list */
void
pk11_FreeObjectList(PK11ObjectListElement *objectList) {
    PK11ObjectListElement *ol;

    for (ol= objectList; ol != NULL; ol = pk11_FreeObjectListElement(ol)) {}
}

/*
 * free a search structure
 */
void
pk11_FreeSearch(PK11SearchResults *search) {
    if (search->handles) {
	PORT_Free(search->handles);
    }
    PORT_Free(search);
}

/*
 * ******************** Session Utilities *******************************
 */

/* update the sessions state based in it's flags and wether or not it's
 * logged in */
static void
pk11_update_state(PK11Slot *slot,PK11Session *session) {
    if (slot->isLoggedIn) {
	if (slot->ssoLoggedIn) {
    	    session->info.state = CKS_RW_SO_FUNCTIONS;
	} else if (session->info.flags & CKF_RW_SESSION) {
    	    session->info.state = CKS_RW_USER_FUNCTIONS;
	} else {
    	    session->info.state = CKS_RO_USER_FUNCTIONS;
	}
    } else {
	if (session->info.flags & CKF_RW_SESSION) {
    	    session->info.state = CKS_RW_PUBLIC_SESSION;
	} else {
    	    session->info.state = CKS_RO_PUBLIC_SESSION;
	}
    }
}

/* update the state of all the sessions on a slot */
static void
pk11_update_all_states(PK11Slot *slot) {
    int i;
    PK11Session *session;

    for (i=0; i < SESSION_HASH_SIZE; i++) {
	PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
	for (session = slot->head[i]; session; session = session->next) {
	    pk11_update_state(slot,session);
	}
	PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)
    }
}

/*
 * context are cipher and digest contexts that are associated with a session
 */
static void
pk11_FreeContext(PK11SessionContext *context) {
    if (context->cipherInfo) {
	(*context->destroy)(context->cipherInfo,PR_TRUE);
    }
    PORT_Free(context);
}

/*
 * create a new nession. NOTE: The session handle is not set, and the
 * session is not added to the slot's session queue.
 */
static PK11Session *
pk11_NewSession(CK_SLOT_ID slotID, CK_NOTIFY notify, CK_VOID_PTR pApplication,
							     CK_FLAGS flags) {
    PK11Session *session;
    PK11Slot *slot = pk11_SlotFromID(slotID);

    if (slot == NULL) return NULL;

    session = PORT_Alloc(sizeof(PK11Session));
    if (session == NULL) return NULL;

    session->next = session->prev = NULL;
    session->refCount = 1;
    session->context = NULL;
    session->search = NULL;
    session->objectIDCount = 1;
#ifdef PKCS11_USE_THREADS
    session->refLock = PR_NewLock(0);
    if (session->refLock == NULL) {
	PORT_Free(session);
	return NULL;
    }
    session->objectLock = PR_NewLock(0);
    if (session->objectLock == NULL) {
	PK11_USE_THREADS(PR_DestroyLock(session->refLock);)
	PORT_Free(session);
	return NULL;
    }
#else
    session->refLock = NULL;
    session->objectLock = NULL;
#endif
    session->objects[0] = NULL;

    session->slot = slot;
    session->notify = notify;
    session->appData = pApplication;
    session->info.flags = flags;
    session->info.slotID = slotID;
    pk11_update_state(slot,session);
    return session;
}


/* free all the data associated with a session. */
static void
pk11_DestroySession(PK11Session *session) {
    PK11ObjectList *op,*next;
    PORT_Assert(session->refCount == 0);

    /* clean out the attributes */
    /* since no one is referencing us, it's safe to walk the chain
     * without a lock */
    for (op = session->objects[0]; op != NULL; op = next) {
        next = op->next;
        /* paranoia */
	op->next = op->prev = NULL;
	pk11_DeleteObject(session,op->parent);
    }
    PK11_USE_THREADS(PR_DestroyLock(session->objectLock);)
    PK11_USE_THREADS(PR_DestroyLock(session->refLock);)
    if (session->context) {
	pk11_FreeContext(session->context);
    }
    if (session->search) {
	pk11_FreeSearch(session->search);
    }
    PORT_Free(session);
}


/*
 * look up a session structure from a session handle
 * generate a reference to it.
 */
PK11Session *
pk11_SessionFromHandle(CK_SESSION_HANDLE handle) {
    PK11Slot	*slot = pk11_SlotFromSessionHandle(handle);
    PK11Session *session;

    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
    pk11queue_find(session,handle,slot->head,SESSION_HASH_SIZE);
    if (session) session->refCount++;
    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)

    return (session);
}

/*
 * release a reference to a session handle
 */
void
pk11_FreeSession(PK11Session *session) {
    PRBool destroy = PR_FALSE;
    PK11_USE_THREADS(PK11Slot *slot = pk11_SlotFromSession(session);)

    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
    if (session->refCount == 1) destroy = PR_TRUE;
    session->refCount--;
    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)

    if (destroy) pk11_DestroySession(session);
}

/*
 * ******************** Object Creation Utilities ***************************
 */


/* Make sure a given attribute exists. If it doesn't, initialize it to
 * value and len
 */
static CK_RV
pk11_forceAttribute(PK11Object *object,CK_ATTRIBUTE_TYPE type,void *value,
							unsigned int len) {
    if ( !pk11_hasAttribute(object, type)) {
	return pk11_AddAttributeType(object,type,value,len);
    }
    return CKR_OK;
}

/*
 * check the consistancy and initialize a Data Object 
 */
static CK_RV
pk11_handleDataObject(PK11Session *session,PK11Object *object) {
    CK_RV rv;

    /* first reject private and token data objects */
    if (pk11_isTrue(object,CKA_PRIVATE) || pk11_isTrue(object,CKA_TOKEN)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    /* now just verify the required date fields */
    rv = pk11_forceAttribute(object,CKA_APPLICATION,NULL,0);
    if (rv != CKR_OK) return rv;
    rv = pk11_forceAttribute(object,CKA_VALUE,NULL,0);
    if (rv != CKR_OK) return rv;

    return CKR_OK;
}

/*
 * check the consistancy and initialize a Certificate Object 
 */
static CK_RV
pk11_handleCertObject(PK11Session *session,PK11Object *object) {
    PK11Attribute *attribute;
    CK_CERTIFICATE_TYPE type;
    SECItem derCert;
    char *label;
    CERTCertDBHandle *handle;
    CERTCertificate *cert;
    CK_RV rv;

    /* certificates must have a type */
    if ( !pk11_hasAttribute(object,CKA_CERTIFICATE_TYPE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    /* we can't sort any certs private */
    if (pk11_isTrue(object,CKA_PRIVATE)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
	
    /* We only support X.509 Certs for now */
    attribute = pk11_FindAttribute(object,CKA_CERTIFICATE_TYPE);
    type = *(CK_CERTIFICATE_TYPE *)attribute->attrib.pValue;
    pk11_FreeAttribute(attribute);

    if (type != CKC_X_509) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    /* X.509 Certificate */

    /* make sure we have a cert */
    if ( !pk11_hasAttribute(object,CKA_VALUE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    /* in PKCS #11, Subject is a required field */
    if ( !pk11_hasAttribute(object,CKA_SUBJECT) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    /*
     * now parse the certificate
     */
    handle = CERT_GetDefaultCertDB();

    /* get the nickname */
    label = pk11_getString(object,CKA_LABEL);
    object->label = label;
    if (label == NULL) {
	return CKR_HOST_MEMORY;
    }
  
    /* get the der cert */ 
    attribute = pk11_FindAttribute(object,CKA_VALUE);
    derCert.data = (unsigned char *)attribute->attrib.pValue;
    derCert.len = attribute->attrib.ulValueLen ;
    
    cert = CERT_NewTempCertificate(handle, &derCert, label, PR_FALSE, PR_TRUE);
    pk11_FreeAttribute(attribute);
    if (cert == NULL) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    /* add it to the object */
    object->objectInfo = cert;
    object->infoFree = (PK11Free) CERT_DestroyCertificate;
    
    /* now just verify the required date fields */
    rv = pk11_forceAttribute(object, CKA_ID, NULL, 0);
    if (rv != CKR_OK) { return rv; }
    rv = pk11_forceAttribute(object,CKA_ISSUER,
				pk11_item_expand(&cert->derIssuer));
    if (rv != CKR_OK) { return rv; }
    rv = pk11_forceAttribute(object,CKA_SERIAL_NUMBER,
				pk11_item_expand(&cert->serialNumber));
    if (rv != CKR_OK) { return rv; }


    if (pk11_isTrue(object,CKA_TOKEN)) {
	CERTCertTrust trust = { CERTDB_USER, CERTDB_USER, CERTDB_USER };

	/* XXX temporary hack for PKCS 12 */
	if(cert->nickname == NULL) {
	    cert->nickname = (char *)PORT_ZAlloc(PORT_Strlen(label)+1);
	    if(cert->nickname == NULL) {
		return CKR_HOST_MEMORY;
	    }
	    PORT_Memcpy(cert->nickname, label, PORT_Strlen(label));
	}

	/* only add certs that have a private key */
	if (SECKEY_KeyForCertExists(SECKEY_GetDefaultKeyDB(),cert) 
							!= SECSuccess) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
	if (CERT_AddTempCertToPerm(cert, label, &trust) != SECSuccess) {
	    return CKR_HOST_MEMORY;
	}
	object->handle |= (PK11_TOKEN_MAGIC | PK11_TOKEN_TYPE_CERT);
	object->inDB = PR_TRUE;
    }

    /* label has been adopted by object->label */
    /*PORT_Free(label); */

    return CKR_OK;
}

static SECKEYLowPublicKey * pk11_GetPubKey(PK11Object *object,CK_KEY_TYPE key);
/*
 * check the consistancy and initialize a Public Key Object 
 */
static CK_RV
pk11_handlePublicKeyObject(PK11Object *object,CK_KEY_TYPE key_type) {
    CK_BBOOL cktrue = TRUE;
    CK_BBOOL encrypt = TRUE;
    CK_BBOOL recover = TRUE;
    CK_BBOOL wrap = TRUE;
    CK_RV rv;

    switch (key_type) {
    case CKK_RSA:
	if ( !pk11_hasAttribute(object, CKA_MODULUS)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_PUBLIC_EXPONENT)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	break;
    case CKK_DSA:
	if ( !pk11_hasAttribute(object, CKA_PRIME)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_SUBPRIME)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_BASE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_VALUE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	encrypt = FALSE;
	recover = FALSE;
	wrap = FALSE;
	break;
    case CKK_DH:
    default:
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    /* make sure the required fields exist */
    rv = pk11_forceAttribute(object,CKA_SUBJECT,NULL,0);
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_ENCRYPT,&encrypt,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_VERIFY,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_VERIFY_RECOVER,
						&recover,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_WRAP,&wrap,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 

    object->objectInfo = pk11_GetPubKey(object,key_type);
    object->infoFree = (PK11Free) SECKEY_LowDestroyPublicKey;

    return CKR_OK;
}

typedef struct {
    CERTCertificate *cert;
    SECItem *pubKey;
} find_cert_callback_arg;

static SECStatus
find_cert_by_pub_key(CERTCertificate *cert, SECItem *k, void *arg) {
    find_cert_callback_arg *cbarg;
    SECKEYPublicKey *pubKey = NULL;
    SECItem *pubItem;

    if((cert == NULL) || (arg == NULL)) {
	return SECFailure;
    }

    /* get cert's public key */
    pubKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    if ( pubKey == NULL ) {
	goto done;
    }

    /* get value to compare from the cert's public key */
    switch ( pubKey->keyType ) {
	case rsaKey:
	    pubItem = &pubKey->u.rsa.modulus;
	    break;
	case dsaKey:
	    pubItem = &pubKey->u.dsa.publicValue;
	    break;
	default:
	    goto done;
    }

    cbarg = (find_cert_callback_arg *)arg;

    if(SECITEM_CompareItem(pubItem, cbarg->pubKey) == SECEqual) {
	cbarg->cert = CERT_DupCertificate(cert);
	return SECFailure;
    }

done:
    if ( pubKey ) {
	SECKEY_DestroyPublicKey(pubKey);
    }

    return (SECSuccess);
}

static SECKEYLowPrivateKey * pk11_mkPrivKey(PK11Object *object,CK_KEY_TYPE key);
static PK11Object *pk11_importCertificate(PK11Slot *slot, CERTCertificate *cert, 
				unsigned char *data, unsigned int size);
/*
 * check the consistancy and initialize a Private Key Object 
 */
static CK_RV
pk11_handlePrivateKeyObject(PK11Object *object,CK_KEY_TYPE key_type) {
    CK_BBOOL cktrue = TRUE;
    CK_BBOOL encrypt = TRUE;
    CK_BBOOL recover = TRUE;
    CK_BBOOL wrap = TRUE;
    CK_RV rv;

    switch (key_type) {
    case CKK_RSA:
	if ( !pk11_hasAttribute(object, CKA_MODULUS)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_PUBLIC_EXPONENT)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_PRIVATE_EXPONENT)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_PRIME_1)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_PRIME_2)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_EXPONENT_1)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_EXPONENT_2)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_COEFFICIENT)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	break;
    case CKK_DSA:
	if ( !pk11_hasAttribute(object, CKA_PRIME)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_SUBPRIME)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_BASE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_VALUE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !pk11_hasAttribute(object, CKA_ID)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	encrypt = FALSE;
	recover = FALSE;
	wrap = FALSE;
	break;
    case CKK_DH:
    default:
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    rv = pk11_forceAttribute(object,CKA_SUBJECT,NULL,0);
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_SENSITIVE,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_DECRYPT,&encrypt,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_SIGN,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_SIGN_RECOVER,&recover,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_UNWRAP,&wrap,sizeof(CK_BBOOL));

    if (pk11_isTrue(object,CKA_TOKEN)) {
	SECKEYLowPrivateKey *privKey;
	char *label;
	SECStatus rv;
	SECItem pubKey;

	privKey=pk11_mkPrivKey(object,key_type);
	if (privKey == NULL) return CKR_HOST_MEMORY;
	label = object->label = pk11_getString(object,CKA_LABEL);

	rv=pk11_Attribute2SecItem(NULL,&pubKey,object,CKA_ID);
	if (rv == CKR_OK) {
	    rv = SECKEY_StoreKeyByPublicKey(SECKEY_GetDefaultKeyDB(),
			privKey, &pubKey, label,
			(SECKEYGetPasswordKey) pk11_givePass, object->slot);

	    /* check for the existance of an existing certificate */
	    if(rv == SECSuccess) {
		find_cert_callback_arg cbarg;
		SECItem pKey;

		switch(key_type) {
		    case CKK_RSA:
			rv=pk11_Attribute2SecItem(NULL,&pKey,
							object,CKA_MODULUS);
			break;
		    case CKK_DSA:
			rv = pk11_Attribute2SecItem(NULL,&pKey,
							object, CKA_VALUE);
			break;
		    default:
			rv = SECFailure;
		} 

		if(rv == SECSuccess) {
		    cbarg.pubKey = &pKey;
		    cbarg.cert = NULL;
		    SEC_TraversePermCerts(CERT_GetDefaultCertDB(),
				       find_cert_by_pub_key, (void *)&cbarg);
		    if(cbarg.cert != NULL) {
			PK11Object *obj = pk11_importCertificate(object->slot,
		    					cbarg.cert, NULL, 0);
		        if(obj != NULL) {
			    pk11_AddSlotObject(object->slot, obj);
			}
			CERT_DestroyCertificate(cbarg.cert);
		    }
		    SECITEM_ZfreeItem(&pKey, PR_FALSE);
		}
		rv = SECSuccess;
	    }

	    if (pubKey.data) PORT_Free(pubKey.data);
	}

	SECKEY_LowDestroyPrivateKey(privKey);
	if (rv != SECSuccess) return CKR_DEVICE_ERROR;
	object->inDB = PR_TRUE;
        object->handle |= (PK11_TOKEN_MAGIC | PK11_TOKEN_TYPE_PRIV);
    } else {
	object->objectInfo = pk11_mkPrivKey(object,key_type);
	if (object->objectInfo == NULL) return CKR_HOST_MEMORY;
	object->infoFree = (PK11Free) SECKEY_LowDestroyPrivateKey;
    }
    /* now NULL out the sensitive attributes */
    if (pk11_isTrue(object,CKA_SENSITIVE)) {
	pk11_nullAttribute(object,CKA_PRIVATE_EXPONENT);
	pk11_nullAttribute(object,CKA_PRIME_1);
	pk11_nullAttribute(object,CKA_PRIME_2);
	pk11_nullAttribute(object,CKA_EXPONENT_1);
	pk11_nullAttribute(object,CKA_EXPONENT_2);
	pk11_nullAttribute(object,CKA_COEFFICIENT);
    }
    return CKR_OK;
}

/*
 * check the consistancy and initialize a Secret Key Object 
 */
static CK_RV
pk11_handleSecretKeyObject(PK11Object *object,CK_KEY_TYPE key_type) {
    CK_RV rv;
    CK_BBOOL cktrue = TRUE;
    CK_BBOOL ckfalse = FALSE;

    rv = pk11_forceAttribute(object,CKA_SENSITIVE,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_ENCRYPT,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_DECRYPT,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_SIGN,&ckfalse,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_VERIFY,&ckfalse,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_WRAP,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_UNWRAP,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 
    if ( !pk11_hasAttribute(object, CKA_VALUE)) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    return CKR_OK;
}

/*
 * check the consistancy and initialize a Key Object 
 */
static CK_RV
pk11_handleKeyObject(PK11Session *session, PK11Object *object) {
    PK11Attribute *attribute;
    CK_KEY_TYPE key_type;
    CK_BBOOL cktrue = TRUE;
    CK_RV rv;

    /* Only private keys can be private or token */
    if ((object->class != CKO_PRIVATE_KEY) &&
          (pk11_isTrue(object,CKA_PRIVATE) || pk11_isTrue(object,CKA_TOKEN))) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    /* Make sure that the private key is CKA_PRIVATE if it's a token */
    if (pk11_isTrue(object,CKA_TOKEN) && !pk11_isTrue(object,CKA_PRIVATE)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    /* verify the required fields */
    if ( !pk11_hasAttribute(object,CKA_KEY_TYPE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    /* now verify the common fields */
    rv = pk11_forceAttribute(object,CKA_ID,NULL,0);
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_START_DATE,NULL,0);
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_END_DATE,NULL,0);
    if (rv != CKR_OK)  return rv; 
    rv = pk11_forceAttribute(object,CKA_DERIVE,&cktrue,sizeof(CK_BBOOL));
    if (rv != CKR_OK)  return rv; 

    /* get the key type */
    attribute = pk11_FindAttribute(object,CKA_KEY_TYPE);
    key_type = *(CK_KEY_TYPE *)attribute->attrib.pValue;
    pk11_FreeAttribute(attribute);

    switch (object->class) {
    case CKO_PUBLIC_KEY:
	return pk11_handlePublicKeyObject(object,key_type);
    case CKO_PRIVATE_KEY:
	return pk11_handlePrivateKeyObject(object,key_type);
    case CKO_SECRET_KEY:
	/* make sure the required fields exist */
	return pk11_handleSecretKeyObject(object,key_type);
    default:
	break;
    }
    return CKR_OBJECT_CLASS_INVALID;
}

/* check the consistance, initialize, and queue in new objects */
static CK_RV 
pk11_handleObject(PK11Object *object, PK11Session *session) {
    PK11Slot *slot = session->slot;
    CK_BBOOL ckfalse = FALSE;
    PK11Attribute *attribute;
    CK_RV rv;

    /* make sure all the base object types are defined. If not set the
     * defaults */
    rv = pk11_forceAttribute(object,CKA_TOKEN,&ckfalse,sizeof(CK_BBOOL));
    if (rv != CKR_OK) return rv;
    rv = pk11_forceAttribute(object,CKA_PRIVATE,&ckfalse,sizeof(CK_BBOOL));
    if (rv != CKR_OK) return rv;
    rv = pk11_forceAttribute(object,CKA_LABEL,NULL,0);
    if (rv != CKR_OK) return rv;

    /* don't create a private object if we aren't logged in */
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(pk11_isTrue(object,CKA_PRIVATE))) {
	return CKR_USER_NOT_LOGGED_IN;
    }


    if (((session->info.flags & CKF_RW_SESSION) == 0) &&
				(pk11_isTrue(object,CKA_TOKEN))) {
	return CKR_SESSION_READ_ONLY;
    }
	
    /* Sigh, PKCS #11 object ID's are unique for all objects on a
     * token */
    PK11_USE_THREADS(PR_Lock(slot->objectLock);)
    object->handle = slot->tokenIDCount++;
    PK11_USE_THREADS(PR_Unlock(slot->objectLock);)

    /* get the object class */
    attribute = pk11_FindAttribute(object,CKA_CLASS);
    if (attribute == NULL) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    object->class = *(CK_OBJECT_CLASS *)attribute->attrib.pValue;
    pk11_FreeAttribute(attribute);

    /* now handle the specific. Get a session handle for these functions
     * to use */
    switch (object->class) {
    case CKO_DATA:
	rv = pk11_handleDataObject(session,object);
    case CKO_CERTIFICATE:
	rv = pk11_handleCertObject(session,object);
	break;
    case CKO_PRIVATE_KEY:
    case CKO_PUBLIC_KEY:
    case CKO_SECRET_KEY:
	rv = pk11_handleKeyObject(session,object);
	break;
    default:
	rv = CKR_OBJECT_CLASS_INVALID;
	break;
    }

    /* can't fail from here on out unless the pk_handlXXX functions have
     * failed the request */
    if (rv != CKR_OK) {
	return rv;
    }

    /* now link the object into the slot and session structures */
    object->slot = slot;
    pk11_AddObject(session,object);

    return CKR_OK;
}

/* import a private key as an object. We don't call handle object.
 * because we don't want*/
static PK11Object *
pk11_importPrivateKey(PK11Slot *slot,SECKEYLowPrivateKey *lowPriv) {
    PK11Object *privateKey;
    CK_KEY_TYPE key_type;
    CK_BBOOL cktrue = TRUE;
    CK_BBOOL ckfalse = FALSE;
    CK_BBOOL sign = TRUE;
    CK_BBOOL encrypt = TRUE;
    CK_BBOOL derive = FALSE;
    CK_RV crv = CKR_OK;
    CK_OBJECT_CLASS privClass = CKO_PRIVATE_KEY;

    /*
     * now lets create an object to hang the attributes off of
     */
    privateKey = pk11_NewObject(slot); /* fill in the handle later */
    if (privateKey == NULL) {
	pk11_FreeObject(privateKey);
	return NULL;
    }


    /* Fill in the common Default values */
    if (pk11_AddAttributeType(privateKey,CKA_START_DATE, NULL, 0) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_END_DATE, NULL, 0) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_SUBJECT, NULL, 0) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_LABEL, NULL, 0) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_CLASS,&privClass,
					sizeof(CK_OBJECT_CLASS)) != CKR_OK) {
	 pk11_FreeObject(privateKey);
	 return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_SENSITIVE, &cktrue,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_PRIVATE, &cktrue,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_TOKEN, &cktrue,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_SIGN_RECOVER, &ckfalse,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }

    /* Now Set up the parameters to generate the key (based on mechanism) */
    switch (lowPriv->keyType) {
    case rsaKey:
	/* format the keys */
	key_type = CKK_RSA;
	sign = TRUE;
	encrypt = TRUE;
	derive = FALSE;
        /* now fill in the RSA dependent paramenters in the public key */
        crv = pk11_AddAttributeType(privateKey,CKA_MODULUS,
			   pk11_item_expand(&lowPriv->u.rsa.modulus));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_ID,
			   pk11_item_expand(&lowPriv->u.rsa.modulus));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_PRIVATE_EXPONENT,
			   pk11_item_expand(&lowPriv->u.rsa.privateExponent));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_PUBLIC_EXPONENT,
			   pk11_item_expand(&lowPriv->u.rsa.publicExponent));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_PRIME_1,
			   pk11_item_expand(&lowPriv->u.rsa.prime[0]));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_PRIME_2,
			   pk11_item_expand(&lowPriv->u.rsa.prime[1]));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_EXPONENT_1,
			   pk11_item_expand(&lowPriv->u.rsa.primeExponent[0]));
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_EXPONENT_2,
			   pk11_item_expand(&lowPriv->u.rsa.primeExponent[1]));
	if (crv != CKR_OK)  break;
        crv = pk11_AddAttributeType(privateKey,CKA_COEFFICIENT,
			   pk11_item_expand(&lowPriv->u.rsa.coefficient));
	break;
    case dsaKey:
	key_type = CKK_DSA;
	sign = TRUE;
	encrypt = FALSE;
	derive = FALSE;
	/* don't forget to get CKA_ID */
	crv = pk11_AddAttributeType(privateKey,CKA_PRIME,
			   pk11_item_expand(&lowPriv->u.dsa.params.prime));
	if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(privateKey,CKA_SUBPRIME,
			   pk11_item_expand(&lowPriv->u.dsa.params.subPrime));
	if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(privateKey,CKA_BASE,
			   pk11_item_expand(&lowPriv->u.dsa.params.base));
	if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(privateKey,CKA_VALUE,
			   pk11_item_expand(&lowPriv->u.dsa.privateValue));
	if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(privateKey,CKA_ID,
			   pk11_item_expand(&lowPriv->u.dsa.publicValue));
	if (crv != CKR_OK) break;
	break;
    case dhKey:
	key_type = CKK_DH;
	sign = FALSE;
	encrypt = FALSE;
	derive = TRUE;
	/* don't forget to get CKA_ID */
	crv = CKR_MECHANISM_INVALID;
	break;
    default:
	crv = CKR_MECHANISM_INVALID;
    }

    if (crv != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }


    if (pk11_AddAttributeType(privateKey,CKA_SIGN, &sign,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_DECRYPT, &encrypt,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_WRAP, &encrypt,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_DERIVE, &derive,
					      sizeof(CK_BBOOL)) != CKR_OK) {
	pk11_FreeObject(privateKey);
	return NULL;
    }
    if (pk11_AddAttributeType(privateKey,CKA_KEY_TYPE,&key_type,
					     sizeof(CK_KEY_TYPE)) != CKR_OK) {
	 pk11_FreeObject(privateKey);
	 return NULL;
    }
    PK11_USE_THREADS(PR_Lock(slot->objectLock);)
    privateKey->handle = slot->tokenIDCount++;
    privateKey->handle |= (PK11_TOKEN_MAGIC | PK11_TOKEN_TYPE_PRIV);
    PK11_USE_THREADS(PR_Unlock(slot->objectLock);)
    privateKey->class = privClass;
    privateKey->slot = slot;
    privateKey->inDB = PR_TRUE;

    return privateKey;
}

/* from pk11cert.c */
SECItem * pk11_mkcertKeyID(CERTCertificate *cert);

/*
 * Question.. Why doesn't import Cert call pk11_handleObject, or
 * pk11 handleCertObject? Answer: because they will try to write
 * this cert back out to the Database, even though it is already in
 * the database.
 */
static PK11Object *
pk11_importCertificate(PK11Slot *slot, CERTCertificate *cert, 
				unsigned char *data, unsigned int size) {
    PK11Object *certObject;
    CK_BBOOL cktrue = TRUE;
    CK_BBOOL ckfalse = FALSE;
    CK_CERTIFICATE_TYPE certType = CKC_X_509;
    CK_RV rv = CKR_OK;
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    SECItem *id;

    /*
     * now lets create an object to hang the attributes off of
     */
    certObject = pk11_NewObject(slot); /* fill in the handle later */
    if (certObject == NULL) {
	return NULL;
    }

    /* initalize the certificate attributes */
    if (pk11_AddAttributeType(certObject, CKA_CLASS, &certClass,
					 sizeof(CK_OBJECT_CLASS)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_TOKEN, &cktrue,
					       sizeof(CK_BBOOL)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_PRIVATE, &ckfalse,
					       sizeof(CK_BBOOL)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_LABEL, cert->nickname,
					PORT_Strlen(cert->nickname))
								!= CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_CERTIFICATE_TYPE,  &certType,
					sizeof(CK_CERTIFICATE_TYPE))!=CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_VALUE, 
			 	pk11_item_expand(&cert->derCert)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_ISSUER, 
			 	pk11_item_expand(&cert->derIssuer)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_SUBJECT, 
		 	pk11_item_expand(&cert->derSubject)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    if (pk11_AddAttributeType(certObject, CKA_SERIAL_NUMBER, 
		 	pk11_item_expand(&cert->serialNumber)) != CKR_OK) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }

    id = pk11_mkcertKeyID(cert);
    if (id == NULL) {
	 pk11_FreeObject(certObject);
	 return NULL;
    }
    rv = pk11_AddAttributeType(certObject, CKA_ID, pk11_item_expand(id));
    SECITEM_FreeItem(id,PR_TRUE);
    if (rv != CKR_OK) {
	pk11_FreeObject(certObject);
	return NULL;
    }
  
    certObject->objectInfo = CERT_DupCertificate(cert);
    certObject->infoFree = (PK11Free) CERT_DestroyCertificate;
    
    /* now just verify the required date fields */
    PK11_USE_THREADS(PR_Lock(slot->objectLock);)
    certObject->handle = slot->tokenIDCount++;
    certObject->handle |= (PK11_TOKEN_MAGIC | PK11_TOKEN_TYPE_CERT);
    PK11_USE_THREADS(PR_Unlock(slot->objectLock);)
    certObject->class = certClass;
    certObject->slot = slot;
    certObject->inDB = PR_TRUE;

    return certObject;
}

/*
 * ******************** Public Key Utilities ***************************
 */
/* Generate a low public key structure from an object */
SECKEYLowPublicKey *pk11_GetPubKey(PK11Object *object,CK_KEY_TYPE key_type) {
    SECKEYLowPublicKey *pubKey;
    PRArenaPool *arena;
    CK_RV rv;

    if (object->class != CKO_PUBLIC_KEY) {
	return NULL;
    }

    /* If we already have a key, use it */
    if (object->objectInfo) {
	return (SECKEYLowPublicKey *)object->objectInfo;
    }

    /* allocate the structure */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;

    pubKey = (SECKEYLowPublicKey *)
			PORT_ArenaAlloc(arena,sizeof(SECKEYLowPublicKey));
    if (pubKey == NULL) {
    	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }

    /* fill in the structure */
    pubKey->arena = arena;
    switch (key_type) {
    case CKK_RSA:
	pubKey->keyType = rsaKey;
	rv = pk11_Attribute2SecItem(arena,&pubKey->u.rsa.modulus,
							object,CKA_MODULUS);
    	if (rv != CKR_OK) break;
    	rv = pk11_Attribute2SecItem(arena,&pubKey->u.rsa.publicExponent,
						object,CKA_PUBLIC_EXPONENT);
	break;
    case CKK_DSA:
	pubKey->keyType = dsaKey;
	rv = pk11_Attribute2SecItem(arena,&pubKey->u.dsa.params.prime,
							object,CKA_PRIME);
    	if (rv != CKR_OK) break;
	rv = pk11_Attribute2SecItem(arena,&pubKey->u.dsa.params.subPrime,
							object,CKA_SUBPRIME);
    	if (rv != CKR_OK) break;
	rv = pk11_Attribute2SecItem(arena,&pubKey->u.dsa.params.base,
							object,CKA_BASE);
    	if (rv != CKR_OK) break;
    	rv = pk11_Attribute2SecItem(arena,&pubKey->u.dsa.publicValue,
							object,CKA_VALUE);
	break;
    case CKK_DH:
    default:
	rv = CKR_KEY_TYPE_INCONSISTENT;
	break;
    }
    if (rv != CKR_OK) {
    	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }

    object->objectInfo = pubKey;
    object->infoFree = (PK11Free) SECKEY_LowDestroyPublicKey;
    return pubKey;
}

/* make a private key from a verified object */
static SECKEYLowPrivateKey *
pk11_mkPrivKey(PK11Object *object,CK_KEY_TYPE key_type) {
    SECKEYLowPrivateKey *privKey;
    PRArenaPool *arena;
    CK_RV rv = CKR_OK;
    SECStatus srv;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;

    privKey = (SECKEYLowPrivateKey *)
			PORT_ArenaAlloc(arena,sizeof(SECKEYLowPrivateKey));
    if (privKey == NULL)  {
	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }

    /* in future this would be a switch on key_type */
    privKey->arena = arena;
    switch (key_type) {
    case CKK_RSA:
	privKey->keyType = rsaKey;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.modulus,
							object,CKA_MODULUS);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.publicExponent,object,
							CKA_PUBLIC_EXPONENT);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.privateExponent,object,
							CKA_PRIVATE_EXPONENT);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.prime[0],object,
								CKA_PRIME_1);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.prime[1],object,
								CKA_PRIME_2);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.primeExponent[0],
						object, CKA_EXPONENT_1);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.primeExponent[1],
							object, CKA_EXPONENT_2);
	if (rv != CKR_OK) break;
	rv=pk11_Attribute2SecItem(arena,&privKey->u.rsa.coefficient,object,
							      CKA_COEFFICIENT);
	if (rv != CKR_OK) break;
        srv = DER_SetUInteger(privKey->arena, &privKey->u.rsa.version,
                          SEC_PRIVATE_KEY_VERSION);
	if (srv != SECSuccess) rv = CKR_HOST_MEMORY;
	break;

    case CKK_DSA:
	privKey->keyType = dsaKey;
	rv = pk11_Attribute2SecItem(arena,&privKey->u.dsa.params.prime,
							object,CKA_PRIME);
    	if (rv != CKR_OK) break;
	rv = pk11_Attribute2SecItem(arena,&privKey->u.dsa.params.subPrime,
							object,CKA_SUBPRIME);
    	if (rv != CKR_OK) break;
	rv = pk11_Attribute2SecItem(arena,&privKey->u.dsa.params.base,
							object,CKA_BASE);
    	if (rv != CKR_OK) break;
    	rv = pk11_Attribute2SecItem(arena,&privKey->u.dsa.privateValue,
							object,CKA_VALUE);
    	if (rv != CKR_OK) break;
    	rv = pk11_Attribute2SecItem(arena,&privKey->u.dsa.publicValue,
							object,CKA_ID);
	/* sigh, can't set the public value.... */
	break;
    case CKK_DH:
    default:
	rv = CKR_KEY_TYPE_INCONSISTENT;
	break;
    }
    if (rv != CKR_OK) {
	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }
    return privKey;
}


/* Generate a low private key structure from an object */
SECKEYLowPrivateKey *pk11_GetPrivKey(PK11Object *object,CK_KEY_TYPE key_type) {
    SECKEYLowPrivateKey *priv = NULL;

    if (object->class != CKO_PRIVATE_KEY) {
	return NULL;
    }
    if (object->objectInfo) {
	return (SECKEYLowPrivateKey *)object->objectInfo;
    }

    if (pk11_isTrue(object,CKA_TOKEN)) {
	/* grab it from the data base */
	char *label = pk11_getString(object,CKA_LABEL);
	SECItem pubKey;
	CK_RV rv;

	/* KEYID is the public KEY for DSA and DH, and the MODULUS for
	 *  RSA */
	rv=pk11_Attribute2SecItem(NULL,&pubKey,object,CKA_ID);
	if (rv != CKR_OK) return NULL;
	
	priv=SECKEY_FindKeyByPublicKey(SECKEY_GetDefaultKeyDB(),&pubKey,
				       (SECKEYGetPasswordKey) pk11_givePass,
				       object->slot);
	if (pubKey.data) PORT_Free(pubKey.data);

	/* don't 'cache' DB private keys */
	return priv;
    } 

    priv = pk11_mkPrivKey(object, key_type);
    object->objectInfo = priv;
    object->infoFree = (PK11Free) SECKEY_LowDestroyPrivateKey;
    return priv;
}

/*
 **************************** Symetric Key utils ************************
 */

static PRBool
pk11_CheckDESKey(unsigned char *key) {
    int i;

    /* format the des key */
    for (i=0; i < 8; i++) {
	key[i] = parityTable[key[i]>>1];
    }

    for (i=0; i < pk11_desWeakTableSize; i++) {
	if (PORT_Memcmp(key,pk11_desWeakTable[i],8) == 0) {
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}

static PRBool
pk11_IsWeakKey(unsigned char *key,CK_KEY_TYPE key_type) {

    switch(key_type) {
    case CKK_DES:
	return pk11_CheckDESKey(key);
    case CKM_DES2_KEY_GEN:
	if (pk11_CheckDESKey(key)) return PR_TRUE;
	return pk11_CheckDESKey(&key[8]);
    case CKM_DES3_KEY_GEN:
	if (pk11_CheckDESKey(key)) return PR_TRUE;
	if (pk11_CheckDESKey(&key[8])) return PR_TRUE;
	return pk11_CheckDESKey(&key[16]);
    default:
	break;
    }
    return PR_FALSE;
}


	    

/**********************************************************************
 *
 *     Start of PKCS 11 functions 
 *
 **********************************************************************/
 
 
    

/* return the function list */
CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR *pFunctionList) {
    *pFunctionList = &pk11_funcList;
    return CKR_OK;
}


/* C_Initialize initializes the Cryptoki library. */
CK_RV C_Initialize(CK_VOID_PTR pReserved) {
    int i,j;
    static PRBool init = PR_FALSE;
    SECStatus rv = SECSuccess;

    /* initialize the Random number generater */
    rv =  RNG_RNGInit();

    /* intialize all the slots */
    if (!init) {
    init = PR_TRUE;
    for (i=0; i < 2; i++) {
#ifdef PKCS11_USE_THREADS
	pk11_slot[i].sessionLock = PR_NewLock(0);
	if (pk11_slot[i].sessionLock == NULL) return CKR_HOST_MEMORY;
	pk11_slot[i].objectLock = PR_NewLock(0);
	if (pk11_slot[i].objectLock == NULL) return CKR_HOST_MEMORY;
#else
	pk11_slot[i].sessionLock = NULL;
	pk11_slot[i].objectLock = NULL;
#endif
	for(j=0; j < SESSION_HASH_SIZE; j++) {
	    pk11_slot[i].head[j] = NULL;
	}
	for(j=0; j < HASH_SIZE; j++) {
	    pk11_slot[i].tokObjects[j] = NULL;
	}
	pk11_slot[i].password = NULL;
	pk11_slot[i].hasTokens = PR_FALSE;
	pk11_slot[i].sessionIDCount = 1;
	pk11_slot[i].sessionCount = 0;
	pk11_slot[i].rwSessionCount = 0;
	pk11_slot[i].tokenIDCount = 1;
	pk11_slot[i].needLogin = PR_FALSE;
	pk11_slot[i].isLoggedIn = PR_FALSE;
	pk11_slot[i].ssoLoggedIn = PR_FALSE;
	pk11_slot[i].DB_loaded = PR_FALSE;
	pk11_slot[i].slotID= ( i ? PRIVATE_KEY_SLOT_ID : NETSCAPE_SLOT_ID);
    }
    /* if the data base is initialized with a null password, remember that */
    pk11_slot[1].needLogin = !pk11_hasNullPassword(&pk11_slot[1].password);
    }

    if (rv != SECSuccess) {
	return CKR_DEVICE_ERROR;
    }

    SECKEY_SetDefaultKeyDBAlg(SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC);

    return CKR_OK;
}

/*C_Finalize indicates that an application is done with the Cryptoki library.*/
CK_RV C_Finalize (CK_VOID_PTR pReserved) {
    return CKR_OK;
}




/* C_GetInfo returns general information about Cryptoki. */
CK_RV  C_GetInfo(CK_INFO_PTR pInfo) {
    pInfo->cryptokiVersion.major = 2;
    pInfo->cryptokiVersion.minor = 0;
    PORT_Memcpy(pInfo->manufacturerID,manufacturerID,32);
    pInfo->libraryVersion.major = 4;
    pInfo->libraryVersion.minor = 0;
    PORT_Memcpy(pInfo->libraryDescription,libraryDescription,32);
    pInfo->flags = 0;
    return CKR_OK;
}

/* C_GetSlotList obtains a list of slots in the system. */
CK_RV C_GetSlotList(CK_BBOOL	tokenPresent,
	 		CK_SLOT_ID_PTR	pSlotList, CK_ULONG_PTR pulCount) {
    *pulCount = 2;
    if (pSlotList != NULL) {
	pSlotList[0] = NETSCAPE_SLOT_ID;
	pSlotList[1] = PRIVATE_KEY_SLOT_ID;
    }
    return CKR_OK;
}
	
/* C_GetSlotInfo obtains information about a particular slot in the system. */
CK_RV C_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo) {
    pInfo->firmwareVersion.major = 0;
    pInfo->firmwareVersion.minor = 0;
    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	PORT_Memcpy(pInfo->manufacturerID,manufacturerID,32);
	PORT_Memcpy(pInfo->slotDescription,slotDescription,64);
	pInfo->flags = CKF_TOKEN_PRESENT;
        pInfo->hardwareVersion.major = 4;
        pInfo->hardwareVersion.minor = 0;
	return CKR_OK;
    case PRIVATE_KEY_SLOT_ID:
	PORT_Memcpy(pInfo->manufacturerID,manufacturerID,32);
	PORT_Memcpy(pInfo->slotDescription,privSlotDescription,64);
	pInfo->flags = CKF_TOKEN_PRESENT;
	/* ok we really should read it out of the keydb file. */
        pInfo->hardwareVersion.major = PRIVATE_KEY_DB_FILE_VERSION;
        pInfo->hardwareVersion.minor = 0;
	return CKR_OK;
    default:
	break;
    }
    return CKR_SLOT_ID_INVALID;
}

#define CKF_THREAD_SAFE 0x8000 /* XXXXX for now */

/* C_GetTokenInfo obtains information about a particular token in the system. */
CK_RV C_GetTokenInfo(CK_SLOT_ID slotID,CK_TOKEN_INFO_PTR pInfo) {
    PK11Slot *slot = pk11_SlotFromID(slotID);
    SECKEYKeyDBHandle *handle;

    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    PORT_Memcpy(pInfo->manufacturerID,manufacturerID,32);
    PORT_Memcpy(pInfo->model,"Libsec 4.0      ",16);
    PORT_Memcpy(pInfo->serialNumber,"0000000000000000",16);
    pInfo->ulMaxSessionCount = 0; /* arbitrarily large */
    pInfo->ulSessionCount = slot->sessionCount;
    pInfo->ulMaxRwSessionCount = 0; /* arbitarily large */
    pInfo->ulRwSessionCount = slot->rwSessionCount;
    pInfo->firmwareVersion.major = 0;
    pInfo->firmwareVersion.minor = 0;
    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	PORT_Memcpy(pInfo->label,tokDescription,32);
        pInfo->flags= CKF_RNG | CKF_WRITE_PROTECTED | CKF_THREAD_SAFE;
	pInfo->ulMaxPinLen = 0;
	pInfo->ulMinPinLen = 0;
	pInfo->ulTotalPublicMemory = 0;
	pInfo->ulFreePublicMemory = 0;
	pInfo->ulTotalPrivateMemory = 0;
	pInfo->ulFreePrivateMemory = 0;
	pInfo->hardwareVersion.major = 4;
	pInfo->hardwareVersion.minor = 0;
	return CKR_OK;
    case PRIVATE_KEY_SLOT_ID:
	PORT_Memcpy(pInfo->label,privTokDescription,32);
    	handle = SECKEY_GetDefaultKeyDB();
	/*
	 * we have three possible states which we may be in:
	 *   (1) No DB password has been initialized. This also means we
	 *   have no keys in the key db.
	 *   (2) Password initialized to NULL. This means we have keys, but
	 *   the user has chosen not use a password.
	 *   (3) Finally we have an initialized password whicn is not NULL, and
	 *   we will need to prompt for it.
	 */
	if (SECKEY_HasKeyDBPassword(handle) == SECFailure) {
	    pInfo->flags = CKF_THREAD_SAFE | CKF_LOGIN_REQUIRED;
	} else if (!slot->needLogin) {
	    pInfo->flags = CKF_THREAD_SAFE | CKF_USER_PIN_INITIALIZED;
	} else {
	    pInfo->flags = CKF_THREAD_SAFE | 
				CKF_LOGIN_REQUIRED | CKF_USER_PIN_INITIALIZED;
	}
	pInfo->ulMaxPinLen = PK11_MAX_PIN;
	pInfo->ulMinPinLen = 0;
	pInfo->ulTotalPublicMemory = 1;
	pInfo->ulFreePublicMemory = 1;
	pInfo->ulTotalPrivateMemory = 1;
	pInfo->ulFreePrivateMemory = 1;
	pInfo->hardwareVersion.major = CERT_DB_FILE_VERSION;
	pInfo->hardwareVersion.minor = 0;
	return CKR_OK;
    default:
	break;
    }
    return CKR_SLOT_ID_INVALID;
}



/* C_GetMechanismList obtains a list of mechanism types supported by a token. */
CK_RV C_GetMechanismList(CK_SLOT_ID slotID,
	CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pulCount) {
    int i;

    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	*pulCount = mechanismCount;
	if (pMechanismList != NULL) {
	    for (i=0; i < mechanismCount; i++) {
		pMechanismList[i] = mechanisms[i].type;
	    }
	}
	return CKR_OK;
    case PRIVATE_KEY_SLOT_ID:
	*pulCount = 0;
	for (i=0; i < mechanismCount; i++) {
	    if (mechanisms[i].privkey) {
		(*pulCount)++;
		if (pMechanismList != NULL) {
		    *pMechanismList++ = mechanisms[i].type;
		}
	    }
	}
	return CKR_OK;
    default:
	break;
    }
    return CKR_SLOT_ID_INVALID;
}


/* C_GetMechanismInfo obtains information about a particular mechanism 
 * possibly supported by a token. */
CK_RV C_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type,
    					CK_MECHANISM_INFO_PTR pInfo) {
    PRBool isPrivateKey;
    int i;

    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	isPrivateKey = PR_FALSE;
	break;
    case PRIVATE_KEY_SLOT_ID:
	isPrivateKey = PR_TRUE;
	break;
    default:
	return CKR_SLOT_ID_INVALID;
    }
    for (i=0; i < mechanismCount; i++) {
        if (type == mechanisms[i].type) {
	    if (isPrivateKey && !mechanisms[i].privkey) {
    		return CKR_MECHANISM_INVALID;
	    }
	    PORT_Memcpy(pInfo,&mechanisms[i].domestic,
						sizeof(CK_MECHANISM_INFO));
	    return CKR_OK;
	}
    }
    return CKR_MECHANISM_INVALID;
}


/* C_InitToken initializes a token. */
CK_RV C_InitToken(CK_SLOT_ID slotID,CK_CHAR_PTR pPin,
 				CK_ULONG ulPinLen,CK_CHAR_PTR pLabel) {
    return CKR_HOST_MEMORY; /*is this the right function for not implemented*/
}


/* C_InitPIN initializes the normal user's PIN. */
CK_RV C_InitPIN(CK_SESSION_HANDLE hSession,
    					CK_CHAR_PTR pPin, CK_ULONG ulPinLen) {
    PK11Session *sp;
    PK11Slot *slot;
    SECKEYKeyDBHandle *handle;
    SECItem *newPin;
    char newPinStr[256];
    SECStatus rv;

    
    sp = pk11_SessionFromHandle(hSession);
    if (sp == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }

    if (sp->info.slotID == NETSCAPE_SLOT_ID) {
	pk11_FreeSession(sp);
	return CKR_PIN_LEN_RANGE;
    }

    /* should be an assert */
    if (sp->info.slotID != PRIVATE_KEY_SLOT_ID) {
	pk11_FreeSession(sp);
	return CKR_SESSION_HANDLE_INVALID;;
    }

    if (sp->info.state != CKS_RW_SO_FUNCTIONS) {
	pk11_FreeSession(sp);
	return CKR_USER_NOT_LOGGED_IN;
    }

    slot = pk11_SlotFromSession(sp);
    pk11_FreeSession(sp);

    /* make sure the pins aren't too long */
    if (ulPinLen > 255) {
	return CKR_PIN_LEN_RANGE;
    }

    handle = SECKEY_GetDefaultKeyDB();
    if (handle == NULL) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }
    if (SECKEY_HasKeyDBPassword(handle) != SECFailure) {
	return CKR_DEVICE_ERROR;
    }

    /* convert to null terminated string */
    PORT_Memcpy(newPinStr,pPin,ulPinLen);
    newPinStr[ulPinLen] = 0; 

    /* build the hashed pins which we pass around */
    newPin = SECKEY_HashPassword(newPinStr,handle->global_salt);
    PORT_Memset(newPinStr,0,sizeof(newPinStr));

    /* change the data base */
    rv = SECKEY_SetKeyDBPassword(handle,newPin);

    /* Now update our local copy of the pin */
    if (rv == SECSuccess) {
	if (slot->password) {
    	    SECITEM_ZfreeItem(slot->password, PR_TRUE);
	}
	slot->password = newPin;
	if (ulPinLen == 0) slot->needLogin = PR_FALSE;
	return CKR_OK;
    }
    SECITEM_ZfreeItem(newPin, PR_TRUE);
    return CKR_PIN_INCORRECT;
}


/* C_SetPIN modifies the PIN of user that is currently logged in. */
/* NOTE: This is only valid for the PRIVATE_KEY_SLOT */
CK_RV C_SetPIN(CK_SESSION_HANDLE hSession, CK_CHAR_PTR pOldPin,
    CK_ULONG ulOldLen, CK_CHAR_PTR pNewPin, CK_ULONG ulNewLen) {
    PK11Session *sp;
    PK11Slot *slot;
    SECKEYKeyDBHandle *handle;
    SECItem *newPin;
    SECItem *oldPin;
    char newPinStr[256],oldPinStr[256];
    SECStatus rv;

    
    sp = pk11_SessionFromHandle(hSession);
    if (sp == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }

    if (sp->info.slotID == NETSCAPE_SLOT_ID) {
	pk11_FreeSession(sp);
	return CKR_PIN_LEN_RANGE;
    }

    /* should be an assert */
    if (sp->info.slotID != PRIVATE_KEY_SLOT_ID) {
	pk11_FreeSession(sp);
	return CKR_SESSION_HANDLE_INVALID;;
    }

    slot = pk11_SlotFromSession(sp);
    if (slot->needLogin && sp->info.state != CKS_RW_USER_FUNCTIONS) {
	pk11_FreeSession(sp);
	return CKR_USER_NOT_LOGGED_IN;
    }

    pk11_FreeSession(sp);

    /* make sure the pins aren't too long */
    if ((ulNewLen > 255) || (ulOldLen > 255)) {
	return CKR_PIN_LEN_RANGE;
    }


    handle = SECKEY_GetDefaultKeyDB();
    if (handle == NULL) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    /* convert to null terminated string */
    PORT_Memcpy(newPinStr,pNewPin,ulNewLen);
    newPinStr[ulNewLen] = 0; 
    PORT_Memcpy(oldPinStr,pOldPin,ulOldLen);
    oldPinStr[ulOldLen] = 0; 

    /* build the hashed pins which we pass around */
    newPin = SECKEY_HashPassword(newPinStr,handle->global_salt);
    oldPin = SECKEY_HashPassword(oldPinStr,handle->global_salt);
    PORT_Memset(newPinStr,0,sizeof(newPinStr));
    PORT_Memset(oldPinStr,0,sizeof(oldPinStr));

    /* change the data base */
    rv = SECKEY_ChangeKeyDBPassword(handle,oldPin,newPin);

    /* Now update our local copy of the pin */
    SECITEM_ZfreeItem(oldPin, PR_TRUE);
    if (rv == SECSuccess) {
	if (slot->password) {
    	    SECITEM_ZfreeItem(slot->password, PR_TRUE);
	}
	slot->password = newPin;
	slot->needLogin = (ulNewLen != 0);
	return CKR_OK;
    }
    SECITEM_ZfreeItem(newPin, PR_TRUE);
    return CKR_PIN_INCORRECT;
}

/* C_OpenSession opens a session between an application and a token. */
CK_RV C_OpenSession(CK_SLOT_ID slotID, CK_FLAGS flags,
   CK_VOID_PTR pApplication,CK_NOTIFY Notify,CK_SESSION_HANDLE_PTR phSession) {
    PK11Slot *slot;
    CK_SESSION_HANDLE sessionID;
    PK11Session *session;

    slot = pk11_SlotFromID(slotID);
    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    /* new session (we only have serial sessions) */
    session = pk11_NewSession(slotID, Notify, pApplication,
						 flags | CKF_SERIAL_SESSION);
    if (session == NULL) return CKR_HOST_MEMORY;

    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
    sessionID = slot->sessionIDCount++;
    if (slotID == PRIVATE_KEY_SLOT_ID) {
	sessionID |= PK11_PRIVATE_KEY_FLAG;
    } else if (flags & CKF_RW_SESSION) {
	/* NETSCAPE_SLOT_ID is Read ONLY */
	session->info.flags &= ~CKF_RW_SESSION;
    }

    session->handle = sessionID;
    pk11_update_state(slot, session);
    pk11queue_add(session, sessionID, slot->head, SESSION_HASH_SIZE);
    slot->sessionCount++;
    if (session->info.flags & CKF_RW_SESSION) {
	slot->rwSessionCount++;
    }
    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)

    *phSession = sessionID;
    return CKR_OK;
}


/* C_CloseSession closes a session between an application and a token. */
CK_RV C_CloseSession(CK_SESSION_HANDLE hSession) {
    PK11Slot *slot;
    PK11Session *session;
    SECItem *pw = NULL;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    slot = pk11_SlotFromSession(session);

    /* lock */
    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
    if (session->next || session->prev) {
	pk11queue_delete(session,hSession,slot->head,SESSION_HASH_SIZE);
	session->refCount--; /* can't go to zero while we hold the reference */
	slot->sessionCount--;
	if (session->info.flags & CKF_RW_SESSION) {
	    slot->rwSessionCount--;
	}
    }
    if (slot->sessionCount == 0) {
	pw = slot->password;
	slot->isLoggedIn = PR_FALSE;
	slot->password = NULL;
    }
    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)

    pk11_FreeSession(session);
    if (pw) SECITEM_ZfreeItem(pw, PR_TRUE);
    return CKR_OK;
}


/* C_CloseAllSessions closes all sessions with a token. */
CK_RV C_CloseAllSessions (CK_SLOT_ID slotID) {
    PK11Slot *slot;
    SECItem *pw = NULL;
#ifdef notdef
    PK11Session *session;
    int i;
#endif

    slot = pk11_SlotFromID(slotID);
    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    /* first log out the card */
    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
    pw = slot->password;
    slot->isLoggedIn = PR_FALSE;
    slot->password = NULL;
    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)
    if (pw) SECITEM_ZfreeItem(pw, PR_TRUE);

/*
 * until fips get's it's own slot, don't close all the sessions, some are new
 * sessions already created by the fips module.
 */
	return CKR_OK;

#ifdef notdef
    /* now close all the current sessions */
    /* NOTE: If you try to open new sessions before C_CloseAllSessions
     * completes, some of those new sessions may or may not be closed by
     * C_CloseAllSessions... but any session running when this code starts
     * will guarrenteed be close, and not session will be partially closed */
    for (i=0; i < SESSION_HASH_SIZE; i++) {
	do {
	    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
	    session = slot->head[i];
	    /* hand deque */
	    /* this duplicates manu of C_close session functions, but because
	     * we know that we are freeing all the sessions, we and do some
	     * more efficient processing */
	    if (session) {
		slot->head[i] = session->next;
		if (session->next) session->next->prev = NULL;
		session->next = session->prev = NULL;
		slot->sessionCount--;
		if (session->info.flags & CKF_RW_SESSION) {
		    slot->rwSessionCount--;
		}
	    }
	    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)
	    if (session) pk11_FreeSession(session);
	} while (session != NULL);
    }
#endif
    return CKR_OK;
}


/* C_GetSessionInfo obtains information about the session. */
CK_RV C_GetSessionInfo(CK_SESSION_HANDLE hSession,
    						CK_SESSION_INFO_PTR pInfo) {
    PK11Session *session;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;

    PORT_Memcpy(pInfo,&session->info,sizeof(CK_SESSION_INFO));
    pk11_FreeSession(session);
    return CKR_OK;
}

/* C_Login logs a user into a token. */
CK_RV C_Login(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType,
				    CK_CHAR_PTR pPin, CK_ULONG ulPinLen) {
    PK11Slot *slot;
    PK11Session *session;
    SECKEYKeyDBHandle *handle;
    SECItem *pin;
    char pinStr[256];


    /* get the slot */
    slot = pk11_SlotFromSessionHandle(hSession);

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
	/* this is a FIPS HACK. FIPS requires all slots to authenticate.
	 * we do the authentication once against the private key slot, but
	 * the code calling us doesn't know this, so we force ops on the
	 * nescape slot to move the the private key slot, this results in
	 * a (potentially) invalid session handle. Make sure the handle is
	 * really invalid before we fail. */
	session = pk11_SessionFromHandle(PK11_TOSLOT1(hSession));
	if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    }
    pk11_FreeSession(session);

    /* can't log into the Netscape Slot */
    if (slot->slotID == NETSCAPE_SLOT_ID)
	 return CKR_USER_TYPE_INVALID;

    if (slot->isLoggedIn) return CKR_USER_ALREADY_LOGGED_IN;
    slot->ssoLoggedIn = PR_FALSE;

    if (ulPinLen > 255) return CKR_PIN_LEN_RANGE;

    /* convert to null terminated string */
    PORT_Memcpy(pinStr,pPin,ulPinLen);
    pinStr[ulPinLen] = 0; 

    handle = SECKEY_GetDefaultKeyDB();

    /*
     * Deal with bootstrap. We allow the SSO to login in with a NULL
     * password if and only if we haven't initialized the KEY DB yet.
     * We only allow this on a RW session.
     */
    if (SECKEY_HasKeyDBPassword(handle) == SECFailure) {
	/* allow SSO's to log in only if there is not password on the
	 * key database */
	if ((userType == CKU_SO) && (session->info.flags & CKF_RW_SESSION)) {
	    /* should this be a fixed password? */
	    if (ulPinLen == 0) {
		SECItem *pw;
    		PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
		pw = slot->password;
		slot->password = NULL;
		slot->isLoggedIn = PR_TRUE;
		slot->ssoLoggedIn = PR_TRUE;
		PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)
		pk11_update_all_states(slot);
		SECITEM_ZfreeItem(pw,PR_TRUE);
		return CKR_OK;
	    }
	    return CKR_PIN_INCORRECT;
	} 
	return CKR_USER_TYPE_INVALID;
    } 

    /* don't allow the SSO to log in if the user is already initialized */
    if (userType != CKU_USER) { return CKR_USER_TYPE_INVALID; }


    /* build the hashed pins which we pass around */
    pin = SECKEY_HashPassword(pinStr,handle->global_salt);
    if (pin == NULL) return CKR_HOST_MEMORY;

    if (SECKEY_CheckKeyDBPassword(handle,pin) == SECSuccess) {
	SECItem *tmp;
	PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
	tmp = slot->password;
	slot->isLoggedIn = PR_TRUE;
	slot->password = pin;
	PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)
        if (tmp) SECITEM_ZfreeItem(tmp, PR_TRUE);

	/* update all sessions */
	pk11_update_all_states(slot);
	return CKR_OK;
    }

    SECITEM_ZfreeItem(pin, PR_TRUE);
    return CKR_PIN_INCORRECT;
}

/* C_Logout logs a user out from a token. */
CK_RV C_Logout(CK_SESSION_HANDLE hSession) {
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    PK11Session *session;
    SECItem *pw = NULL;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
	/* this is a FIPS HACK. FIPS requires all slots to authenticate.
	 * we do the authentication once against the private key slot, but
	 * the code calling us doesn't know this, so we force ops on the
	 * nescape slot to move the the private key slot, this results in
	 * a (potentially) invalid session handle. Make sure the handle is
	 * really invalid before we fail. */
	session = pk11_SessionFromHandle(PK11_TOSLOT1(hSession));
	if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    }

    if (!slot->isLoggedIn) return CKR_USER_NOT_LOGGED_IN;

    PK11_USE_THREADS(PR_Lock(slot->sessionLock);)
    pw = slot->password;
    slot->isLoggedIn = PR_FALSE;
    slot->ssoLoggedIn = PR_FALSE;
    slot->password = NULL;
    PK11_USE_THREADS(PR_Unlock(slot->sessionLock);)
    if (pw) SECITEM_ZfreeItem(pw, PR_TRUE);

    pk11_update_all_states(slot);
    return CKR_OK;
}


/* C_CreateObject creates a new object. */
CK_RV C_CreateObject(CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, 
					CK_OBJECT_HANDLE_PTR phObject) {
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    PK11Session *session;
    PK11Object *object;
    CK_RV rv;
    int i;


    /*
     * now lets create an object to hang the attributes off of
     */
    object = pk11_NewObject(slot); /* fill in the handle later */
    if (object == NULL) {
	return CKR_HOST_MEMORY;
    }

    /*
     * load the template values into the object
     */
    for (i=0; i < ulCount; i++) {
	rv = pk11_AddAttributeType(object,pk11_attr_expand(&pTemplate[i]));
	if (rv != CKR_OK) {
	    pk11_FreeObject(object);
	    return rv;
	}
    }

    /* get the session */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    /*
     * handle the base object stuff
     */
    rv = pk11_handleObject(object,session);
    pk11_FreeSession(session);
    if (rv != CKR_OK) {
	pk11_FreeObject(object);
	return rv;
    }

    *phObject = object->handle;
    return CKR_OK;
}


/* C_CopyObject copies an object, creating a new object for the copy. */
CK_RV C_CopyObject(CK_SESSION_HANDLE hSession,
       CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
					CK_OBJECT_HANDLE_PTR phNewObject) {
    PK11Object *destObject,*srcObject;
    PK11Session *session;
    CK_RV rv = CKR_OK;
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    int i;

    /* Get srcObject so we can find the class */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    srcObject = pk11_ObjectFromHandle(hObject,session);
    if (srcObject == NULL) {
	pk11_FreeSession(session);
	return CKR_OBJECT_HANDLE_INVALID;
    }
    /*
     * create an object to hang the attributes off of
     */
    destObject = pk11_NewObject(slot); /* fill in the handle later */
    if (destObject == NULL) {
	pk11_FreeSession(session);
        pk11_FreeObject(srcObject);
	return CKR_HOST_MEMORY;
    }

    /*
     * load the template values into the object
     */
    for (i=0; i < ulCount; i++) {
	if (pk11_modifyType(pTemplate[i].type,srcObject->class) == PK11_NEVER) {
	    rv = CKR_ATTRIBUTE_READ_ONLY;
	    break;
	}
	rv = pk11_AddAttributeType(destObject,pk11_attr_expand(&pTemplate[i]));
	if (rv != CKR_OK) { break; }
    }
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
        pk11_FreeObject(srcObject);
	pk11_FreeObject(destObject);
	return rv;
    }

    /* sensitive can only be changed to TRUE */
    if (pk11_hasAttribute(destObject,CKA_SENSITIVE)) {
	if (!pk11_isTrue(destObject,CKA_SENSITIVE)) {
	    pk11_FreeSession(session);
            pk11_FreeObject(srcObject);
	    pk11_FreeObject(destObject);
	    return CKR_ATTRIBUTE_READ_ONLY;
	}
    }

    /*
     * now copy the old attributes from the new attributes
     */
    /* don't create a token object if we aren't in a rw session */
    /* sigh we need to hold the lock to copy a consistant version of
     * the object. */
    rv = pk11_CopyObject(destObject,srcObject);

    destObject->class = srcObject->class;
    pk11_FreeObject(srcObject);
    if (rv != CKR_OK) {
	pk11_FreeObject(destObject);
	pk11_FreeSession(session);
    }

    rv = pk11_handleObject(destObject,session);
    *phNewObject = destObject->handle;
    pk11_FreeSession(session);
    if (rv != CKR_OK) {
	pk11_FreeObject(destObject);
	return rv;
    }
    
    return CKR_OK;
}


/* C_DestroyObject destroys an object. */
CK_RV C_DestroyObject(CK_SESSION_HANDLE hSession,
		 				CK_OBJECT_HANDLE hObject) {
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    PK11Session *session;
    PK11Object *object;
    PK11FreeStatus status;

    /*
     * This whole block just makes sure we really can destroy the
     * requested object.
     */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    object = pk11_ObjectFromHandle(hObject,session);
    if (object == NULL) {
	pk11_FreeSession(session);
	return CKR_OBJECT_HANDLE_INVALID;
    }

    /* don't destroy a private object if we aren't logged in */
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(pk11_isTrue(object,CKA_PRIVATE))) {
	pk11_FreeSession(session);
	pk11_FreeObject(object);
	return CKR_USER_NOT_LOGGED_IN;
    }

    /* don't destroy a token object if we aren't in a rw session */

    if (((session->info.flags & CKF_RW_SESSION) == 0) &&
				(pk11_isTrue(object,CKA_TOKEN))) {
	pk11_FreeSession(session);
	pk11_FreeObject(object);
	return CKR_SESSION_READ_ONLY;
    }

    /* ACTUALLY WE NEED TO DEAL WITH TOKEN OBJECTS AS WELL */
    PK11_USE_THREADS(PR_Lock(session->objectLock);)
    pk11_DeleteObject(session,object);
    PK11_USE_THREADS(PR_Unlock(session->objectLock);)


    pk11_FreeSession(session);

    /*
     * get some indication if the object is destroyed. Note: this is not
     * 100%. Someone mey have an object reference outstanding (though that
     * should not be the case by here. Also not that the object is "half"
     * destroyed. Our internal representation is destroyed, but it is still
     * in the data base (sigh).
     */
    status = pk11_FreeObject(object);

    return (status != PK11_DestroyFailure) ? CKR_OK : CKR_DEVICE_ERROR;
}


/* C_GetObjectSize gets the size of an object in bytes. */
CK_RV C_GetObjectSize(CK_SESSION_HANDLE hSession,
    			CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize) {
    *pulSize = 0;
    return CKR_OK;
}


/* C_GetAttributeValue obtains the value of one or more object attributes. */
CK_RV C_GetAttributeValue(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount) {
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    PK11Session *session;
    PK11Object *object;
    PK11Attribute *attribute;
    PRBool sensitive;
    int i;

    /*
     * make sure we're allowed
     */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    object = pk11_ObjectFromHandle(hObject,session);
    pk11_FreeSession(session);
    if (object == NULL) {
	return CKR_OBJECT_HANDLE_INVALID;
    }

    /* don't read a private object if we aren't logged in */
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(pk11_isTrue(object,CKA_PRIVATE))) {
	pk11_FreeObject(object);
	return CKR_USER_NOT_LOGGED_IN;
    }

    sensitive = pk11_isTrue(object,CKA_SENSITIVE);
    for (i=0; i < ulCount; i++) {
	/* Make sure that this attribute is retrievable */
	if (sensitive && pk11_isSensitive(pTemplate[i].type,object->class)) {
	    pk11_FreeObject(object);
	    return CKR_ATTRIBUTE_SENSITIVE;
	}
	attribute = pk11_FindAttribute(object,pTemplate[i].type);
	if (attribute == NULL) {
	    pk11_FreeObject(object);
	    return CKR_ATTRIBUTE_TYPE_INVALID;
	}
	if (pTemplate[i].pValue != NULL) {
	    PORT_Memcpy(pTemplate[i].pValue,attribute->attrib.pValue,
						attribute->attrib.ulValueLen);
	}
	pTemplate[i].ulValueLen = attribute->attrib.ulValueLen;
	pk11_FreeAttribute(attribute);
    }

    pk11_FreeObject(object);
    return CKR_OK;
}

/* C_SetAttributeValue modifies the value of one or more object attributes */
CK_RV C_SetAttributeValue (CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount) {
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    PK11Session *session;
    PK11Attribute *attribute;
    PK11Object *object;
    PRBool isToken;
    int i;

    /*
     * make sure we're allowed
     */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    object = pk11_ObjectFromHandle(hObject,session);
    if (object == NULL) {
        pk11_FreeSession(session);
	return CKR_OBJECT_HANDLE_INVALID;
    }

    /* don't modify a private object if we aren't logged in */
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(pk11_isTrue(object,CKA_PRIVATE))) {
	pk11_FreeSession(session);
	pk11_FreeObject(object);
	return CKR_USER_NOT_LOGGED_IN;
    }

    /* don't modify a token object if we aren't in a rw session */
    isToken = pk11_isTrue(object,CKA_TOKEN);
    if (((session->info.flags & CKF_RW_SESSION) == 0) && isToken) {
	pk11_FreeSession(session);
	pk11_FreeObject(object);
	return CKR_SESSION_READ_ONLY;
    }
    pk11_FreeSession(session);


    for (i=0; i < ulCount; i++) {
	PK11Attribute *newAttribute;
	/* Make sure that this attribute is retrievable */
	switch (pk11_modifyType(pTemplate[i].type,object->class)) {
	case PK11_NEVER:
	case PK11_ONCOPY:
        default:
	    pk11_FreeObject(object);
	    return CKR_ATTRIBUTE_READ_ONLY;

        case PK11_SENSITIVE:
	    if ((*(CK_BBOOL *)pTemplate[i].pValue) == FALSE) {
		pk11_FreeObject(object);
	        return CKR_ATTRIBUTE_READ_ONLY;
	    }
        case PK11_ALWAYS:
	    break;
	}
	/* find the old attribute */
	attribute = pk11_FindAttribute(object,pTemplate[i].type);
	if (attribute == NULL) {
	    pk11_FreeObject(object);
	    return CKR_ATTRIBUTE_TYPE_INVALID;
	}

	/* Create the new Attribute */
	newAttribute = pk11_NewAttribute(pk11_attr_expand(&pTemplate[i]));
	if (newAttribute == NULL) {
    	    pk11_FreeAttribute(attribute);
	    pk11_FreeObject(object);
	    return CKR_HOST_MEMORY;
	}

	/* delete the old attribute */
	pk11_DeleteAttribute(object,attribute);

	/* add the new one to our list */
	pk11_AddAttribute(object,newAttribute);
    }

    pk11_FreeObject(object);
    return CKR_OK;
}

/* stolen from keydb.c */
#define KEYDB_PW_CHECK_STRING   "password-check"
#define KEYDB_PW_CHECK_LEN      14

SECKEYLowPrivateKey * SECKEY_DecryptKey(DBT *key, SECItem *pwitem,
					SECKEYKeyDBHandle *handle);
typedef struct keyNode {
    struct keyNode *next;
    SECKEYLowPrivateKey *privKey;
    CERTCertificate *cert;
    SECItem *pubItem;
} keyNode;

typedef struct {
    PRArenaPool *arena;
    keyNode *head;
    PK11Slot *slot;
} keyList;

static SECStatus
add_key_to_list(DBT *key, DBT *data, void *arg)
{
    keyList *keylist;
    keyNode *node;
    void *keydata;
    SECKEYLowPrivateKey *privKey = NULL;
    
    keylist = (keyList *)arg;

    privKey = SECKEY_DecryptKey(key, keylist->slot->password,
				SECKEY_GetDefaultKeyDB());
    if ( privKey == NULL ) {
	goto loser;
    }

    /* allocate the node struct */
    node = PORT_ArenaZAlloc(keylist->arena, sizeof(keyNode));
    if ( node == NULL ) {
	goto loser;
    }
    
    /* allocate room for key data */
    keydata = PORT_ArenaZAlloc(keylist->arena, key->size);
    if ( keydata == NULL ) {
	goto loser;
    }

    /* link node into list */
    node->next = keylist->head;
    keylist->head = node;
    
    node->privKey = privKey;
    switch (privKey->keyType) {
      case rsaKey:
	node->pubItem = &privKey->u.rsa.modulus;
	break;
      case dsaKey:
	node->pubItem = &privKey->u.dsa.publicValue;
	break;
      default:
	break;
    }
    
    return(SECSuccess);
loser:
    if ( privKey ) {
	SECKEY_LowDestroyPrivateKey(privKey);
    }
    return(SECSuccess);
}

/*
 * If the cert is a user cert, then try to match it to a key on the
 * linked list of private keys built earlier.
 * If the cert matches one on the list, then save it.
 */
static SECStatus
add_cert_to_list(CERTCertificate *cert, SECItem *k, void *pdata)
{
    keyList *keylist;
    keyNode *node;
    SECKEYPublicKey *pubKey = NULL;
    SECItem *pubItem;
    CERTCertificate *oldcert;
    
    keylist = (keyList *)pdata;
    
    /* only if it is a user cert and has a nickname!! */
    if ( ( ( cert->trust->sslFlags & CERTDB_USER ) ||
	  ( cert->trust->emailFlags & CERTDB_USER ) ||
	  ( cert->trust->objectSigningFlags & CERTDB_USER ) ) &&
	( cert->nickname != NULL ) ) {

	/* get cert's public key */
	pubKey = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
	if ( pubKey == NULL ) {
	    goto done;
	}
	/* get value to compare from the cert's public key */
	switch ( pubKey->keyType ) {
	  case rsaKey:
	    pubItem = &pubKey->u.rsa.modulus;
	    break;
	  case dsaKey:
	    pubItem = &pubKey->u.dsa.publicValue;
	    break;
	  default:
	    goto done;
	}

	node = keylist->head;
	while ( node ) {
	    /* if key type is different, then there is no match */
	    if ( node->privKey->keyType == pubKey->keyType ) {

		/* compare public value from cert with public value from
		 * the key
		 */
		if ( SECITEM_CompareItem(pubItem, node->pubItem) == SECEqual ){
		    /* this is a match */

		    /* if no cert has yet been found for this key, or this
		     * cert is newer, then save this cert
		     */
		    if ( ( node->cert == NULL ) || 
			CERT_IsNewer(cert, node->cert ) ) {

			oldcert = node->cert;
			
			/* get a real DB copy of the cert, since the one
			 * passed in here is not properly recorded in the
			 * temp database
			 */
			node->cert =
			    CERT_FindCertByKey(CERT_GetDefaultCertDB(),
					       &cert->certKey);

			/* free the old cert if there was one */
			if ( oldcert ) {
			    CERT_DestroyCertificate(oldcert);
			}
		    }
		}
	    }
	    
	    node = node->next;
	}
    }
done:
    if ( pubKey ) {
	SECKEY_DestroyPublicKey(pubKey);
    }

    return(SECSuccess);
}

static SECItem *
decodeKeyDBGlobalSalt(DBT *saltData)
{
    SECItem *saltitem;
    
    saltitem = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
    if ( saltitem == NULL ) {
	return(NULL);
    }
    
    saltitem->data = (unsigned char *)PORT_ZAlloc(saltData->size);
    if ( saltitem->data == NULL ) {
	PORT_Free(saltitem);
	return(NULL);
    }
    
    saltitem->len = saltData->size;
    PORT_Memcpy(saltitem->data, saltData->data, saltitem->len);
    
    return(saltitem);
}

/* C_FindObjectsInit initializes a search for token and session objects 
 * that match a template. */
CK_RV C_FindObjectsInit(CK_SESSION_HANDLE hSession,
    			CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount) {
    PK11ObjectListElement *objectList = NULL;
    PK11ObjectListElement *olp;
    PK11SearchResults *search,*freeSearch;
    PK11Session *session;
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    int count, i;
    CK_RV rv;
    SECStatus ret;
    keyList keylist;
    keyNode *node;
    PK11Object *privateKeyObject;
    PK11Object *certObject;
    
    if ((!slot->isLoggedIn) && (slot->needLogin)) {
	return CKR_USER_NOT_LOGGED_IN;
    }

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }

    keylist.arena = NULL;
    
    /* resync token objects with the data base */
    if (session->info.slotID == PRIVATE_KEY_SLOT_ID) {
	if (slot->DB_loaded == PR_FALSE) {
	    /* traverse the database, collecting the index keys of all
	     * records into a linked list
	     */
	    keylist.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	    if ( keylist.arena  == NULL ) {
		goto resync_done;
	    }
	    keylist.head = NULL;
	    keylist.slot = slot;
		
	    /* collect all of the keys */
	    ret = SECKEY_TraverseKeys(SECKEY_GetDefaultKeyDB(),
				      add_key_to_list, (void *)&keylist);
	    if (ret != SECSuccess) {
		goto resync_done;
	    }

	    /* find certs that match any of the keys */
	    rv = SEC_TraversePermCerts(CERT_GetDefaultCertDB(),
				       add_cert_to_list, (void *)&keylist);
	    if ( rv != SECSuccess ) {
		goto resync_done;
	    }

	    /* now traverse the list and entry certs/keys into the
	     * pkcs11 world
	     */
	    node = keylist.head;
	    while ( node ) {

		/* create the private key object */
		privateKeyObject = pk11_importPrivateKey(slot, node->privKey);
		if ( privateKeyObject == NULL ) {
		    goto end_loop;
		}

		/* add the key ID attribute */
		pk11_DeleteAttributeType(privateKeyObject, CKA_ID);
		if ( pk11_AddAttributeType(privateKeyObject, CKA_ID,
					   node->pubItem->data,
					   node->pubItem->len) != CKR_OK) {
		    pk11_FreeObject(privateKeyObject);
		    privateKeyObject = NULL;
		    goto end_loop;
		}

		if ( node->cert ) {
		    /* Now import the Cert */
		    certObject = pk11_importCertificate(slot, node->cert,
							node->pubItem->data,
							node->pubItem->len);

		    /* Copy the CK_ID & subject */
		    if ( certObject ) {
			/* NOTE: cert has been adopted */
			PK11Attribute *attribute;
			PK11Attribute *newAttribute;

			/* Copy the CKA_ID */
			attribute = pk11_FindAttribute(certObject, CKA_ID);
			newAttribute=
			    pk11_NewAttribute(pk11_attr_expand(&attribute->attrib));
			if (newAttribute != NULL) {
			    pk11_DeleteAttributeType(privateKeyObject, CKA_ID);
			    pk11_AddAttribute(privateKeyObject, newAttribute);
			}
			/* Copy the Subject */
			attribute = pk11_FindAttribute(certObject,CKA_SUBJECT);
			newAttribute=
			    pk11_NewAttribute(pk11_attr_expand(&attribute->attrib));
			if (newAttribute != NULL) {
			    pk11_DeleteAttributeType(privateKeyObject,
						     CKA_SUBJECT);
			    pk11_AddAttribute(privateKeyObject,
					      newAttribute);
			}
			pk11_AddSlotObject(slot, certObject);
		    }
		}

		pk11_AddSlotObject(slot, privateKeyObject);

end_loop:
		SECKEY_LowDestroyPrivateKey(node->privKey);
		if ( node->cert ) {
		    CERT_DestroyCertificate(node->cert);
		}
		
		node = node->next;
	    }
	    
	    slot->DB_loaded = PR_TRUE;
	}
    }

resync_done:
    if ( keylist.arena ) {
	PORT_FreeArena(keylist.arena, PR_FALSE);
    }
    
    /* build list of found objects in the session */
    rv = pk11_searchObjectList(&objectList,slot->tokObjects,
					slot->objectLock, pTemplate, ulCount);
    if (rv != CKR_OK) {
	pk11_FreeObjectList(objectList);
	pk11_FreeSession(session);
	return rv;
    }


    /* copy list to session */
    count = 0;
    for (olp = objectList; olp != NULL; olp = olp->next) {
	count++;
    }
    search = (PK11SearchResults *)PORT_Alloc(sizeof(PK11SearchResults));
    if (search != NULL) {
	search->handles = (CK_OBJECT_HANDLE *)
				PORT_Alloc(sizeof(CK_OBJECT_HANDLE) * count);
	if (search->handles != NULL) {
	    for (i=0; i < count; i++) {
		search->handles[i] = objectList->object->handle;
		objectList = pk11_FreeObjectListElement(objectList);
	    }
	} else {
	    PORT_Free(search);
	    search = NULL;
	}
    }
    if (search == NULL) {
	pk11_FreeObjectList(objectList);
	pk11_FreeSession(session);
	return CKR_OK;
    }

    /* store the search info */
    search->index = 0;
    search->size = count;
    if ((freeSearch = session->search) != NULL) {
	session->search = NULL;
	pk11_FreeSearch(freeSearch);
    }
    session->search = search;
    pk11_FreeSession(session);
    return CKR_OK;
}


/* C_FindObjects continues a search for token and session objects 
 * that match a template, obtaining additional object handles. */
CK_RV C_FindObjects(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE_PTR phObject,CK_ULONG ulMaxObjectCount,
    					CK_ULONG_PTR pulObjectCount) {
    PK11Session *session;
    PK11SearchResults *search;
    int	transfer;
    int left;

    *pulObjectCount = 0;
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    if (session->search == NULL) {
	pk11_FreeSession(session);
	return CKR_OK;
    }
    search = session->search;
    left = session->search->size - session->search->index;
    transfer = (ulMaxObjectCount > left) ? left : ulMaxObjectCount;
    PORT_Memcpy(phObject,&search->handles[search->index],
					transfer*sizeof(CK_OBJECT_HANDLE_PTR));
    search->index += transfer;
    if (search->index == search->size) {
	session->search = NULL;
	pk11_FreeSearch(search);
    }
    *pulObjectCount = transfer;
    return CKR_OK;
}

/* C_FindObjectsFinal finishes a search for token and session objects. */
CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE hSession) {
    PK11Session *session;
    PK11SearchResults *search;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    search = session->search;
    session->search = NULL;
    if (search == NULL) {
	pk11_FreeSession(session);
	return CKR_OK;
    }
    pk11_FreeSearch(search);
    return CKR_OK;
}



/*
 ************** Crypto Functions:     Utilities ************************
 */

/*
 * All the C_InitXXX functions have a set of common checks and processing they
 * all need to do at the beginning. This is done here.
 */
static
CK_RV pk11_InitGeneric(PK11Session *session,PK11SessionContext **contextPtr,
    PK11ContextType ctype,PK11Object **keyPtr, CK_OBJECT_HANDLE hKey,
	 CK_KEY_TYPE *keyTypePtr, CK_OBJECT_CLASS pubKeyType,
						 CK_ATTRIBUTE_TYPE operation) {
    PK11Object *key = NULL;
    PK11Attribute *att;
    PK11SessionContext *context;

    /* We can only init if there is not current context active */
    if (session->context != NULL)  {
	return CKR_OPERATION_ACTIVE;
    }

    /* find the key */
    if (keyPtr) {
        key = pk11_ObjectFromHandle(hKey,session);
        if (key == NULL) {
	    return CKR_KEY_HANDLE_INVALID;
    	}

	/* make sure it's a valid  key for this operation */
	if (((key->class != CKO_SECRET_KEY) && (key->class != pubKeyType))
					|| !pk11_isTrue(key,operation)) {
	    pk11_FreeObject(key);
	    return CKR_KEY_TYPE_INCONSISTENT;
	}
	/* get the key type */
	att = pk11_FindAttribute(key,CKA_KEY_TYPE);
	PORT_Assert(att != NULL);
	*keyTypePtr = *(CK_KEY_TYPE *)att->attrib.pValue;
	pk11_FreeAttribute(att);
	*keyPtr = key;
    }

    /* allocate the context structure */
    context = (PK11SessionContext *)PORT_Alloc(sizeof(PK11SessionContext));
    if (context == NULL) {
	if (key) pk11_FreeObject(key);
	return CKR_HOST_MEMORY;
    }
    context->type = ctype;
    context->multi = PR_TRUE;
    context->cipherInfo = NULL;

    *contextPtr = context;
    return CKR_OK;
}

/*
 * code to grab the context. Needed by every PK11_UpdateXXX, C_FinalXXX,
 * and C_XXX function
 */

static CK_RV
pk11_GetContext(CK_SESSION_HANDLE handle,PK11SessionContext **contextPtr,
	PK11ContextType type, PRBool needMulti, PK11Session **sessionPtr) {
    PK11Session *session;
    PK11SessionContext *context;

    session = pk11_SessionFromHandle(handle);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    context = session->context;
    /* make sure the context is valid */
    if((context==NULL)||(context->type!=type)||(needMulti&&!(context->multi))){
        pk11_FreeSession(session);
	return CKR_OPERATION_NOT_INITIALIZED;
    }
    *contextPtr = context;
    if (sessionPtr != NULL) {
	*sessionPtr = session;
    } else {
	pk11_FreeSession(session);
    }
    return CKR_OK;
}

/*
 ************** Crypto Functions:     Encrypt ************************
 */

/* C_EncryptInit initializes an encryption operation. */
/* This function is used by C_EncryptInit and C_WrapKey. The only difference
 * in their uses if whether or not etype is CKA_ENCRYPT or CKA_WRAP */
static CK_RV pk11_EncryptInit(CK_SESSION_HANDLE hSession,
	 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey,
					      CK_ATTRIBUTE_TYPE etype) {
    PK11Session *session;
    PK11Object *key;
    PK11SessionContext *context;
    PK11Attribute *att;
    CK_RC2_CBC_PARAMS *rc2_param;
    CK_KEY_TYPE key_type;
    CK_RV rv = CKR_OK;
    SECKEYLowPublicKey *pubKey;
    unsigned effectiveKeyLength;
    int t;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;

    rv = pk11_InitGeneric(session,&context,PK11_ENCRYPT,&key,hKey,&key_type,
    						CKO_PUBLIC_KEY, etype);
						
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
	return rv;
    }

    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	pubKey = pk11_GetPubKey(key,CKK_RSA);
	if (pubKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = pubKey;
	context->update = (PK11Cipher) (pMechanism->mechanism == CKM_RSA_X_509
			? RSA_EncryptRaw : RSA_EncryptBlock);
	context->destroy = pk11_Null;
	break;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	if (key_type != CKK_RC2) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = pk11_FindAttribute(key,CKA_VALUE);
	rc2_param = (CK_RC2_CBC_PARAMS *)pMechanism->pParameter;
	effectiveKeyLength = (rc2_param->ulEffectiveBits+7)/8;
	context->cipherInfo = RC2_CreateContext(att->attrib.pValue,
		att->attrib.ulValueLen, rc2_param->iv,
		 pMechanism->mechanism == CKM_RC2_ECB ? SEC_RC2 :
					 SEC_RC2_CBC,effectiveKeyLength);
	pk11_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (PK11Cipher) RC2_Encrypt;
	context->destroy = (PK11Destroy) RC2_DestroyContext;
	break;
    case CKM_RC4:
	if (key_type != CKK_RC4) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = pk11_FindAttribute(key,CKA_VALUE);
	context->cipherInfo = RC4_CreateContext(att->attrib.pValue,
						att->attrib.ulValueLen);
	pk11_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (PK11Cipher) RC4_Encrypt;
	context->destroy = (PK11Destroy) RC4_DestroyContext;
	break;
    case CKM_DES_ECB:
	if (key_type != CKK_DES) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES;
	goto finish_des;
    case CKM_DES_CBC:
	if (key_type != CKK_DES) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES_CBC;
	goto finish_des;
    case CKM_DES3_ECB:
	if ((key_type != CKK_DES2) && (key_type != CKK_DES3)) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES_EDE3;
	goto finish_des;
    case CKM_DES3_CBC:
	if ((key_type != CKK_DES2) && (key_type != CKK_DES3)) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES_EDE3_CBC;
finish_des:
	att = pk11_FindAttribute(key,CKA_VALUE);
	context->cipherInfo = DES_CreateContext(att->attrib.pValue,
				pMechanism->pParameter,t, PR_TRUE);
	pk11_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (PK11Cipher) DES_Encrypt;
	context->destroy = (PK11Destroy) DES_DestroyContext;

	break;
    default:
	rv = CKR_MECHANISM_INVALID;
	break;
    }

    pk11_FreeObject(key);
    if (rv != CKR_OK) {
        pk11_FreeContext(context);
	pk11_FreeSession(session);
	return rv;
    }
    session->context = context;
    pk11_FreeSession(session);
    return CKR_OK;
}

/* C_EncryptInit initializes an encryption operation. */
CK_RV C_EncryptInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
	return pk11_EncryptInit(hSession, pMechanism, hKey, CKA_ENCRYPT);
}

/* C_Encrypt encrypts single-part data. */
CK_RV C_Encrypt (CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    		CK_ULONG ulDataLen, CK_BYTE_PTR pEncryptedData,
					 CK_ULONG_PTR pulEncryptedDataLen) {
    PK11Session *session;
    PK11SessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulEncryptedDataLen;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_ENCRYPT,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    /* do it: NOTE: this assumes buf size is big enough. */
    rv = (*context->update)(context->cipherInfo, pEncryptedData, 
					&outlen, maxoutlen, pData, ulDataLen);
    *pulEncryptedDataLen = (CK_ULONG) outlen;
    pk11_FreeContext(context);
    session->context = NULL;
    pk11_FreeSession(session);

    return (rv == SECSuccess) ? CKR_OK : CKR_DEVICE_ERROR;
}


/* C_EncryptUpdate continues a multiple-part encryption operation. */
CK_RV C_EncryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart,	
					CK_ULONG_PTR pulEncryptedPartLen) {
    PK11SessionContext *context;
    unsigned int outlen;
    unsigned int maxout = *pulEncryptedPartLen;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_ENCRYPT,PR_TRUE,NULL);
    if (crv != CKR_OK) return crv;

    /* do it: NOTE: this assumes buf size in is >= buf size out! */
    rv = (*context->update)(context->cipherInfo,pEncryptedPart, 
					&outlen, maxout, pPart, ulPartLen);
    *pulEncryptedPartLen = (CK_ULONG) outlen;
    return (rv == SECSuccess) ? CKR_OK : CKR_DEVICE_ERROR;
}


/* C_EncryptFinal finishes a multiple-part encryption operation. */
CK_RV C_EncryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastEncryptedPart, CK_ULONG_PTR pulLastEncryptedPartLen) {
    PK11Session *session;
    PK11SessionContext *context;
    CK_RV crv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_ENCRYPT,PR_TRUE,&session);
    if (crv != CKR_OK) return crv;

    /* do it */
    session->context = NULL;
    *pulLastEncryptedPartLen = 0;
    pk11_FreeContext(context);
    pk11_FreeSession(session);
    return CKR_OK;
}

/*
 ************** Crypto Functions:     Decrypt ************************
 */

/* C_DecryptInit initializes a decryption operation. */
static CK_RV pk11_DecryptInit( CK_SESSION_HANDLE hSession,
  CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey, CK_ATTRIBUTE_TYPE dtype){
    PK11Session *session;
    PK11Object *key;
    PK11Attribute *att;
    PK11SessionContext *context;
    CK_RC2_CBC_PARAMS *rc2_param;
    CK_KEY_TYPE key_type;
    CK_RV rv = CKR_OK;
    unsigned effectiveKeyLength;
    SECKEYLowPrivateKey *privKey;
    int t;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;

    rv = pk11_InitGeneric(session,&context,PK11_DECRYPT,&key,hKey,&key_type,
						CKO_PRIVATE_KEY,dtype);
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
	return rv;
    }

    /*
     * now handle each mechanism
     */
    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	privKey = pk11_GetPrivKey(key,CKK_RSA);
	if (privKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = privKey;
	context->update = (PK11Cipher) (pMechanism->mechanism == CKM_RSA_X_509
			? RSA_DecryptRaw : RSA_DecryptBlock);
	context->destroy = (context->cipherInfo == key->objectInfo) ?
		(PK11Destroy) pk11_Null : (PK11Destroy) pk11_FreePrivKey;
	break;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	if (key_type != CKK_RC2) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = pk11_FindAttribute(key,CKA_VALUE);
	rc2_param = (CK_RC2_CBC_PARAMS *)pMechanism->pParameter;
	effectiveKeyLength = (rc2_param->ulEffectiveBits+7)/8;
	context->cipherInfo = RC2_CreateContext(att->attrib.pValue,
		att->attrib.ulValueLen, rc2_param->iv,
		 pMechanism->mechanism == CKM_RC2_ECB ? SEC_RC2 :
					 SEC_RC2_CBC, effectiveKeyLength);
	pk11_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (PK11Cipher) RC2_Decrypt;
	context->destroy = (PK11Destroy) RC2_DestroyContext;
	break;
    case CKM_RC4:
	if (key_type != CKK_RC4) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = pk11_FindAttribute(key,CKA_VALUE);
	context->cipherInfo = RC4_CreateContext(att->attrib.pValue,
						att->attrib.ulValueLen);
	pk11_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (PK11Cipher) RC4_Decrypt;
	context->destroy = (PK11Destroy) RC4_DestroyContext;
	break;
    case CKM_DES_ECB:
	if (key_type != CKK_DES) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES;
	goto finish_des;
    case CKM_DES_CBC:
	if (key_type != CKK_DES) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES_CBC;
	goto finish_des;
    case CKM_DES3_ECB:
	if ((key_type != CKK_DES2) && (key_type != CKK_DES3)) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES_EDE3;
	goto finish_des;
    case CKM_DES3_CBC:
	if ((key_type != CKK_DES2) && (key_type != CKK_DES3)) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = SEC_DES_EDE3_CBC;
finish_des:
	att = pk11_FindAttribute(key,CKA_VALUE);
	context->cipherInfo = DES_CreateContext(att->attrib.pValue,
				pMechanism->pParameter,t, PR_FALSE);
	pk11_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (PK11Cipher) DES_Decrypt;
	context->destroy = (PK11Destroy) DES_DestroyContext;

	break;
    default:
	rv = CKR_MECHANISM_INVALID;
	break;
    }

    pk11_FreeObject(key);
    if (rv != CKR_OK) {
        pk11_FreeContext(context);
	pk11_FreeSession(session);
	return rv;
    }
    session->context = context;
    return CKR_OK;
}

/* C_DecryptInit initializes a decryption operation. */
CK_RV C_DecryptInit( CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    return pk11_DecryptInit(hSession,pMechanism,hKey,CKA_DECRYPT);
}

/* C_Decrypt decrypts encrypted data in a single part. */
CK_RV C_Decrypt(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedData,CK_ULONG ulEncryptedDataLen,CK_BYTE_PTR pData,
    						CK_ULONG_PTR pulDataLen) {
    PK11Session *session;
    PK11SessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulDataLen;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_DECRYPT,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    rv = (*context->update)(context->cipherInfo, pData, &outlen, maxoutlen, 
					pEncryptedData, ulEncryptedDataLen);
    *pulDataLen = (CK_ULONG) outlen;
    pk11_FreeContext(context);
    session->context = NULL;
    pk11_FreeSession(session);
    return (rv == SECSuccess)  ? CKR_OK : CKR_DEVICE_ERROR;
}


/* C_DecryptUpdate continues a multiple-part decryption operation. */
CK_RV C_DecryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart, CK_ULONG ulEncryptedPartLen,
    				CK_BYTE_PTR pPart, CK_ULONG_PTR pulPartLen) {
    PK11SessionContext *context;
    unsigned int outlen;
    unsigned int maxout = *pulPartLen;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_DECRYPT,PR_TRUE,NULL);
    if (crv != CKR_OK) return crv;

    /* do it: NOTE: this assumes buf size in is >= buf size out! */
    rv = (*context->update)(context->cipherInfo,pPart, &outlen,
		 maxout, pEncryptedPart, ulEncryptedPartLen);
    *pulPartLen = (CK_ULONG) outlen;
    return (rv == SECSuccess)  ? CKR_OK : CKR_DEVICE_ERROR;
}


/* C_DecryptFinal finishes a multiple-part decryption operation. */
CK_RV C_DecryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastPart, CK_ULONG_PTR pulLastPartLen) {
    PK11Session *session;
    PK11SessionContext *context;
    CK_RV crv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_DECRYPT,PR_TRUE,&session);
    if (crv != CKR_OK) return crv;

    /* do it */
    session->context = NULL;
    *pulLastPartLen = 0;
    pk11_FreeContext(context);
    pk11_FreeSession(session);
    return CKR_OK;
}


/*
 ************** Crypto Functions:     Digest (HASH)  ************************
 */

/* C_DigestInit initializes a message-digesting operation. */
CK_RV C_DigestInit(CK_SESSION_HANDLE hSession,
    					CK_MECHANISM_PTR pMechanism) {
    PK11Session *session;
    PK11SessionContext *context;
    MD2Context *md2_context;
    MD5Context *md5_context;
    SHA1Context *sha1_context;
    CK_RV rv = CKR_OK;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    rv = pk11_InitGeneric(session,&context,PK11_HASH,NULL,0,NULL, 0, 0);
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
	return rv;
    }

    switch(pMechanism->mechanism) {
    case CKM_MD2:
	md2_context = MD2_NewContext();
	context->cipherInfo = (void *)md2_context;
	context->cipherInfoLen = sizeof(md2_context);
	context->currentMech = CKM_MD2;
        if (context->cipherInfo == NULL) {
	    rv= CKR_HOST_MEMORY;
	    
	}
	context->hashUpdate = (PK11Hash) MD2_Update;
	context->end = (PK11End) MD2_End;
	context->destroy = (PK11Destroy) MD2_DestroyContext;
	MD2_Begin(md2_context);
	break;
    case CKM_MD5:
	md5_context = MD5_NewContext();
	context->cipherInfo = (void *)md5_context;
	context->cipherInfoLen = sizeof(md5_context);
	context->currentMech = CKM_MD5;
        if (context->cipherInfo == NULL) {
	    rv= CKR_HOST_MEMORY;
	    
	}
	context->hashUpdate = (PK11Hash) MD5_Update;
	context->end = (PK11End) MD5_End;
	context->destroy = (PK11Destroy) MD5_DestroyContext;
	MD5_Begin(md5_context);
	break;
    case CKM_SHA_1:
	sha1_context = SHA1_NewContext();
	context->cipherInfo = (void *)sha1_context;
	context->cipherInfoLen = sizeof(sha1_context);
	context->currentMech = CKM_SHA_1;
        if (context->cipherInfo == NULL) {
	    rv= CKR_HOST_MEMORY;
	    break;
	}
	context->hashUpdate = (PK11Hash) SHA1_Update;
	context->end = (PK11End) SHA1_End;
	context->destroy = (PK11Destroy) SHA1_DestroyContext;
	SHA1_Begin(sha1_context);
	break;
    default:
	rv = CKR_MECHANISM_INVALID;
	break;
    }

    if (rv != CKR_OK) {
        pk11_FreeContext(context);
	pk11_FreeSession(session);
	return rv;
    }
    session->context = context;
    pk11_FreeSession(session);
    return CKR_OK;
}


/* C_Digest digests data in a single part. */
CK_RV C_Digest(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pulDigestLen) {
    PK11Session *session;
    PK11SessionContext *context;
    unsigned int digestLen;
    unsigned int maxout = *pulDigestLen;
    CK_RV crv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_HASH,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    /* do it: */
    (*context->hashUpdate)(context->cipherInfo, pData, ulDataLen);
    /*  NOTE: this assumes buf size is bigenough for the algorithm */
    (*context->end)(context->cipherInfo, pDigest, &digestLen,maxout);
    *pulDigestLen = digestLen;

    session->context = NULL;
    pk11_FreeContext(context);
    return CKR_OK;
}


/* C_DigestUpdate continues a multiple-part message-digesting operation. */
CK_RV C_DigestUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
					    CK_ULONG ulPartLen) {
    PK11SessionContext *context;
    CK_RV crv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_HASH,PR_TRUE,NULL);
    if (crv != CKR_OK) return crv;
    /* do it: */
    (*context->hashUpdate)(context->cipherInfo, pPart, ulPartLen);
    return CKR_OK;
}


/* C_DigestFinal finishes a multiple-part message-digesting operation. */
CK_RV C_DigestFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pulDigestLen) {
    PK11Session *session;
    PK11SessionContext *context;
    unsigned int maxout = *pulDigestLen;
    unsigned int digestLen;
    CK_RV crv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession, &context, PK11_HASH, PR_TRUE, &session);
    if (crv != CKR_OK) return crv;

    if (pDigest != NULL) {
        (*context->end)(context->cipherInfo, pDigest, &digestLen, maxout);
        *pulDigestLen = digestLen;
    } else {
	*pulDigestLen = 0;
    }

    session->context = NULL;
    pk11_FreeContext(context);
    pk11_FreeSession(session);
    return CKR_OK;
}


/*
 ************** Crypto Functions:     Sign  ************************
 */

/* C_SignInit initializes a signature (private key encryption) operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_RV C_SignInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    PK11Session *session;
    PK11Object *key;
    PK11SessionContext *context;
    CK_KEY_TYPE key_type;
    CK_RV rv = CKR_OK;
    SECKEYLowPrivateKey *privKey;
    unsigned char randomBlock[20];

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    rv = pk11_InitGeneric(session,&context,PK11_SIGN,&key,hKey,&key_type,
						CKO_PRIVATE_KEY,CKA_SIGN);
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
	return rv;
    }

    context->multi = PR_TRUE;

    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	privKey = pk11_GetPrivKey(key,CKK_RSA);
	if (privKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = privKey;
	context->update = (PK11Cipher) (pMechanism->mechanism == CKM_RSA_X_509
			? RSA_SignRaw : RSA_Sign);
	/* What's this magic about? well we don't 'cache' token versions
	 * of private keys because (1) it's sensitive data, and (2) it never
	 * gets destroyed. Instead we grab the key out of the database as
	 * necessary, but now the key is our context, and we need to free
	 * it when we are done. Non-token private keys will get freed when
	 * the user destroys the session object (or the session the session
	 * object lives in) */
	context->destroy = (context->cipherInfo == key->objectInfo) ?
		(PK11Destroy)pk11_Null:(PK11Destroy)pk11_FreePrivKey;
	break;
    case CKM_DSA:
	if (key_type != CKK_DSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	privKey = pk11_GetPrivKey(key,CKK_DSA);
	if (privKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = DSA_CreateSignContext(privKey);
	RNG_GenerateGlobalRandomBytes(randomBlock,sizeof(randomBlock));
        DSA_Presign((DSASignContext *)context->cipherInfo,randomBlock);
	context->update = (PK11Cipher) DSA_Sign;
	context->destroy = (PK11Destroy) DSA_DestroySignContext;
        if (key->objectInfo != privKey) SECKEY_LowDestroyPrivateKey(privKey);
	break;
    default:
	rv = CKR_MECHANISM_INVALID;
	break;
    }

    if (rv != CKR_OK) {
        PORT_Free(context);
	pk11_FreeSession(session);
	pk11_FreeObject(key);
	return rv;
    }
    session->context = context;
    return CKR_OK;
}


/* C_Sign signs (encrypts with private key) data in a single part,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_RV C_Sign(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,CK_ULONG ulDataLen,CK_BYTE_PTR pSignature,
    					CK_ULONG_PTR pulSignatureLen) {
    PK11Session *session;
    PK11SessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulSignatureLen;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_SIGN,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    rv = (*context->update)(context->cipherInfo, pSignature,
					&outlen, maxoutlen, pData, ulDataLen);
    *pulSignatureLen = (CK_ULONG) outlen;
    pk11_FreeContext(context);
    session->context = NULL;
    pk11_FreeSession(session);

    return (rv == SECSuccess) ? CKR_OK : CKR_DEVICE_ERROR;
}


/* C_SignUpdate continues a multiple-part signature operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_RV C_SignUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
    							CK_ULONG ulPartLen) {
    return CKR_OPERATION_NOT_INITIALIZED;
}


/* C_SignFinal finishes a multiple-part signature operation, 
 * returning the signature. */
CK_RV C_SignFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pSignature,
					    CK_ULONG_PTR pulSignatureLen) {
    return CKR_OPERATION_NOT_INITIALIZED;
}

/*
 ************** Crypto Functions:     Sign Recover  ************************
 */
/* C_SignRecoverInit initializes a signature operation,
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
CK_RV C_SignRecoverInit(CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    switch (pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	return C_SignInit(hSession,pMechanism,hKey);
    default:
	break;
    }
    return CKR_MECHANISM_INVALID;
}


/* C_SignRecover signs data in a single operation
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
CK_RV C_SignRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
  CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen) {
    return C_Sign(hSession,pData,ulDataLen,pSignature,pulSignatureLen);
}

/*
 ************** Crypto Functions:     verify  ************************
 */

/* C_VerifyInit initializes a verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature (e.g. DSA) */
CK_RV C_VerifyInit(CK_SESSION_HANDLE hSession,
			   CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    PK11Session *session;
    PK11Object *key;
    PK11SessionContext *context;
    CK_KEY_TYPE key_type;
    CK_RV rv = CKR_OK;
    SECKEYLowPublicKey *pubKey;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    rv = pk11_InitGeneric(session,&context,PK11_VERIFY,&key,hKey,&key_type,
						CKO_PUBLIC_KEY,CKA_VERIFY);
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
	return rv;
    }

    context->multi = PR_TRUE;

    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	pubKey = pk11_GetPubKey(key,CKK_RSA);
	if (pubKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = pubKey;
	context->verify = (PK11Verify) (pMechanism->mechanism == CKM_RSA_X_509
			? RSA_CheckSignRaw : RSA_CheckSign);
	context->destroy = pk11_Null;
	break;
    case CKM_DSA:
	if (key_type != CKK_DSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	pubKey = pk11_GetPubKey(key,CKK_DSA);
	if (pubKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = DSA_CreateVerifyContext(pubKey);
	context->verify = (PK11Verify) DSA_Verify;
	context->destroy = (PK11Destroy) DSA_DestroyVerifyContext;
	break;
    default:
	rv = CKR_MECHANISM_INVALID;
	break;
    }

    if (rv != CKR_OK) {
        PORT_Free(context);
	pk11_FreeSession(session);
	pk11_FreeObject(key);
	return rv;
    }
    session->context = context;
    return CKR_OK;
}


/* C_Verify verifies a signature in a single-part operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_RV C_Verify(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen) {
    PK11Session *session;
    PK11SessionContext *context;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_VERIFY,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    rv = (*context->verify)(context->cipherInfo,pSignature, ulSignatureLen,
							 pData, ulDataLen);
    pk11_FreeContext(context);
    session->context = NULL;
    pk11_FreeSession(session);

    return (rv == SECSuccess) ? CKR_OK : CKR_SIGNATURE_INVALID;

}


/* C_VerifyUpdate continues a multiple-part verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
CK_RV C_VerifyUpdate( CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart,
						CK_ULONG ulPartLen) {
    return CKR_OPERATION_NOT_INITIALIZED;
}


/* C_VerifyFinal finishes a multiple-part verification operation, 
 * checking the signature. */
CK_RV C_VerifyFinal(CK_SESSION_HANDLE hSession,
			CK_BYTE_PTR pSignature,CK_ULONG ulSignatureLen) {
    return CKR_OPERATION_NOT_INITIALIZED;
}

/*
 ************** Crypto Functions:     Verify  Recover ************************
 */

/* C_VerifyRecoverInit initializes a signature verification operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
CK_RV C_VerifyRecoverInit(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    PK11Session *session;
    PK11Object *key;
    PK11SessionContext *context;
    CK_KEY_TYPE key_type;
    CK_RV rv = CKR_OK;
    SECKEYLowPublicKey *pubKey;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    rv = pk11_InitGeneric(session,&context,PK11_VERIFY_RECOVER,
			&key,hKey,&key_type,CKO_PUBLIC_KEY,CKA_VERIFY_RECOVER);
    if (rv != CKR_OK) {
	pk11_FreeSession(session);
	return rv;
    }

    context->multi = PR_TRUE;

    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    rv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	pubKey = pk11_GetPubKey(key,CKK_RSA);
	if (pubKey == NULL) {
	    rv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = pubKey;
	context->update = (PK11Cipher) (pMechanism->mechanism == CKM_RSA_X_509
			? RSA_CheckSignRecoverRaw : RSA_CheckSignRecover);
	context->destroy = pk11_Null;
	break;
    default:
	rv = CKR_MECHANISM_INVALID;
	break;
    }

    if (rv != CKR_OK) {
        PORT_Free(context);
	pk11_FreeSession(session);
	pk11_FreeObject(key);
	return rv;
    }
    session->context = context;
    return CKR_OK;
}


/* C_VerifyRecover verifies a signature in a single-part operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
CK_RV C_VerifyRecover(CK_SESSION_HANDLE hSession,
		 CK_BYTE_PTR pSignature,CK_ULONG ulSignatureLen,
    				CK_BYTE_PTR pData,CK_ULONG_PTR pulDataLen) {
    PK11Session *session;
    PK11SessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulDataLen;
    CK_RV crv;
    SECStatus rv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession,&context,PK11_VERIFY_RECOVER,
							PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    rv = (*context->update)(context->cipherInfo, pData, &outlen, maxoutlen, 
						pSignature, ulSignatureLen);
    *pulDataLen = (CK_ULONG) outlen;
    pk11_FreeContext(context);
    session->context = NULL;
    pk11_FreeSession(session);
    return (rv == SECSuccess)  ? CKR_OK : CKR_DEVICE_ERROR;
}

/*
 **************************** Key Functions:  ************************
 */

#define MAX_KEY_LEN 256
/* C_GenerateKey generates a secret key, creating a new key object. */
CK_RV C_GenerateKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount,
    						CK_OBJECT_HANDLE_PTR phKey) {
    PK11Object *key;
    PK11Session *session;
    PRBool checkWeak = PR_FALSE;
    int key_length = 0;
    CK_KEY_TYPE key_type;
    CK_OBJECT_CLASS class = CKO_SECRET_KEY;
    CK_RV rv = CKR_OK;
    int i;
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    char buf[MAX_KEY_LEN];
    CK_MECHANISM_TYPE key_gen_type;
    SECOidTag algtag = SEC_OID_UNKNOWN;

    /*
     * now lets create an object to hang the attributes off of
     */
    key = pk11_NewObject(slot); /* fill in the handle later */
    if (key == NULL) {
	return CKR_HOST_MEMORY;
    }

    /*
     * load the template values into the object
     */
    for (i=0; i < ulCount; i++) {
	if (pTemplate[i].type == CKA_VALUE_LEN) {
	    key_length = *(CK_ULONG *)pTemplate[i].pValue;
	    continue;
	}

	rv = pk11_AddAttributeType(key,pk11_attr_expand(&pTemplate[i]));
	if (rv != CKR_OK) break;
    }
    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	return rv;
    }

    /* make sure we don't have any class, key_type, or value fields */
    pk11_DeleteAttributeType(key,CKA_CLASS);
    pk11_DeleteAttributeType(key,CKA_KEY_TYPE);
    pk11_DeleteAttributeType(key,CKA_VALUE);

    /* Now Set up the parameters to generate the key (based on mechanism) */
    key_gen_type = pMechanism->mechanism;
    switch (pMechanism->mechanism) {
    case CKM_RC2_KEY_GEN:
	key_type = CKK_RC2;
	if (key_length == 0) rv = CKR_TEMPLATE_INCOMPLETE;
	break;
    case CKM_RC4_KEY_GEN:
	key_type = CKK_RC4;
	if (key_length == 0) rv = CKR_TEMPLATE_INCOMPLETE;
	break;
    case CKM_DES_KEY_GEN:
	key_type = CKK_DES;
	key_length = 8;
	checkWeak = PR_TRUE;
	break;
    case CKM_DES2_KEY_GEN:
	key_type = CKK_DES2;
	key_length = 16;
	checkWeak = PR_TRUE;
	break;
    case CKM_DES3_KEY_GEN:
	key_type = CKK_DES3;
	key_length = 24;
	checkWeak = PR_TRUE;
	break;
    case CKM_PBE_MD2_DES_CBC:
	algtag = SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC;
	key_type = CKK_DES;
	key_length = 8;
	goto have_key_type;
    case CKM_PBE_MD5_DES_CBC:
	algtag = SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC;
	key_type = CKK_DES;
	key_length = 8;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
	algtag = SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC;
	key_type = CKK_DES;
	key_length = 8;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC;
	key_length = 24;
	key_type = CKK_DES3;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC;
	key_length = 5;
	key_type = CKK_RC2;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC;
	key_length = 16;
	key_type = CKK_RC2;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4;
	key_length = 5;
	key_type = CKK_RC4;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4;
	key_type = CKK_RC4;
	key_length = 16;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_RC2_CBC:
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC2_CBC;
	key_type = CKK_RC2;
	key_length = 
	    ((CK_NETSCAPE_PBE_PARAMS *)pMechanism->pParameter)->ulEffectiveBits;
	goto have_key_type;
    case CKM_NETSCAPE_PBE_SHA1_RC4: 
	algtag = SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC4;
	key_type = CKK_RC4;
	key_length = 
	    ((CK_NETSCAPE_PBE_PARAMS *)pMechanism->pParameter)->ulEffectiveBits;
	checkWeak = PR_FALSE;
have_key_type:
	key_gen_type = CKM_NETSCAPE_PBE_KEY_GEN;
	break;
    default:
	rv = CKR_MECHANISM_INVALID;
    }
    /* make sure we aren't going to overflow the buffer */
    if (sizeof(buf) < key_length) {
	/* someone is getting pretty optimistic about how big their key can
	 * be... */
        rv = CKR_TEMPLATE_INCONSISTENT;
    }

    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	return rv;
    }

    if(key_gen_type == CKM_NETSCAPE_PBE_KEY_GEN) {
	SECAlgorithmID algid;
	SECItem *pbe_key, mech;
	CK_PBE_PARAMS *pbe_params;
	SECStatus pbe_rv;

	mech.data = (unsigned char *)pMechanism->pParameter;
	mech.len = (unsigned int)pMechanism->ulParameterLen;
	pbe_rv = PK11_ParamToAlgid(algtag, &mech, NULL, &algid);
	if(pbe_rv == SECSuccess) {
	    pbe_params = (CK_PBE_PARAMS *)pMechanism->pParameter;
	    mech.data = (unsigned char *)pbe_params->pPassword;
	    mech.len = (unsigned int)pbe_params->ulPasswordLen;
	    pbe_key = SEC_PKCS5GetKey(&algid, &mech);
	    if(pbe_key != NULL) {
		PORT_Memcpy(buf, pbe_key->data, pbe_key->len);
		key_length = pbe_key->len;
		SECITEM_ZfreeItem(pbe_key, PR_TRUE);

		if(pbe_params->pInitVector == NULL) {
		    pbe_key = SEC_PKCS5GetIV(&algid, &mech);
		    if(pbe_key != NULL) {
			pbe_params->pInitVector = (CK_CHAR_PTR)PORT_ZAlloc(
				pbe_key->len);
			if(pbe_params->pInitVector != NULL) {
			    PORT_Memcpy(pbe_params->pInitVector, pbe_key->data,
				pbe_key->len);
			} else {
			    pbe_rv = SECFailure;
			}
			SECITEM_ZfreeItem(pbe_key, PR_TRUE);
		    } else {
			pbe_rv = SECFailure;
		    }
		}
	    } else {
		pbe_rv = SECFailure;
	    }
	    SECOID_DestroyAlgorithmID(&algid, PR_FALSE);
	}

	if(pbe_rv == SECFailure) {
	    rv = CKR_MECHANISM_INVALID;
	}
    } else {
	/* get the key, check for weak keys and repeat if found */
	do {
	    RNG_GenerateGlobalRandomBytes(buf,key_length);
	} while (checkWeak && pk11_IsWeakKey((unsigned char *)buf,key_type));
    }

    /* Add the class, key_type, and value */
    rv = pk11_AddAttributeType(key,CKA_CLASS,&class,sizeof(CK_OBJECT_CLASS));
    if (rv != CKR_OK) { pk11_FreeObject(key); return rv; }
    rv = pk11_AddAttributeType(key,CKA_KEY_TYPE,&key_type,sizeof(CK_KEY_TYPE));
    if (rv != CKR_OK) { pk11_FreeObject(key); return rv; }
    rv = pk11_AddAttributeType(key,CKA_CLASS,&class,sizeof(CK_OBJECT_CLASS));
    if (rv != CKR_OK) { pk11_FreeObject(key); return rv; }
    rv = pk11_AddAttributeType(key,CKA_VALUE,buf,key_length);
    if (rv != CKR_OK) { pk11_FreeObject(key); return rv; }

    /* get the session */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
	pk11_FreeObject(key);
        return CKR_SESSION_HANDLE_INVALID;
    }

    /*
     * handle the base object stuff
     */
    rv = pk11_handleObject(key,session);
    pk11_FreeSession(session);
    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	return rv;
    }

    *phKey = key->handle;
    return CKR_OK;
}


/* C_GenerateKeyPair generates a public-key/private-key pair, 
 * creating new key objects. */
CK_RV C_GenerateKeyPair (CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG ulPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG ulPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPrivateKey,
    					CK_OBJECT_HANDLE_PTR phPublicKey) {
    PK11Object *publicKey,*privateKey;
    PK11Session *session;
    CK_KEY_TYPE key_type;
    CK_RV crv = CKR_OK;
    SECStatus rv;
    CK_OBJECT_CLASS pubClass = CKO_PUBLIC_KEY;
    CK_OBJECT_CLASS privClass = CKO_PRIVATE_KEY;
    int i;
    /* Common */
    SECKEYLowPrivateKey *lowPriv;
    /* RSA */
    int public_modulus_bits = 0;
    SECItem pubExp;
    /* DSA */
    SECKEYLowPublicKey *lowPub;
    PQGParams param;
    DSAKeyGenContext *dsacontext;
    unsigned char randomBlock[20];
    /* Diffie Hellman */
    int private_value_bits = 0;
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);
    RNGContext *rng;

    /*
     * now lets create an object to hang the attributes off of
     */
    publicKey = pk11_NewObject(slot); /* fill in the handle later */
    if (publicKey == NULL) {
	return CKR_HOST_MEMORY;
    }

    /*
     * load the template values into the publicKey
     */
    for (i=0; i < ulPublicKeyAttributeCount; i++) {
	if (pPublicKeyTemplate[i].type == CKA_MODULUS_BITS) {
	    public_modulus_bits = *(CK_ULONG *)pPublicKeyTemplate[i].pValue;
	    continue;
	}

	crv = pk11_AddAttributeType(publicKey,
				    pk11_attr_expand(&pPublicKeyTemplate[i]));
	if (crv != CKR_OK) break;
    }

    if (crv != CKR_OK) {
	pk11_FreeObject(publicKey);
	return CKR_HOST_MEMORY;
    }

    privateKey = pk11_NewObject(slot); /* fill in the handle later */
    if (publicKey == NULL) {
	pk11_FreeObject(publicKey);
	return CKR_HOST_MEMORY;
    }
    /*
     * now load the private key template
     */
    for (i=0; i < ulPrivateKeyAttributeCount; i++) {
	if (pPrivateKeyTemplate[i].type == CKA_VALUE_BITS) {
	    private_value_bits = *(CK_ULONG *)pPrivateKeyTemplate[i].pValue;
	    continue;
	}

	crv = pk11_AddAttributeType(privateKey,
				    pk11_attr_expand(&pPrivateKeyTemplate[i]));
	if (crv != CKR_OK) break;
    }

    if (crv != CKR_OK) {
	pk11_FreeObject(publicKey);
	pk11_FreeObject(privateKey);
	return CKR_HOST_MEMORY;
    }
    pk11_DeleteAttributeType(privateKey,CKA_CLASS);
    pk11_DeleteAttributeType(privateKey,CKA_KEY_TYPE);
    pk11_DeleteAttributeType(privateKey,CKA_VALUE);
    pk11_DeleteAttributeType(publicKey,CKA_CLASS);
    pk11_DeleteAttributeType(publicKey,CKA_KEY_TYPE);
    pk11_DeleteAttributeType(publicKey,CKA_VALUE);

    /* Now Set up the parameters to generate the key (based on mechanism) */
    switch (pMechanism->mechanism) {
    case CKM_RSA_PKCS_KEY_PAIR_GEN:
	/* format the keys */
    	pk11_DeleteAttributeType(publicKey,CKA_MODULUS);
    	pk11_DeleteAttributeType(privateKey,CKA_ID);
    	pk11_DeleteAttributeType(privateKey,CKA_MODULUS);
    	pk11_DeleteAttributeType(privateKey,CKA_PRIVATE_EXPONENT);
    	pk11_DeleteAttributeType(privateKey,CKA_PUBLIC_EXPONENT);
    	pk11_DeleteAttributeType(privateKey,CKA_PRIME_1);
    	pk11_DeleteAttributeType(privateKey,CKA_PRIME_2);
    	pk11_DeleteAttributeType(privateKey,CKA_EXPONENT_1);
    	pk11_DeleteAttributeType(privateKey,CKA_EXPONENT_2);
    	pk11_DeleteAttributeType(privateKey,CKA_COEFFICIENT);
	key_type = CKK_RSA;
	if (public_modulus_bits == 0) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    break;
	}

	/* extract the exponent */
	crv=pk11_Attribute2SecItem(NULL,&pubExp,publicKey,CKA_PUBLIC_EXPONENT);
	if (crv != CKR_OK) break;
        crv = pk11_AddAttributeType(privateKey,CKA_PUBLIC_EXPONENT,
				    		    pk11_item_expand(&pubExp));
	if (crv != CKR_OK) {
	    PORT_Free(pubExp.data);
	    break;
	}

	rng = RNG_CreateContext();
	if (rng == NULL) {
	    PORT_Free(pubExp.data);
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	lowPriv = RSA_NewKey(rng, public_modulus_bits, &pubExp);
	PORT_Free(pubExp.data);
	if (lowPriv == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
        /* now fill in the RSA dependent paramenters in the public key */
        crv = pk11_AddAttributeType(publicKey,CKA_MODULUS,
			   pk11_item_expand(&lowPriv->u.rsa.modulus));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        /* now fill in the RSA dependent paramenters in the private key */
        crv = pk11_AddAttributeType(privateKey,CKA_ID,
			   pk11_item_expand(&lowPriv->u.rsa.modulus));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_MODULUS,
			   pk11_item_expand(&lowPriv->u.rsa.modulus));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_PRIVATE_EXPONENT,
			   pk11_item_expand(&lowPriv->u.rsa.privateExponent));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_PRIME_1,
			   pk11_item_expand(&lowPriv->u.rsa.prime[0]));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_PRIME_2,
			   pk11_item_expand(&lowPriv->u.rsa.prime[1]));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_EXPONENT_1,
			   pk11_item_expand(&lowPriv->u.rsa.primeExponent[0]));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_EXPONENT_2,
			   pk11_item_expand(&lowPriv->u.rsa.primeExponent[1]));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        crv = pk11_AddAttributeType(privateKey,CKA_COEFFICIENT,
			   pk11_item_expand(&lowPriv->u.rsa.coefficient));
	SECKEY_LowDestroyPrivateKey(lowPriv);
	break;
    case CKM_DSA_KEY_PAIR_GEN:
    	pk11_DeleteAttributeType(publicKey,CKA_VALUE);
    	pk11_DeleteAttributeType(privateKey,CKA_ID);
	pk11_DeleteAttributeType(privateKey,CKA_PRIME);
	pk11_DeleteAttributeType(privateKey,CKA_SUBPRIME);
	pk11_DeleteAttributeType(privateKey,CKA_BASE);
	key_type = CKK_DSA;

	/* extract the necessary paramters and copy them to the private key */
	crv=pk11_Attribute2SecItem(NULL,&param.prime,publicKey,CKA_PRIME);
	if (crv != CKR_OK) break;
	crv=pk11_Attribute2SecItem(NULL,&param.subPrime,publicKey,CKA_SUBPRIME);
	if (crv != CKR_OK) {
	    PORT_Free(param.prime.data);
	    break;
	}
	crv=pk11_Attribute2SecItem(NULL,&param.base,publicKey,CKA_BASE);
	if (crv != CKR_OK) {
	    PORT_Free(param.prime.data);
	    PORT_Free(param.subPrime.data);
	    break;
	}
        crv = pk11_AddAttributeType(privateKey,CKA_PRIME,
				   	   pk11_item_expand(&param.prime));
	if (crv != CKR_OK) {
	    PORT_Free(param.prime.data);
	    PORT_Free(param.subPrime.data);
	    PORT_Free(param.base.data);
	    break;
	}
        crv = pk11_AddAttributeType(privateKey,CKA_SUBPRIME,
				    	   pk11_item_expand(&param.subPrime));
	if (crv != CKR_OK) {
	    PORT_Free(param.prime.data);
	    PORT_Free(param.subPrime.data);
	    PORT_Free(param.base.data);
	    break;
	}
        crv = pk11_AddAttributeType(privateKey,CKA_BASE,
				    	    pk11_item_expand(&param.base));
	if (crv != CKR_OK) {
	    PORT_Free(param.prime.data);
	    PORT_Free(param.subPrime.data);
	    PORT_Free(param.base.data);
	    break;
	}

	/* Generate the key */
	dsacontext = DSA_CreateKeyGenContext(&param);
	PORT_Free(param.prime.data);
	PORT_Free(param.subPrime.data);
	PORT_Free(param.base.data);
	if (dsacontext == NULL) { crv = CKR_HOST_MEMORY; break; }

	RNG_GenerateGlobalRandomBytes(randomBlock,sizeof(randomBlock));
	rv = DSA_KeyGen(dsacontext,&lowPub,&lowPriv,randomBlock);
	if (rv != SECSuccess) { crv = CKR_HOST_MEMORY; break; }

	DSA_DestroyKeyGenContext(dsacontext);

	/* store the generated key into the attributes */
        crv = pk11_AddAttributeType(publicKey,CKA_VALUE,
			   pk11_item_expand(&lowPub->u.dsa.publicValue));
	if (crv != CKR_OK) { SECKEY_LowDestroyPublicKey(lowPub);}
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
        /* now fill in the RSA dependent paramenters in the private key */
        crv = pk11_AddAttributeType(privateKey,CKA_ID,
			   pk11_item_expand(&lowPub->u.dsa.publicValue));
        crv = pk11_AddAttributeType(privateKey,CKA_VALUE,
			   pk11_item_expand(&lowPriv->u.dsa.privateValue));
	if (crv != CKR_OK) { SECKEY_LowDestroyPrivateKey(lowPriv); break;}
	break;
    case CKM_DH_PKCS_KEY_PAIR_GEN:
	pk11_DeleteAttributeType(privateKey,CKA_PRIME);
	pk11_DeleteAttributeType(privateKey,CKA_BASE);
	crv = CKR_MECHANISM_INVALID;
	break;
    default:
	crv = CKR_MECHANISM_INVALID;
    }

    if (crv != CKR_OK) {
	pk11_FreeObject(privateKey);
	pk11_FreeObject(publicKey);
	return crv;
    }


    /* Add the class, key_type The loop lets us check errors blow out
     *  on errors and clean up at the bottom */
    session = NULL; /* make pedtantic happy... session cannot leave the*/
		    /* loop below NULL unless an error is set... */
    do {
	crv = pk11_AddAttributeType(privateKey,CKA_CLASS,&privClass,
						sizeof(CK_OBJECT_CLASS));
        if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(publicKey,CKA_CLASS,&pubClass,
						sizeof(CK_OBJECT_CLASS));
        if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(privateKey,CKA_KEY_TYPE,&key_type,
						sizeof(CK_KEY_TYPE));
        if (crv != CKR_OK) break;
	crv = pk11_AddAttributeType(publicKey,CKA_KEY_TYPE,&key_type,
						sizeof(CK_KEY_TYPE));
        if (crv != CKR_OK) break;
        session = pk11_SessionFromHandle(hSession);
        if (session == NULL) crv = CKR_SESSION_HANDLE_INVALID;
    } while (0);

    if (crv != CKR_OK) {
	 pk11_FreeObject(privateKey);
	 pk11_FreeObject(publicKey);
	 return crv;
    }

    /*
     * handle the base object cleanup for the public Key
     */
    crv = pk11_handleObject(publicKey,session);
    if (crv != CKR_OK) {
        pk11_FreeSession(session);
	pk11_FreeObject(privateKey);
	pk11_FreeObject(publicKey);
	return crv;
    }

    /*
     * handle the base object cleanup for the private Key
     * If we have any problems, we destroy the public Key we've
     * created and linked.
     */
    crv = pk11_handleObject(privateKey,session);
    pk11_FreeSession(session);
    if (crv != CKR_OK) {
	pk11_FreeObject(privateKey);
	C_DestroyObject(hSession,publicKey->handle);
	return crv;
    }
    *phPrivateKey = privateKey->handle;
    *phPublicKey = publicKey->handle;

    return CKR_OK;
}

/* XXXXX it doesn't matter yet, since we colapse error conditions in the
 * level above, but we really should map those few key error differences */
CK_RV pk11_mapWrap(CK_RV rv) { return rv; }

/* C_WrapKey wraps (i.e., encrypts) a key. */
CK_RV C_WrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey,
					 CK_ULONG_PTR pulWrappedKeyLen) {
    PK11Session *session;
    PK11Attribute *attribute;
    PK11Object *key;
    CK_RV rv;

    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
    	return CKR_SESSION_HANDLE_INVALID;
    }

    key = pk11_ObjectFromHandle(hKey,session);
    pk11_FreeSession(session);
    if (key == NULL) {
	return CKR_KEY_HANDLE_INVALID;
    }

    /* XXX this code is used to extract keys from the database for PKCS #12
     * the keys are being pulled out unencrypted...which they should not be
     * in any other case.  eventually, they will be extracted encrypted via
     * pkcs 8/pkcs 5.
     */
    if(pMechanism->mechanism == CKM_NETSCAPE_NULL_ENCRYPT) {
	SECKEYLowPrivateKey *lk;
	/* XXX handle only RSA keys */
	*pulWrappedKeyLen = (CK_ULONG)sizeof(SECKEYLowPrivateKey);
	if(pWrappedKey != NULL) {
	    lk = pk11_GetPrivKey(key, CKK_RSA);
	    if(lk != NULL) {
		SECKEYLowPrivateKey *returnKey = SECKEY_CopyLowPrivateKey(lk);
		if(returnKey) {

		    /* XXX we are returning a pointer to a SECKEY_LowPrivateKey
		     * structure.  it will be dereferenced in
		     * PK11_ExportPrivateKeyInfo.
		     */
		    *((SECKEYLowPrivateKey **)pWrappedKey) = returnKey;
		    *pulWrappedKeyLen = (CK_ULONG)sizeof(SECKEYLowPrivateKey *);
		} else {
		    pWrappedKey = NULL;
		    *pulWrappedKeyLen = 0;
		}
		if(lk != key->objectInfo) {
		    pk11_FreePrivKey(lk, PR_TRUE);
		}
	    } else {
		*pulWrappedKeyLen = 0;
	    }
	} 
	pk11_FreeObject(key);
	if(pWrappedKey == NULL) {
	    return CKR_KEY_TYPE_INCONSISTENT;
	} else {
	    return CKR_OK;
	}
    }

    attribute = pk11_FindAttribute(key,CKA_VALUE);
    if (key->class != CKO_SECRET_KEY) {
	pk11_FreeObject(key);
	return CKR_KEY_TYPE_INCONSISTENT;
    }

    pk11_FreeObject(key);
    if (attribute == NULL) {
	return CKR_KEY_TYPE_INCONSISTENT;
    }
     
    rv = pk11_EncryptInit(hSession, pMechanism, hWrappingKey, CKA_WRAP);
    if (rv != CKR_OK) {
	pk11_FreeAttribute(attribute);
	return pk11_mapWrap(rv);
    }
    rv = C_Encrypt(hSession, attribute->attrib.pValue, 
		attribute->attrib.ulValueLen,pWrappedKey,pulWrappedKeyLen);
    pk11_FreeAttribute(attribute);
    return pk11_mapWrap(rv);
}


/* C_UnwrapKey unwraps (decrypts) a wrapped key, creating a new key object. */
CK_RV C_UnwrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey, CK_ULONG ulWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
						 CK_OBJECT_HANDLE_PTR phKey) {
    PK11Object *key;
    PK11Session *session;
    int key_length = 0;
    unsigned char * buf;
    CK_RV rv = CKR_OK;
    int i;
    CK_ULONG bsize;
    PK11Slot *slot = pk11_SlotFromSessionHandle(hSession);


    /*
     * now lets create an object to hang the attributes off of
     */
    key = pk11_NewObject(slot); /* fill in the handle later */
    if (key == NULL) {
	return CKR_HOST_MEMORY;
    }

    /*
     * load the template values into the object
     */
    for (i=0; i < ulAttributeCount; i++) {
	if (pTemplate[i].type == CKA_VALUE_LEN) {
	    key_length = *(CK_ULONG *)pTemplate[i].pValue;
	    continue;
	}

	rv = pk11_AddAttributeType(key,pk11_attr_expand(&pTemplate[i]));
	if (rv != CKR_OK) break;
    }
    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	return rv;
    }

    /* make sure we don't have any class, key_type, or value fields */
    if (!pk11_hasAttribute(key,CKA_CLASS)) {
	pk11_FreeObject(key);
	return CKR_TEMPLATE_INCOMPLETE;
    }
    if (!pk11_hasAttribute(key,CKA_KEY_TYPE)) {
	pk11_FreeObject(key);
	return CKR_TEMPLATE_INCOMPLETE;
    }

    rv = pk11_DecryptInit(hSession, pMechanism, hUnwrappingKey, CKA_UNWRAP);
    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	return pk11_mapWrap(rv);
    }

    /* allocate the buffer to decrypt into 
     * this assumes the unwrapped key is never larger than the
     * wrapped key. For all the mechanisms we support this is true */
    buf = (unsigned char *)PORT_Alloc( ulWrappedKeyLen);


    rv = C_Decrypt(hSession, pWrappedKey, ulWrappedKeyLen, buf, &bsize);
    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	PORT_Free(buf);
	return pk11_mapWrap(rv);
    }

    if (key_length == 0) {
	key_length = bsize;
    }
    if (key_length > MAX_KEY_LEN) {
	pk11_FreeObject(key);
	PORT_Free(buf);
	return CKR_TEMPLATE_INCONSISTENT;
    }
    
    /* Add the class, key_type, and value */
    rv = pk11_AddAttributeType(key,CKA_VALUE,buf,key_length);
    PORT_Free(buf);
    if (rv != CKR_OK) { pk11_FreeObject(key); return rv; }

    /* get the session */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) {
	pk11_FreeObject(key);
        return CKR_SESSION_HANDLE_INVALID;
    }

    /*
     * handle the base object stuff
     */
    rv = pk11_handleObject(key,session);
    pk11_FreeSession(session);
    if (rv != CKR_OK) {
	pk11_FreeObject(key);
	return rv;
    }

    *phKey = key->handle;
    return CKR_OK;

}


/* C_DeriveKey derives a key from a base key, creating a new key object. */
CK_RV C_DeriveKey( CK_SESSION_HANDLE hSession,
	 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey,
	 CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount, 
						CK_OBJECT_HANDLE_PTR phKey) {
    return CKR_MECHANISM_INVALID;
}

/*
 **************************** Radom Functions:  ************************
 */

/* C_SeedRandom mixes additional seed material into the token's random number 
 * generator. */
CK_RV C_SeedRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed,
    CK_ULONG ulSeedLen) {

    RNG_RandomUpdate(pSeed, ulSeedLen);
    return CKR_OK;
}


/* C_GenerateRandom generates random data. */
CK_RV C_GenerateRandom(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR	pRandomData, CK_ULONG ulRandomLen) {

    RNG_GenerateGlobalRandomBytes(pRandomData, ulRandomLen);
    return CKR_OK;
}


/* C_GetFunctionStatus obtains an updated status of a function running 
 * in parallel with an application. */
CK_RV C_GetFunctionStatus(CK_SESSION_HANDLE hSession) {
    return CKR_FUNCTION_NOT_PARALLEL;
}


/* C_CancelFunction cancels a function running in parallel */
CK_RV C_CancelFunction(CK_SESSION_HANDLE hSession) {
    return CKR_FUNCTION_NOT_PARALLEL;
}

/* C_GetOperationState saves the state of the cryptographic 
 *operation in a session. */
CK_RV C_GetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG_PTR pulOperationStateLen) {
    PK11SessionContext *context;
    PK11Session *session;
    CK_RV crv;

    /* make sure we're legal */
    crv = pk11_GetContext(hSession, &context, PK11_HASH, PR_TRUE, &session);
    if (crv != CKR_OK) return crv;

    *pulOperationStateLen = context->cipherInfoLen + sizeof(CK_MECHANISM_TYPE)
				+ sizeof(PK11ContextType);
    if (pOperationState == NULL) {
        pk11_FreeSession(session);
	return CKR_OK;
    }
    PORT_Memcpy(pOperationState,&context->type,sizeof(PK11ContextType));
    pOperationState += sizeof(PK11ContextType);
    PORT_Memcpy(pOperationState,&context->currentMech,
						sizeof(CK_MECHANISM_TYPE));
    pOperationState += sizeof(CK_MECHANISM_TYPE);
    PORT_Memcpy(pOperationState,context->cipherInfo,context->cipherInfoLen);
    pk11_FreeSession(session);
    return CKR_OK;
}



/* C_SetOperationState restores the state of the cryptographic operation in a session. */
CK_RV C_SetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG  ulOperationStateLen,
        CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey) {
    PK11SessionContext *context;
    PK11Session *session;
    PK11ContextType type;
    CK_MECHANISM mech;
    CK_RV crv = CKR_OK;

    /* make sure we're legal */
    session = pk11_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    context = session->context;
    session->context = NULL;
    if (context) { 
	pk11_FreeContext(context);
    }
    pk11_FreeSession(session);
    PORT_Memcpy(&type,pOperationState, sizeof(PK11ContextType));
    pOperationState += sizeof(PK11ContextType);
    PORT_Memcpy(&mech.mechanism,pOperationState,sizeof(CK_MECHANISM_TYPE));
    pOperationState += sizeof(CK_MECHANISM_TYPE);
    mech.pParameter = NULL;
    mech.ulParameterLen = 0;
    switch (type) {
    case PK11_HASH:
	crv = C_DigestInit(hSession,&mech);
	if (crv != CKR_OK) break;
	crv = pk11_GetContext(hSession, &context, PK11_HASH, PR_TRUE, &session);
	if (crv != CKR_OK) break;
	PORT_Memcpy(context->cipherInfo,pOperationState,context->cipherInfoLen);
        pk11_FreeSession(session);
	break;
    default:
	/* sigh, do sign/encrypt/decrypt later */
	crv = CKR_SAVED_STATE_INVALID;
    }
   return crv;
}

/* Dual-function cryptographic operations */

/* C_DigestEncryptUpdate continues a multiple-part digesting and encryption operation. */
CK_RV C_DigestEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
    CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen){
    return CKR_FUNCTION_NOT_SUPPORTED;
}


/* C_DecryptDigestUpdate continues a multiple-part decryption and digesting operation. */
CK_RV C_DecryptDigestUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR  pEncryptedPart, CK_ULONG  ulEncryptedPartLen,
    				CK_BYTE_PTR  pPart, CK_ULONG_PTR pulPartLen){
    return CKR_FUNCTION_NOT_SUPPORTED;
}


/* C_SignEncryptUpdate continues a multiple-part signing and encryption operation. */
CK_RV C_SignEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
	 CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen){
    return CKR_FUNCTION_NOT_SUPPORTED;
}


/* C_DecryptVerifyUpdate continues a multiple-part decryption and verify operation. */
CK_RV C_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pEncryptedData, CK_ULONG  ulEncryptedDataLen, 
				CK_BYTE_PTR  pData, CK_ULONG_PTR pulDataLen){
    return CKR_FUNCTION_NOT_SUPPORTED;
}

/* C_DigestKey continues a multi-part message-digesting operation,
 * by digesting the value of a secret key as part of the data already digested.
 */
CK_RV C_DigestKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey) {
    return CKR_FUNCTION_NOT_SUPPORTED;
}

