/*
 * Security Policy Enforcement for crypto code
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: policy.c,v 1.4.2.6 1997/05/24 00:23:40 jwz Exp $
 */

#include "xp.h"
#include "secport.h"
#include "secnav.h"
#include "sslproto.h"
#include "cert.h"	/* for CertCertificate */
#include "prefapi.h"
#include "zig.h"
#include "xpgetstr.h"
#include "gui.h"	/* for XP_AppCodeName */
#include "fe_proto.h"	/* for FE_Alert() */
#include "proto.h"	/* for XP_FindSomeContext() */

#ifdef AKBAR
extern char *XP_AppLanguage;
extern char * XP_PlatformFileToURL (const char *name);
#endif /* AKBAR */

#define NOT_FOR_RELEASE_TO_THE_PUBLIC 0

#define PR_MAYBE ((PRBool)SSL_RESTRICTED)

#define ACQUIRE_WRITE_LOCK
#define RELEASE_WRITE_LOCK
#define ACQUIRE_READ_LOCK
#define RELEASE_READ_LOCK

/* pointers to global string indexes */
/* there oughtta be a better way than this... */
extern int	XP_SMIME_DES_CBC;
extern int	XP_SMIME_DES_EDE3;
extern int	XP_SMIME_RC2_CBC_128;
extern int	XP_SMIME_RC2_CBC_40;
extern int	XP_SMIME_RC2_CBC_64;

#ifdef DOING_RC5PAD
extern int	XP_SMIME_RC5PAD_64_16_128;
extern int	XP_SMIME_RC5PAD_64_16_40;
extern int	XP_SMIME_RC5PAD_64_16_64;
#endif

extern int	XP_SSL2_DES_192_EDE3;	
extern int	XP_SSL2_DES_64;	
extern int	XP_SSL2_RC2_128;	
extern int	XP_SSL2_RC2_40;	
extern int	XP_SSL2_RC4_128;	
extern int	XP_SSL2_RC4_40;	

extern int	XP_SSL3_RSA_RC4_128_MD5;
extern int	XP_SSL3_RSA_3DES_SHA;	
extern int	XP_SSL3_RSA_DES_SHA;	
extern int	XP_SSL3_RSA_RC4_40_MD5;	
extern int	XP_SSL3_RSA_RC2_40_MD5;	
extern int	XP_SSL3_RSA_NULL_MD5;

#ifdef FORTEZZA
extern int	XP_SSL3_FORTEZZA_SHA;	
extern int	XP_SSL3_FORTEZZA_RC4_SHA;
extern int	XP_SSL3_FORTEZZA_NULL_SHA;	
#endif

extern int 	XP_CONFIG_NO_POLICY_STRINGS;


#ifndef PORT_Atol
#define PORT_Atol atol
#endif

typedef enum {
    undefinedType = 0,		/* make 0 invalid */
    boolType,
    longType,
    stringType,
    numValueTypes		/* this many valueTypes */
} valueType;

typedef struct _algdesc {
    char *	policyDesc;	/* name to identify this in policy module */
    char *	sectionName;	/* section of the policy file */
    char *	prefName;	/* associated preference name */
    int *	descNum;	/* string ID, for looking up in pref UI */
    PRBool 	allowed;	/* does the policy module allow this */
    PRBool 	preferred;	/* is this alg preference enabled ? */
    char *	stringValue;	/* value for string type items */
    valueType 	type;		/* what type of value is this item */
    long	longValue;	/* value for "long" type items */
    long 	protocolID;	/* protocol specific identifier */
} policyAlgorithmDesc;

/*
 * Well-defined indices for certain entries in table below 
 */
#define ALGNDX_SW_VERSION	0
#define ALGNDX_COUNTRIES	1
#define ALGNDX_EXCLUDED		2
#define ALGNDX_KEYGEN_BITS	3

/* Global Policy Version pointer (yuck).  
 * Application can override default value before calling SECNAV_PolicyInit().
 */
const char * XP_PolicyVersion = "P2"; /* "P2" means second "official" policy */

static PRBool policyFileIsOK;

/*
 * This is the policy table of possible algorithms for Dogbert
 * Keep it sorted in decreasing ordfer of preference, within family.
 */
policyAlgorithmDesc algTable[] = {
    /*
     * General items
     */
    {   /* which version of mozilla is the file intended for */
	"Software-Version", 			"General",
	NULL, 					0, 	
	PR_FALSE, 	PR_FALSE, 	NULL, 	stringType,	0L,
	-1L
    },
    {
	"Countries", 				"General",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	stringType,	0L,
	-1L
    },
    {
	"Excluded-Countries", 			"General",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	stringType,	0L,
	-1L
    },
    {
	"MAX-GEN-KEY-BITS", 			"KEYGEN",
	"security.keygen.maxbits", 		0,	
	PR_FALSE,	PR_FALSE,	NULL,	longType,	512L,
	-1L
    },

    /*
     * PKCS12 algorithms and key lengths, in order of decreasing preference.
     */
    {
	"PKCS12-DES-EDE3", 			"PKCS12",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	PKCS12_DES_EDE3_168
    },
    {
	"PKCS12-DES-56", 			"PKCS12",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	PKCS12_DES_56
    },
    {
	"PKCS12-RC2-128", 			"PKCS12",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	PKCS12_RC2_CBC_128
    },
    {
	"PKCS12-RC4-128", 			"PKCS12",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	PKCS12_RC4_128
    },
    {
	"PKCS12-RC2-40", 			"PKCS12",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	PKCS12_RC2_CBC_40
    },
    {
	"PKCS12-RC4-40", 			"PKCS12",
	NULL, 					0,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	PKCS12_RC4_40
    },

    /*
     * SSL 2 algorithms
     *	in order of decreasing preference
     */
    {
	"SSL2-RC4-128-WITH-MD5", 		CIPHER_FAMILY_SSL2,
	"security.ssl2.rc4_128", 		&XP_SSL2_RC4_128,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_EN_RC4_128_WITH_MD5
    },
    {
	"SSL2-RC2-128-CBC-WITH-MD5", 		CIPHER_FAMILY_SSL2,
	"security.ssl2.rc2_128", 		&XP_SSL2_RC2_128,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_EN_RC2_128_CBC_WITH_MD5
    },
    {
	"SSL2-DES-168-EDE3-CBC-WITH-MD5", 	CIPHER_FAMILY_SSL2,
	"security.ssl2.des_ede3_192", 		&XP_SSL2_DES_192_EDE3,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_EN_DES_192_EDE3_CBC_WITH_MD5
    },
    {
	"SSL2-DES-56-CBC-WITH-MD5", 		CIPHER_FAMILY_SSL2,
	"security.ssl2.des_64", 		&XP_SSL2_DES_64,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_EN_DES_64_CBC_WITH_MD5
    },
    {
	"SSL2-RC4-128-EXPORT40-WITH-MD5", 	CIPHER_FAMILY_SSL2,
	"security.ssl2.rc4_40", 		&XP_SSL2_RC4_40,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_EN_RC4_128_EXPORT40_WITH_MD5
    },
    {
	"SSL2-RC2-128-CBC-EXPORT40-WITH-MD5", 	CIPHER_FAMILY_SSL2,
	"security.ssl2.rc2_40", 		&XP_SSL2_RC2_40,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5
    },

    /*
     * SSL 3 algorithms
     *	in order of decreasing preference
     */
#ifdef FORTEZZA
    {
	"SSL3-FORTEZZA-DMS-WITH-FORTEZZA-CBC-SHA", CIPHER_FAMILY_SSL3,
	"security.ssl3.fortezza_fortezza_sha", 	&XP_SSL3_FORTEZZA_SHA,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA
    },
    {
	"SSL3-FORTEZZA-DMS-WITH-RC4-128-SHA",	CIPHER_FAMILY_SSL3,
	"security.ssl3.fortezza_rc4_sha", 	&XP_SSL3_FORTEZZA_RC4_SHA,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_FORTEZZA_DMS_WITH_RC4_128_SHA
    },
#endif
    {
	"SSL3-RSA-WITH-RC4-128-MD5", 		CIPHER_FAMILY_SSL3,
	"security.ssl3.rsa_rc4_128_md5", 	&XP_SSL3_RSA_RC4_128_MD5,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_RSA_WITH_RC4_128_MD5
    },
    {
	"SSL3-RSA-WITH-3DES-EDE-CBC-SHA", 	CIPHER_FAMILY_SSL3,
	"security.ssl3.rsa_des_ede3_sha", 	&XP_SSL3_RSA_3DES_SHA,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_RSA_WITH_3DES_EDE_CBC_SHA
    },
    {
	"SSL3-RSA-WITH-DES-CBC-SHA", 		CIPHER_FAMILY_SSL3,
	"security.ssl3.rsa_des_sha", 		&XP_SSL3_RSA_DES_SHA,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_RSA_WITH_DES_CBC_SHA
    },
    {
	"SSL3-RSA-WITH-RC4-40-MD5", 		CIPHER_FAMILY_SSL3,
	"security.ssl3.rsa_rc4_40_md5", 	&XP_SSL3_RSA_RC4_40_MD5,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_RSA_EXPORT_WITH_RC4_40_MD5
    },
    {
	"SSL3-RSA-WITH-RC2-CBC-40-MD5", 	CIPHER_FAMILY_SSL3,
	"security.ssl3.rsa_rc2_40_md5", 	&XP_SSL3_RSA_RC2_40_MD5,	
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5
    },
#ifdef FORTEZZA
    {
	"SSL3-FORTEZZA-DMS-WITH-NULL-SHA", 	CIPHER_FAMILY_SSL3,
	"security.ssl3.fortezza_null_sha", 	&XP_SSL3_FORTEZZA_NULL_SHA,	
	PR_TRUE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_FORTEZZA_DMS_WITH_NULL_SHA
    },
#endif
    {
	"SSL3-RSA-WITH-NULL-MD5", 		CIPHER_FAMILY_SSL3,
	"security.ssl3.rsa_null_md5", 		&XP_SSL3_RSA_NULL_MD5,	
	PR_TRUE,	PR_FALSE,	NULL,	boolType,	0L,
	SSL_RSA_WITH_NULL_MD5
    },

    /*
     * SMIME bulk-cipher suites
     *	in order of decreasing preference
     */
    {
	"SMIME-DES-EDE3", 			CIPHER_FAMILY_SMIME,
	"security.smime.des_ede3", 		&XP_SMIME_DES_EDE3,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_DES_EDE3_168
    },
    {
	"SMIME-RC2-CBC-128", 			CIPHER_FAMILY_SMIME,
	"security.smime.rc2_128", 		&XP_SMIME_RC2_CBC_128,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_RC2_CBC_128
    },
#ifdef DOING_RC5PAD
    {
	"SMIME-RC5PAD-64-16-128", 		CIPHER_FAMILY_SMIME,
	"security.smime.rc5.b64r16k128", 	&XP_SMIME_RC5PAD_64_16_128,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_RC5PAD_64_16_128
    },
#endif
    {
	"SMIME-DES-CBC", 			CIPHER_FAMILY_SMIME,
	"security.smime.des", 			&XP_SMIME_DES_CBC,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_DES_CBC_56
    },
    {
	"SMIME-RC2-CBC-64", 			CIPHER_FAMILY_SMIME,
	"security.smime.rc2_64", 		&XP_SMIME_RC2_CBC_64,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_RC2_CBC_64
    },
#ifdef DOING_RC5PAD
    {
	"SMIME-RC5PAD-64-16-64", 		CIPHER_FAMILY_SMIME,
	"security.smime.rc5.b64r16k64", 	&XP_SMIME_RC5PAD_64_16_64,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_RC5PAD_64_16_64
    },
#endif
    {
	"SMIME-RC2-CBC-40", 			CIPHER_FAMILY_SMIME,
	"security.smime.rc2_40", 		&XP_SMIME_RC2_CBC_40,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_RC2_CBC_40
    }
#ifdef DOING_RC5PAD
   ,{
	"SMIME-RC5PAD-64-16-40", 		CIPHER_FAMILY_SMIME,
	"security.smime.rc5.b64r16k40", 	&XP_SMIME_RC5PAD_64_16_40,
	PR_FALSE,	PR_FALSE,	NULL,	boolType,	0L,
	SMIME_RC5PAD_64_16_40
    }
#endif
};

static const int tableSize  = sizeof(algTable) / sizeof(policyAlgorithmDesc);


static unsigned char policyCaCertDERPubKey[] = {

    0x30, 0x81, 0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 
    0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 
    0x89, 0x02, 0x81, 0x81, 0x00, 0xbc, 0x27, 0xb9, 
    0x4c, 0x56, 0x70, 0xec, 0xe7, 0xe9, 0x4d, 0x4d, 
    0x8a, 0x86, 0x9f, 0xf0, 0xa8, 0x3f, 0x12, 0xdb, 
    0x95, 0x1a, 0x20, 0x4e, 0xed, 0xcb, 0x14, 0x00, 
    0xe6, 0x45, 0xb1, 0xc1, 0x34, 0xee, 0x98, 0x38, 
    0xe2, 0xb3, 0x8a, 0xa5, 0x49, 0xa8, 0xf5, 0x86, 
    0x86, 0x72, 0xc6, 0x72, 0xbb, 0x18, 0xdd, 0x5d, 
    0xaa, 0xbc, 0x21, 0x6c, 0x9d, 0x82, 0xdd, 0xec, 
    0xfe, 0x21, 0xac, 0x3d, 0xa1, 0xf5, 0x99, 0xe7, 
    0x3a, 0x9d, 0xfd, 0x94, 0xd4, 0xb7, 0xb4, 0xc7, 
    0xbb, 0x5c, 0x38, 0xeb, 0xaa, 0x08, 0xda, 0x7b, 
    0x1d, 0xfd, 0xfe, 0x51, 0xcb, 0x20, 0x3c, 0xcf, 
    0x89, 0x1c, 0x9e, 0x2f, 0x6d, 0x22, 0x16, 0x5b, 
    0x53, 0xda, 0x65, 0xee, 0x19, 0x37, 0xf6, 0x73, 
    0xe4, 0x6a, 0x7a, 0x46, 0x12, 0x29, 0xbb, 0xd7, 
    0x2f, 0x57, 0xd3, 0x6d, 0x05, 0x2e, 0x22, 0xfe, 
    0xbc, 0x42, 0xb0, 0x34, 0x7f, 0x02, 0x03, 0x01, 
    0x00, 0x01

};

/* read in preferences associated with policy items */
void
SECNAV_PrefsRefresh(void)
{
    policyAlgorithmDesc *desc = algTable;
    int i;

ACQUIRE_WRITE_LOCK

    /* read prefs for each boolean item in the table */
    for ( i = tableSize; i > 0; i-- ) {
	if ( desc->type == boolType && desc->allowed && desc->prefName ) {
	    int 	err;
	    XP_Bool 	prefval;

	    err = PREF_GetBoolPref(desc->prefName, &prefval);
	    PORT_Assert(0 <= err);
	    if (0 <= err)
		desc->preferred = ( prefval ? PR_TRUE : PR_FALSE );
	}
	++desc;
    }

RELEASE_WRITE_LOCK
}

/* Update preferences in local table, and in pref software table.
 * Caller should also call PREF_SavePrefFile() after updating the prefs.
 */
void
SECNAV_PrefBoolUpdate(long protocolID, PRBool prefval)
{
    policyAlgorithmDesc *desc = algTable;
    int i;
    int err = 0;

ACQUIRE_WRITE_LOCK

    /* read prefs for each boolean item in the table */
    for ( i = tableSize; i > 0; i-- ) {
	if ( desc->protocolID == protocolID && desc->type == boolType ) {
	    err = PREF_SetBoolPref(desc->prefName, prefval);
	    PORT_Assert(0 <= err);
	    if (0 <= err)
		desc->preferred = ( prefval ? PR_TRUE : PR_FALSE );
	    break;
	}
	++desc;
    }

RELEASE_WRITE_LOCK
    PORT_Assert(i > 0);
}


static void
parseMetaInfo( policyAlgorithmDesc *desc, char * value)
{
    if (!value)
    	return;
    switch ( desc->type ) {
    case stringType:
	/* if an old string exists, free it */
	if ( desc->stringValue ) {
	    PORT_Free(desc->stringValue);
	    desc->stringValue = NULL;
	}
	/* copy the string value */
	desc->stringValue = PORT_Strdup(value);
	break;
		    
    case boolType:
	if ('t' == *value || 'y' == *value)		/* true, yes */
	    desc->allowed = PR_TRUE;
	else if ('m' == *value || 'c' == *value)	/* conditional, maybe */
	    desc->allowed = PR_MAYBE;
	else /* if ('f' == *value || 'n' == *value) */  /* false, no */
	    desc->allowed = PR_FALSE;
	break;

    case longType:
	desc->longValue = PORT_Atol(value);
	break;
    }
}

static void
processMetaInfo(ZIG * zig, policyAlgorithmDesc *desc)
{
    unsigned long infoLen;	/* pointless, since info is always a string */
    void * 	info;
    int 	result;

    info = NULL;
    result = SOB_get_metainfo(zig, NULL, desc->policyDesc, &info, &infoLen);
    if (0 <= result) {
	parseMetaInfo(desc, (char *)info);
    }
    if (info) 
	PORT_Free(info);
}

static PRBool
isPolicyCert( CERTCertificate * cert )
{
    PRBool ret;
    if (cert->derPublicKey.len != sizeof policyCaCertDERPubKey)
	return PR_FALSE;
    ret = (PRBool)(0 == memcmp( cert->derPublicKey.data, policyCaCertDERPubKey,
			sizeof policyCaCertDERPubKey));
    return ret;
}

/* copy up to "destLen"-1 "numeric characters" from src to dest,
 * where "numeric characters" are 0-9 and "."
 * If strspn() existed in NSPR, or on all platforms, I wouldn't need this...
 */
static void
numStrNCpy(char * dest, const char * src, int destLen)
{
    static const char   numChrs[] 	= { "0123456789." };
    static       char   isNumeric[256]; 
           const char * cp;

    for (cp = numChrs; *cp != 0; ++cp)
    	isNumeric[(unsigned char)(*cp)] = 1;
    while (--destLen > 0 ) {
	char cc = *src++;
    	if (!isNumeric[(unsigned char)cc])
	    break;
	*dest++ = cc;
    }
    *dest = 0;
}

/* returns true if policy file is wrong version for this user.
 * returns false if policy file is OK for this user.
 */
static PRBool
wrongVersion( ZIG * zig )
{
    char *	stringValue;
    const char *langString;
    int		slen;
    char 	cmpString[100];
    char 	verString[100];

    static const char format[]	= { "%s/%s%s [%s] " };
    static const char en[]	= { "en" };

    /* if XP_AppLanguage does not contain a valid language string, use "en" */
    langString = (XP_AppLanguage == NULL || *XP_AppLanguage == 0) ? en : 
    		  XP_AppLanguage;

    /* get the "numeric" portion of the app version string into our buffer */
    numStrNCpy(verString, XP_AppVersion, sizeof verString);

    /* avoid buffer overflows */
    slen = PORT_Strlen(XP_AppCodeName)	+ 
    	   PORT_Strlen(verString) 	+ 
    	   PORT_Strlen(XP_PolicyVersion)+ 
    	   PORT_Strlen(langString)	+ 6;
    PORT_Assert( slen <= sizeof cmpString); 
    if (slen > sizeof cmpString)
	return PR_TRUE;	/* failed */

    /* construct our local version string for comparison */
    sprintf(cmpString, format, XP_AppCodeName, verString, 
    	    XP_PolicyVersion, langString);

    /* now get the software version string from the policy file */
    processMetaInfo( zig, &algTable[ALGNDX_SW_VERSION] );
    stringValue = algTable[ALGNDX_SW_VERSION].stringValue;
    if (!stringValue)		/* MUST be present */
    	return PR_TRUE;

    slen = PORT_Strlen(stringValue);
    if (!slen)
    	return PR_TRUE;		/* MUST NOT be empty */

    /* they must match for as much of the string as is present in the file */
    if (0 == PORT_Strncmp(stringValue, cmpString, slen))
    	return PR_FALSE;	/* passed with flying colors */
    return PR_TRUE;	/* failed */
}

static void
readPolicyFile(char *filename)
{

    ZIG * zig;
    int   err;
    int   i;
    PRBool   valid 	= PR_FALSE;
    SOBITEM *item;

    zig = SOB_new();
    if (!zig) {
    	/* set some error ?? XXX */
	return;
    }
    err = SOB_pass_archive( ZIG_F_GUESS, filename, "", zig);
    if (err < 0)
    	goto fail;		/* file doesn't exist */
    if (zig->valid < 0)
    	goto fail;		/* or fails signature test */

    /* Verify that it is signed by netscape ! */
    SOB_find(zig, "", ZIG_WALK);
    while (SOB_find_next(zig, &item) >= 0) { 
	FINGERZIG *	fing;

	fing = (FINGERZIG *) item->data;

	if (!fing || !fing->cert) 
	    break;		/* bogus cert chain, stop here */
	valid = isPolicyCert( fing->cert );
	if (valid)
	    break;		/* found it, stop here */
    }

    if (!valid) 
    	goto fail;

    /* Validate the version number field before processing the rest. */
    if (wrongVersion(zig))
    	goto fail;

    /* parse name-value pairs that we care about */
    for (i = 1; i < tableSize; ++i) {
	processMetaInfo( zig, algTable + i);
    }
    policyFileIsOK = PR_TRUE;

fail:
    (void)SOB_destroy(&zig);
}

/*
 *  Read in policy bits.   
 */
void
SECNAV_PolicyInit(void)
{
    policyAlgorithmDesc *desc	= algTable;
    char * name;

    char* nativeName = WH_FileName("", xpCryptoPolicy);
    char* urlName = XP_PlatformFileToURL(nativeName);
    name = XP_STRDUP(urlName + XP_STRLEN("file://"));
    if (urlName) XP_FREE(urlName);
    if (nativeName) XP_FREE(nativeName);

ACQUIRE_WRITE_LOCK

    if (name) {
    	readPolicyFile(name);
	PORT_Free(name);
    }

RELEASE_WRITE_LOCK

    if (! policyFileIsOK) {
	MWContext * 	context;
	char *		tmpMsg;
	char * 		msg;

	context = XP_FindSomeContext();
	tmpMsg  = XP_GetString(XP_CONFIG_NO_POLICY_STRINGS);
	if (tmpMsg != NULL) {
	    msg = PORT_Strdup(tmpMsg);
	    if (msg != NULL) {
		FE_Alert(context, msg);
		PORT_Free(msg);
	    }
	}
    }

}


struct SECNAVPolicyFindStr {
    int		nextAlgDescIndex;
    long 	protocolID[1];	/* actual count may be larger */
};


/*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *   Query functions
 */

SECNAVPolicyFind *
SECNAV_FindInit(const char * cipherFamily, 
		PRBool policyFilter, 	 /* include iff allowed by policy */
		PRBool preferenceFilter) /* include iff preference chosen */
{
    SECNAVPolicyFind *	snpf;
    int			famLen	= PORT_Strlen(cipherFamily);

    snpf = (SECNAVPolicyFind *)malloc(
    		(sizeof snpf) + tableSize * (sizeof snpf->protocolID[0]) );
    if (snpf) {
	int	ndx;
	int	count = -1;

ACQUIRE_READ_LOCK

	for (ndx = tableSize; --ndx >= 0; ) {
	    if (!PORT_Strncmp(algTable[ndx].sectionName, cipherFamily, famLen)) {
		if (policyFilter && ! algTable[ndx].allowed)
		    continue;
		if (preferenceFilter && ! algTable[ndx].preferred)
		    continue;
	    	snpf->protocolID[++count] = algTable[ndx].protocolID;
	    }
	}
	snpf->nextAlgDescIndex = count;

RELEASE_READ_LOCK

    }
    return snpf;
}


long
SECNAV_FindNext(SECNAVPolicyFind * snpf)
{
    if (!snpf || 0 > snpf->nextAlgDescIndex)
    	return -1L;
    return snpf->protocolID[snpf->nextAlgDescIndex--];
}


void
SECNAV_FindEnd(SECNAVPolicyFind * snpf)
{
    PORT_Free(snpf);
}


PRBool
SECNAV_AlgPreferred(long protocolID)
{
    policyAlgorithmDesc *desc	= algTable;
    int 		count 	= tableSize;
    PRBool		ret	= PR_FALSE;

    while (count && protocolID != desc->protocolID) {
    	++desc;
	--count;
    }
    if (count > 0)
    	ret = desc->preferred;
    return ret;
}

PRBool
SECNAV_AlgAllowed(long protocolID)
{
    policyAlgorithmDesc *desc	= algTable;
    int 		count 	= tableSize;
    PRBool		ret	= PR_FALSE;

    while (count && protocolID != desc->protocolID) {
    	++desc;
	--count;
    }
    if (count > 0)
    	ret = desc->allowed;
    return ret;
}

PRBool
SECNAV_AlgOKToDo(long protocolID)
{
    policyAlgorithmDesc *desc	= algTable;
    int 		count 	= tableSize;

    while (count && protocolID != desc->protocolID) {
    	++desc;
	--count;
    }
    return (PRBool)(count > 0 && desc->preferred && desc->allowed);
}

long
SECNAV_getMaxKeyGenBits(void)
{
    return algTable[ALGNDX_KEYGEN_BITS].longValue;
}

const char *
SECNAV_GetProtoName(long protocolID)
{
    policyAlgorithmDesc *desc	= algTable;
    int 		count 	= tableSize;
    const char *	ret	= NULL;

    while (count && protocolID != desc->protocolID) {
    	++desc;
	--count;
    }
    if (count > 0)
    	ret = (const char *)desc->prefName;
    return ret;
}

const char *
SECNAV_GetProtoDesc(long protocolID)
{
    policyAlgorithmDesc *desc	= algTable;
    int 		count 	= tableSize;
    const char *	ret	= NULL;

    while (count && protocolID != desc->protocolID) {
    	++desc;
	--count;
    }
    if (count > 0)
    	ret = (const char *)XP_GetString(desc->descNum[0]);
    return ret;
}

