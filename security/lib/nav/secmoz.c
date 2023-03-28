/*
 * Security stuff specific to the client (mozilla).
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secmoz.c,v 1.99.2.14 1997/05/24 00:23:41 jwz Exp $
 */
#include "xp.h"
#include "cert.h"
#include "secder.h"
#include "secitem.h"
#include "key.h"
#include "crypto.h"
#include "base64.h"
#include "net.h"
#include "secnav.h"
#include "ssl.h"
#include "secmime.h"	/* for SECMIME_ funcs */
#ifdef XP_MAC
#include "certdb.h"
#include "sslimpl.h"
#else
#include "../cert/certdb.h"
#include "../ssl/sslimpl.h"
#endif
#include "htmldlgs.h"
#include "sslproto.h"
#include "sslerr.h"
#include "fe_proto.h"
#include "xpgetstr.h"
#include "prefapi.h"
#include "xp_file.h"
#include "secport.h"
#include "secasn1.h"
#include "pk11func.h"
#include "secmod.h"
#include "secrng.h"
#include "pkcs12.h"
#include "p12plcy.h"

#ifdef JAVA
#include "java.h"
#endif


#ifdef FORTEZZA
#include "fortdlgs.h"
#endif

extern int XP_CONFIG_NO_CIPHERS_STRINGS;
extern int XP_CONFIG_MAYBE_CIPHERS_STRINGS;
extern int XP_CONFIG_SMIME_CIPHERS_STRINGS;
extern int XP_CONFIG_SSL2_CIPHERS_STRINGS;
extern int XP_CONFIG_SSL3_CIPHERS_STRINGS;
extern int XP_CONFCIPHERS_TITLE_STRING;
extern int XP_HIGH_GRADE;
extern int XP_MEDIUM_GRADE;
extern int XP_LOW_GRADE;
extern int XP_VIEW_CERT_POLICY;
extern int XP_CHECK_CERT_STATUS;
extern int XP_SSL2_RC4_128;
extern int XP_SSL2_RC2_128;
extern int XP_SSL2_DES_192_EDE3;
extern int XP_SSL2_DES_64;
extern int XP_SSL2_RC4_40;
extern int XP_SSL2_RC2_40;
extern int XP_SSL3_RSA_RC4_128_MD5;
extern int XP_SSL3_RSA_3DES_SHA;
extern int XP_SSL3_RSA_DES_SHA;
extern int XP_SSL3_RSA_RC4_40_MD5;
extern int XP_SSL3_RSA_RC2_40_MD5;
extern int XP_SSL3_RSA_NULL_MD5;
#ifdef FORTEZZA
extern int XP_SSL3_FORTEZZA_SHA;
extern int XP_SSL3_FORTEZZA_RC4_SHA;
extern int XP_SSL3_FORTEZZA_NULL_SHA;
#endif
extern int SEC_ERROR_NO_MEMORY;
extern int XP_SEC_ENTER_EXPORT_PWD;
extern int XP_SEC_ENTER_IMPORT_PWD;
extern int XP_SEC_PROMPT_NICKNAME_COLLISION;
extern int SEC_ERROR_BAD_EXPORT_ALGORITHM;
extern int SEC_ERROR_IMPORTING_CERTIFICATES;
extern int SEC_ERROR_EXPORTING_CERTIFICATES;
extern int XP_PKCS12_IMPORT_FILE_PROMPT;
extern int XP_PKCS12_EXPORT_FILE_PROMPT;
extern int SEC_ERROR_PKCS12_DECODING_PFX;
extern int SEC_ERROR_PKCS12_INVALID_MAC;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE;
extern int SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM;
extern int SEC_ERROR_PKCS12_UNSUPPORTED_VERSION;
extern int SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT;
extern int SEC_ERROR_PKCS12_CERT_COLLISION;
extern int SEC_ERROR_PKCS12_DUPLICATE_DATA;
extern int SEC_ERROR_USER_CANCELLED;
extern int XP_PKCS12_SUCCESSFUL_IMPORT;
extern int XP_PKCS12_SUCCESSFUL_EXPORT;
extern int SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME;
extern int SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY;
extern int SEC_ERROR_PKCS12_UNABLE_TO_READ;
extern int SEC_ERROR_PKCS12_UNABLE_TO_WRITE;
extern int SEC_ERROR_PKCS12_KEY_DATABASE_NOT_INITIALIZED;

extern int SEC_PK11_MANUFACTURER;
extern int SEC_PK11_LIBARARY;
extern int SEC_PK11_TOKEN;
extern int SEC_PK11_PRIV_TOKEN;
extern int SEC_PK11_SLOT;
extern int SEC_PK11_PRIV_SLOT;
extern int SEC_PK11_FIPS_SLOT;
extern int SEC_PK11_FIPS_PRIV_SLOT;

extern int XP_SEC_REENTER_TO_CONFIRM_PWD;
extern int XP_SEC_BAD_CONFIRM_EXPORT_PWD;

void SEC_Init(void);

/* What personal cert does the user want to be presented by default? */

struct {
    enum { ASK, AUTO, STR } mode;
    char *chosen;
} UserCertChoice = {
    ASK,
    (char *) 0
};


/*
 * Representation of a key size.  The name is how it is displayed
 * to the user for selection purposes.  XXX The attributes need to
 * be implemented; they should describe appropriate (or inappropriate)
 * uses of the key size.  (For example, a 1024-bit key can be used in
 * exported environments for payment purposes only.  This should be
 * marked in the attributes so when the keygen is being done for a
 * payment protocol it can choose 1024-bit keys but keygens for other
 * purposes cannot choose 1024.)  I think.
 */
typedef struct SECKeySizeChoiceInfoStr {
    char *name;
    int size;
    unsigned int attributes;
} SECKeySizeChoiceInfo;

/*
 * All possible key size choices.
 */
static SECKeySizeChoiceInfo SECKeySizeChoiceList[] = {
    { (char *)0, 1024, 0 },
    { (char *)0, 768, 0 },
    { (char *)0, 512, 0 },
    { (char *)0, 0, 0 }
};

static SECStatus enable_SSL_cipher_prefs(void);
static SECStatus enable_SSL_cipher_policies(void);
static SECStatus enable_SMIME_cipher_prefs(void);
static SECStatus enable_SMIME_cipher_policies(void);
static SECStatus enable_PKCS12_cipher_policies(void);

typedef struct _CipherConfigState CipherConfigState;

struct _CipherConfigState 
{
    int cipher;
    char *name;
    char *desc;
    int version;
    PRBool enabled;
};

/* ---------------- executable code follows ----------------------- */

static PQGParams *
decode_pqg_params(char *str)
{
    char *buf;
    unsigned int len;
    PRArenaPool *arena;
    PQGParams *params;
    SECStatus status;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	return NULL;

    params = PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (params == NULL)
	goto loser;
    params->arena = arena;

    buf = (char *)ATOB_AsciiToData(str, &len);
    if ((buf == NULL) || (len == 0))
	goto loser;

    status = SEC_ASN1Decode(arena, params, SECKEY_PQGParamsTemplate, buf, len);
    if (status != SECSuccess)
	goto loser;

    return params;

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}

static int
pqg_prime_bits(char *str)
{
    PQGParams *params = NULL;
    int primeBits = 0, i;

    params = decode_pqg_params(str);
    if (params == NULL)
	goto done; /* lose */

    for (i = 0; params->prime.data[i] == 0; i++)
	/* empty */;
    primeBits = (params->prime.len - i) * 8;

done:
    if (params != NULL)
	PQG_DestroyParams(params);
    return primeBits;
}

/*
 * Should return only the choices available depending on export, etc.
 * AND the first item in the list should be the chosen default.
 * (Later, the specification parameter should also have control over
 * limiting the choices or specifying the default.)
 */
char **
SECNAV_GetKeyChoiceList(char *type, char *pqgString)
{
    char **	list;
    char *	pqg;
    char *	end;
    int 	i;
    int 	j;
    int 	len;
    int 	primeBits;

    len = sizeof (SECKeySizeChoiceList) / sizeof (SECKeySizeChoiceInfo);

    /* XXX Help! When do I free this??? */
    list = (char**)PORT_Alloc (len * sizeof (char **));

    if (pqgString != NULL)
	pqgString = PORT_Strdup(pqgString);

    if (type && (PORT_Strcasecmp(type, "dsa") == 0)) {
	if (pqgString == NULL) {
	    list[0] = NULL;
	    return list;
	}
	pqg = pqgString;

	j = 0;
	do {
	    end = PORT_Strchr(pqg, ',');
	    if (end != NULL)
		*end = '\0';
	    primeBits = pqg_prime_bits(pqg);
	    for (i = 0; SECKeySizeChoiceList[i].size != 0; i++) {
		if (SECKeySizeChoiceList[i].size == primeBits) {
		    list[j++] = SECKeySizeChoiceList[i].name;
		    break;
		}
	    }
	    pqg = end + 1;
	} while (end != NULL);
    } else {
	int maxKeyBits = SECNAV_getMaxKeyGenBits();
	j = 0;
	for (i = 0; SECKeySizeChoiceList[i].size != 0; i++) {
	    if (SECKeySizeChoiceList[i].size <= maxKeyBits)
		list[j++] = SECKeySizeChoiceList[i].name;
	}
    }
    list[j] = NULL; 

    if (pqgString != NULL)
	PORT_Free(pqgString);
    return list;
}

/*
 * Based on the choice specified, generate a public/private key pair,
 * recording the pair in the key database.
 */
PRBool
SECNAV_GenKeyFromChoice(void *proto_win, LO_Element *form,
			char *choiceString, char *challenge,
			char *typeString, char *pqgString,
			char **pValue, PRBool *pDone)
{
    SECKeySizeChoiceInfo *choice;
    SECStatus rv;
    char *end, *str;
    PQGParams *pqg = NULL;
    int primeBits;
    PRBool found_match = PR_FALSE;
    KeyType type;

    choice = SECKeySizeChoiceList;
    while (PORT_Strcmp(choice->name, choiceString) != 0)
	choice++;

    if (typeString == NULL || (PORT_Strcasecmp(typeString, "rsa") == 0)) {
	type = rsaKey;
    } else if (PORT_Strcasecmp(typeString, "dsa") == 0) {
	type = dsaKey;
    } else {
	return PR_FALSE;
    }

    if (pqgString != NULL)
	pqgString = PORT_Strdup(pqgString);

    if (type == dsaKey) {
	if (pqgString == NULL)
	    return PR_FALSE;

	str = pqgString;

	do {
	    end = PORT_Strchr(str, ',');
	    if (end != NULL)
		*end = '\0';
	    primeBits = pqg_prime_bits(str);
	    if (choice->size == primeBits)
		goto found_match;
	    str = end + 1;
	} while (end != NULL);

	PORT_Free(pqgString);
	return PR_FALSE;

    found_match:
	pqg = decode_pqg_params(str);
    }

    /*
     * XXX Should we check that *pValue is NULL when it comes in?
     * If so, we should we do if it is not -- just free it?
     */
    *pValue = NULL;

    /* XXX - who owns challenge string?? */
    rv = SEC_MakeKeyGenDialog(proto_win, form, choice->size, challenge,
			      type, pqg, pValue, pDone);

    if (pqgString != NULL)
	PORT_Free(pqgString);
    return PR_TRUE;
}

char *
SECNAV_GetHostFromURL(char *url)
{
    char *hostname;
    char *colon;

    /* if no colon, then it must be only the hostname */
    colon = PORT_Strchr(url, ':');
    if ( colon == NULL ) {
	hostname = PORT_Strdup(url);
	return(hostname);
    }
    
    hostname = NET_ParseURL(url, GET_HOST_PART);
    
    /* Sigh, netlib include the :portnumber as part of the "hostname" */
    colon = PORT_Strchr(hostname, ':');
    if ( colon != NULL ) {
	*colon = '\0';
    }

    return(hostname);
}

void
SECNAV_Posting(int fd)
{
    SSLSocket *ss;
    
    ss = ssl_FindSocket(fd);
    
    if ( ss && ss->sec ) {
	ss->sec->app_operation = SSLAppOpPost;
    }
    
    return;
}

void
SECNAV_HTTPHead(int fd)
{
    SSLSocket *ss;
    
    ss = ssl_FindSocket(fd);
    
    if ( ss && ss->sec ) {
	ss->sec->app_operation = SSLAppOpHeader;
    }
    
    return;
}

/*
 * Callback from SSL for checking certificate of the server
 */
int
SECNAV_ConfirmServerCert(void *arg, int fd, PRBool checkSig, PRBool isServer)
{
    SECStatus rv;
    SSLSocket *ss;
    char *hostname;
    CERTCertificate *cert;

    if ( isServer ) {
	return((int)SECFailure);
    }
    
    ss = ssl_FindSocket(fd);
    
    if ( ss && ss->sec ) {
	rv = CERT_VerifyCertNow(CERT_GetDefaultCertDB(), ss->sec->peerCert,
				checkSig, certUsageSSLServer, arg);
	if ( rv == SECSuccess ) { /* cert is OK */

	    /* now check the name field in the cert */
	    if ( ss->sec->url ) {
		hostname = SECNAV_GetHostFromURL(ss->sec->url);
		if ( hostname ) {
		    rv = CERT_VerifyCertName(ss->sec->peerCert, hostname);
		    PORT_Free(hostname);
		}
	    }
	}
	if ( rv == SECSuccess ) {
	    if ( ( ss->sec->app_operation == SSLAppOpPost ) &&
		( ss->sec->post_ok != PR_TRUE ) ) {
		cert = ss->sec->peerCert;
		/* generate a warning if we are doing a post and the user
		 * has requested it.
		 */
		if ( cert->trust &&
		    ( cert->trust->sslFlags & CERTDB_SEND_WARN ) ) {
		    PORT_SetError(SSL_ERROR_POST_WARNING);
		    rv = SECFailure;
		}
	    }
	}
    } else {
	rv = SECFailure;
    }

    return((int)rv);
}

void mydestroy(void *cert, PRBool dummy)
{
    CERT_DestroyCertificate((CERTCertificate *)cert);
}

static SECStatus
SECCmpCertChainWCANames(CERTCertificate *cert, CERTDistNames *caNames) 
{
    int j;
    SECItem *caname;
    SECItem issuerName;
    SECItem compatIssuerName;
    int headerlen;
    uint32 contentlen;
    SECStatus rv;
    CERTCertificate *curcert, *oldcert;
    int depth;
    
    depth=0;
    curcert = CERT_DupCertificate(cert);
    
    while( curcert ) {
	issuerName = cert->derIssuer;

	/* compute an alternate issuer name for compatibility with 2.0
	 * enterprise server, which send the CA names without
	 * the outer layer of DER hearder
	 */
	rv = DER_Lengths(&issuerName, &headerlen, &contentlen);
	if ( rv == SECSuccess ) {
	    compatIssuerName.data = &issuerName.data[headerlen];
	    compatIssuerName.len = issuerName.len - headerlen;
	} else {
	    compatIssuerName.data = NULL;
	    compatIssuerName.len = 0;
	}
	    
	for (j = 0; j < caNames->nnames; j++) {
	    caname = &caNames->names[j];
	    if (SECITEM_CompareItem(&issuerName, caname) == SECEqual) {
		rv = SECSuccess;
		CERT_DestroyCertificate(curcert);
		goto done;
	    } else if (SECITEM_CompareItem(&compatIssuerName, caname) == SECEqual) {
		rv = SECSuccess;
		CERT_DestroyCertificate(curcert);
		goto done;
	    }
	}
	if ( ( depth <= 20 ) &&
	    ( SECITEM_CompareItem(&curcert->derIssuer, &curcert->derSubject)
	     != SECEqual ) ) {
	    oldcert = curcert;
	    curcert = CERT_FindCertByName(curcert->dbhandle,
					  &curcert->derIssuer);
	    CERT_DestroyCertificate(oldcert);
	    depth++;
	} else {
	    CERT_DestroyCertificate(curcert);
	    curcert = NULL;
	}
    }
    rv = SECFailure;

done:
    return(rv);
}       

int
SECNAV_GetClientAuthData(void *arg,
			 int fd,
			 CERTDistNames *caNames,
			 CERTCertificate **pRetCert,
			 SECKEYPrivateKey **pRetKey)
{
    CERTCertificate *cert;
    void *proto_win;
    SECKEYPrivateKey *privkey;
    int ret;
    SSLSocket *ss;
    PRBool isFortezza = PR_FALSE;
    
    proto_win = (void *)arg;
    ss = ssl_FindSocket(fd);

    /* in the case of fortezza we already know the cert (we always
     * need the cert, so we grab it early..
     */
    if (ss && ss->ssl3) {
#ifdef notdef
	if (ss->ssl3->hs.kea == kea_fortezza) {
	    isFortezza = PR_TRUE; /* client cert must be fortezza */
	}
#endif
	/* Diffie-Helman is the other we need to worry about */
	/* Allow DSA and RSA to work interchangably */
    }

    PORT_Assert(!isFortezza);

    switch (UserCertChoice.mode) {
    case STR:
        cert = PK11_FindCertFromNickname(UserCertChoice.chosen,proto_win);
	if ( !cert ) {
	    ret = SECNAV_MakeClientAuthDialog(proto_win, fd, caNames);
	    break;
	}

	privkey = PK11_FindKeyByAnyCert(cert,proto_win);	
	if ( !privkey ) {
	    CERT_DestroyCertificate(cert);
	    ret = SECNAV_MakeClientAuthDialog(proto_win, fd, caNames);
	    break;
	}
	
	*pRetCert = cert;
	*pRetKey = privkey;
	
	ret = 0;
	break;
    case AUTO:
    {
	int i;
	CERTCertNicknames *names = NULL;
	CERTCertificate *cert;
	SECStatus rv;
	
	names = CERT_GetCertNicknames(CERT_GetDefaultCertDB(),
				      SEC_CERT_NICKNAMES_USER,proto_win);
	if (names != NULL) {
	    for (i = 0; i < names->numnicknames; i++) {
		cert = PK11_FindCertFromNickname(names->nicknames[i],proto_win),
		rv = SECCmpCertChainWCANames(cert, caNames);
		if ( rv == SECSuccess ) {
		    *pRetCert = cert;
		    *pRetKey=PK11_FindKeyByAnyCert(cert, proto_win);
		    CERT_FreeNicknames(names);
		    return(0);
		} else {
		    CERT_DestroyCertificate(cert);
		}
	    }
	    CERT_FreeNicknames(names);
	}
    }
    /* NOTE - fall through */
    default:
	ret = SECNAV_MakeClientAuthDialog(proto_win, fd, caNames);
	break;
    }
    /* since we are handling this asynchronously we can ignore the
     * cert and key return values.  They will be set when we restart
     * the connection.
     */
    return(ret);
}

SECStatus RNG_RNGInit(); /* XXX move when we move RNG_RNGInit to pkcs11.c */


static char *
certDBNameCallback(void *arg, int dbVersion)
{
    char *fnarg;
    
    switch ( dbVersion ) {
      case 7:
	fnarg = "7";
	break;
      case 6:
	fnarg = "6";
	break;
      case 5:
	fnarg = "5";
	break;
      case 4:
      default:
	fnarg = NULL;
	break;
    }
    
    return(WH_FileName(fnarg, xpCertDB));
}

static char *
keyDBNameCallback(void *arg, int dbVersion)
{
    char *fnarg;
    
    switch ( dbVersion ) {
      case 3:
	fnarg = "3";
	break;
      case 2:
      default:
	fnarg = NULL;
	break;
    }
    
    return(WH_FileName(fnarg, xpKeyDB));
}

int SECNAV_AskPrefToPK11(int);

char * PR_CALLBACK
SECNAV_GetCAPolicyString(char *org, unsigned long noticeNumber, void *arg)
{
    char *prefname;
    char *charvalue = NULL;
    int ret;
    int len;
    char *tmporg, *saveorg;
    int c;
    
    /* get space for a new copy of the org string */
    len = PORT_Strlen(org);
    saveorg = tmporg = (char *)PORT_Alloc(len + 1);
    if ( saveorg == NULL ) {
	return(NULL);
    }
    
    /* only use characters that are alpha */
    while ( ( c = *org ) != '\0' ) {
	if ( isalpha(c) ) {
	    *tmporg = c;
	    tmporg++;
	}
	org++;
    }
    *tmporg = '\0';
    
    prefname = PR_smprintf("security.canotices.%s.notice%ld",
			   saveorg, noticeNumber);
    
    PORT_Free(saveorg);
    
    if ( prefname != NULL ) {
	ret = PREF_CopyCharPref(prefname, &charvalue);
	PORT_Free(prefname);
    }
    
    return(charvalue);
}

/*
 * Initialize the security library for mozilla
 */
void
SECNAV_Init(void)
{
    CERTCertDBHandle *	cdb_handle;
    SECKEYKeyDBHandle *	kdb_handle;
    char *		keydbname;
    char *		secmodule;
    int 		i;
    int 		ret;
    SECStatus 		rv;
    XP_Bool 		prefval;
    int32		ask,timeout;
    PK11SlotInfo	*slot;
    char *		prefchar = NULL;
    
    /*
     * Deal with non-hard-coded strings before other initialization.
     */

    i = 0;
    SECKeySizeChoiceList[i++].name = PORT_Strdup(XP_GetString(XP_HIGH_GRADE));
    SECKeySizeChoiceList[i++].name = PORT_Strdup(XP_GetString(XP_MEDIUM_GRADE));
    SECKeySizeChoiceList[i++].name = PORT_Strdup(XP_GetString(XP_LOW_GRADE));


    /*
     * Initialize the certificate database.
     */
    cdb_handle = (CERTCertDBHandle *)PORT_ZAlloc(sizeof(CERTCertDBHandle));
    if (cdb_handle != NULL) {
	rv = CERT_OpenCertDB(cdb_handle, PR_FALSE, certDBNameCallback, NULL);
	
	if (rv != SECSuccess) {
	    /* that failed; try getting a "volatile" database */
	    rv = CERT_OpenVolatileCertDB(cdb_handle);
	}
	if (rv == SECSuccess) {
	    CERT_SetDefaultCertDB(cdb_handle);
	}
    }

    /*
     * Initialize the private key database.
     */
    /* in the "ideal world"(tm), the Key database would be initialized in
     * the Software PKCS#11 module. Unfortunately there's not enough of
     * an interface to pass all the info to the PCKS#11 module, so go ahead
     * and initialize here...
     */
    RNG_RNGInit(); /* #$@$#@ Guess what SECKEY_OpenKeyDB calls if there is
		    * no keyDB? You got it Get Random Data... just one more
		    * reason to want to move this call into pkcs11.c
		    */

    
    kdb_handle = SECKEY_OpenKeyDB(PR_FALSE, keyDBNameCallback, NULL);
    
    if (kdb_handle != NULL) {
	SECKEY_SetDefaultKeyDB(kdb_handle);
    }

    /*
     * Configure the strings for pkcs11.
     */
    {
	char *man= PORT_Strdup(XP_GetString(SEC_PK11_MANUFACTURER));
	char *libdes = PORT_Strdup(XP_GetString(SEC_PK11_LIBARARY));
	char *tokdes = PORT_Strdup(XP_GetString(SEC_PK11_TOKEN));
	char *ptokdes = PORT_Strdup(XP_GetString(SEC_PK11_PRIV_TOKEN));
	char *slotdes = PORT_Strdup(XP_GetString(SEC_PK11_SLOT));
	char *pslotdes = PORT_Strdup(XP_GetString(SEC_PK11_PRIV_SLOT));
	char *fslotdes = PORT_Strdup(XP_GetString(SEC_PK11_FIPS_SLOT));
	char *fpslotdes = PORT_Strdup(XP_GetString(SEC_PK11_FIPS_PRIV_SLOT));

	PK11_ConfigurePKCS11(man,libdes,tokdes,ptokdes,slotdes,pslotdes,
							fslotdes,fpslotdes);
    }


    /*
     * set our default password function
     */
    PK11_SetPasswordFunc(SECNAV_PromptPassword);

    /*
     * OK, now we initialize the PKCS11 subsystems
     */
    secmodule = WH_FileName(NULL,xpSecModuleDB);
    SECMOD_init(secmodule);


    ret = PREF_GetIntPref("security.ask_for_password",&ask);
    ret = PREF_GetIntPref("security.password_lifetime",&timeout);

    slot = PK11_GetInternalKeySlot();
    PK11_SetSlotPWValues(slot,SECNAV_AskPrefToPK11((int)ask),(int)timeout);
    PK11_FreeSlot(slot);

    /*
     * Initialize the common parts of the security library.
     */
    SEC_Init();

    /* read export policy bits */
    SECNAV_PolicyInit();

    /* tell SSL what algs it is allowed to do by policy */
    enable_SSL_cipher_policies();

    /* tell SMIME what algs it is allowed to do by policy */
    enable_SMIME_cipher_policies();

    /* tell PKCS12 what algs it is allowed to do by policy */
    enable_PKCS12_cipher_policies();

    /* read preferences corresponding to policy bits */
    SECNAV_PrefsRefresh();

    /*
     * Enable/Disable SSL based on user prefs
     */
    ret = PREF_GetBoolPref("security.enable_ssl2", &prefval);
    SSL_EnableDefault(SSL_ENABLE_SSL2, prefval);
    
    ret = PREF_GetBoolPref("security.enable_ssl3", &prefval);
    SSL_EnableDefault(SSL_ENABLE_SSL3, prefval);

    ret = PREF_CopyCharPref("security.default_personal_cert", &prefchar);
    SECNAV_SetDefUserCertList(prefchar);

    /*
     * Initialize the cipher preferences specified by the user.
     */
    enable_SSL_cipher_prefs();
    enable_SMIME_cipher_prefs();

    SECNAV_DefaultEmailCertInit();

    CERT_SetCAPolicyStringCallback(SECNAV_GetCAPolicyString, NULL);
}

/*
 * Shutdown the security library.
 *
 * Currently closes the certificate and key databases.
 */
void
SECNAV_Shutdown(void)
{
    CERTCertDBHandle *cdb_handle;
    SECKEYKeyDBHandle *kdb_handle;

    cdb_handle = CERT_GetDefaultCertDB();
    if (cdb_handle != NULL) {
	CERT_ClosePermCertDB(cdb_handle);
    }

    kdb_handle = SECKEY_GetDefaultKeyDB();
    if (kdb_handle != NULL) {
	SECKEY_CloseKeyDB(kdb_handle);
    }
}

int
SECNAV_SetupSecureSocket(int fd, char *url, MWContext *cx)
{
    int status;
    
    if ( cx == NULL ) {
	goto loser;
    }

#ifndef AKBAR
#ifdef JAVA
    /* XXX - if this is a java context, then we lose for now, since we can't
     * figure out how to get the correct context at this point.
     */
    if ( cx->type == MWContextJava ) {
	cx = NSN_JavaContextToRealContext(cx);
    }
#endif
#endif /* !AKBAR */
    
    /* pass URL to ssl code */
    status = SSL_SetURL(fd, (char *)url);
    if (status) {
	goto loser;
    }

    /* set certificate validation hook */
    status = SSL_AuthCertificateHook(fd, SECNAV_ConfirmServerCert,
				     (void *)cx);

    if (status) {
	goto loser;
    }

    status = SSL_GetClientAuthDataHook(fd, SECNAV_GetClientAuthData,
				       (void *)cx);

    if (status) {
	goto loser;
    }

    status = SSL_BadCertHook(fd, SECNAV_HandleBadCert, (void *)cx);
		
    if (status) {
	goto loser;
    }

#ifdef FORTEZZA
    status = Fortezza_AlertHook(fd, SECMOZ_FortezzaAlert, (void *)cx);
		
    if (status) {
	goto loser;
    }
#endif

    return(0);
    
loser:
    return(-1);
}

void
SECNAV_NewUserCertificate(MWContext *cx)
{
    URL_Struct *fetchurl;
    
    fetchurl = NET_CreateURLStruct(SEC_CLIENT_CERT_PAGE, NET_DONT_RELOAD);

    /* don't set the window target if the context is null.  This is for
     * the Mac, which doesn't have an MWContext for preferences.
     */
    if ( cx != NULL ) {
	StrAllocCopy(fetchurl->window_target, "_CA_PAGE");
    }

    FE_GetURL(cx,fetchurl);

    return;
}


/*
 * Password obfuscation, etc.
 */

static unsigned char secmoz_tmmdi[] = {	/* tmmdi == They Made Me Do It */
    0xd0,	/* jsw, karlton */
    0x86,	/* repka, paquin */
    0x9c,	/* freier, elgamal */
    0xde,	/* jonm, bobj */
    0xc6,	/* jevering, kent */
    0xee,	/* fur, sharoni */
    0xeb,	/* ari, sk */
    0x3e	/* terry, atotic */
};

char *
SECNAV_MungeString(const char *unmunged_string)
{
    RC4Context *rc4;
    unsigned char *munged_data;
    unsigned int inlen, outlen, maxlen;
    char *munged_string;
    SECStatus rv;

    /*
     * The encrypted version of NULL is NULL.
     */
    if (unmunged_string == NULL)
	return NULL;

    inlen = PORT_Strlen(unmunged_string);

    /*
     * Similarly, the encrypted version of "" is "".
     * But, since our caller will want to free any non-null return value,
     * we actually have to allocate and copy this (no)thing.
     */
    if (inlen == 0)
	return PORT_Strdup(unmunged_string);

    maxlen = inlen + 64;		/* XXX slop */
    munged_data = (unsigned char *) PORT_Alloc(maxlen);
    if (munged_data == NULL)
	return NULL;

    rc4 = RC4_CreateContext(secmoz_tmmdi, sizeof(secmoz_tmmdi));
    if (rc4 == NULL) {
	PORT_Free(munged_data);
	return NULL;
    }

    rv = RC4_Encrypt(rc4, munged_data, &outlen, maxlen,
		     (const unsigned char *)unmunged_string, inlen);

    RC4_DestroyContext(rc4, PR_TRUE);

    if (rv != SECSuccess) {
	PORT_Free(munged_data);
	return NULL;
    }

    munged_string = BTOA_DataToAscii(munged_data, outlen);

    PORT_Free(munged_data);
    return munged_string;
}

char *
SECNAV_UnMungeString(const char *munged_string)
{
    RC4Context *rc4;
    unsigned char *munged_data;
    unsigned int inlen, outlen, maxlen;
    char *unmunged_string;
    SECStatus rv;

    /*
     * The decrypted version of NULL is NULL.
     */
    if (munged_string == NULL)
	return NULL;

    /*
     * Similarly, the decrypted version of "" is "".
     * But, since our caller will want to free any non-null return value,
     * we actually have to allocate and copy this (no)thing.
     */
    if (*munged_string == '\0')
	return PORT_Strdup(munged_string);

    munged_data = ATOB_AsciiToData(munged_string, &inlen);
    if (munged_data == NULL)
	return NULL;

    maxlen = inlen + 64;		/* XXX slop */
    unmunged_string = (char *) PORT_Alloc(maxlen);
    if (unmunged_string == NULL) {
	PORT_Free(munged_data);
	return NULL;
    }

    --maxlen;				/* reserve 1 for null terminator */

    rc4 = RC4_CreateContext(secmoz_tmmdi, sizeof(secmoz_tmmdi));
    if (rc4 == NULL) {
	PORT_Free(munged_data);
	PORT_Free(unmunged_string);
	return NULL;
    }

    rv = RC4_Decrypt(rc4, (unsigned char *)unmunged_string, &outlen, maxlen,
		     munged_data, inlen);

    RC4_DestroyContext(rc4, PR_TRUE);
    PORT_Free(munged_data);

    if (rv != SECSuccess) {
	PORT_Free(unmunged_string);
	return NULL;
    }

    unmunged_string[outlen] = '\0';
    return unmunged_string;
}

/*
 * extract the database key from a certificate and base64 encode it
 * to turn it into a string
 */

char *
secmoz_CertToKeyString(CERTCertificate *cert)
{
    char *keystring;
    
    keystring = BTOA_DataToAscii(cert->certKey.data, cert->certKey.len);
    
    return(keystring);
}

/*
 * Convert a URL with a base64 encoded cert database key in it to the
 * certificate that the key references.
 */
CERTCertificate *
secmoz_URLToCert(char *url)
{
    char *keystring;
    SECItem dbkey;
    CERTCertificate *cert;
    unsigned int size;

    /* base 64 encoded database key is after : */
    keystring = PORT_Strchr(url, ':');
    if ( keystring == NULL ) {
	return NULL;
    }

    keystring++;
    
    if ( *keystring == '\0' ) {
	return NULL;
    }

    /* do the base64 decode */
    dbkey.data = ATOB_AsciiToData(keystring, &size);
    dbkey.len = (size_t)size;

    if ( dbkey.data == NULL ) {
	return NULL;
    }

    /* look up the cert */
    cert = CERT_FindCertByKey(CERT_GetDefaultCertDB(), &dbkey);
    
    return(cert);
}

/*
 * make a string of ascii hex digits from the certificate serial number
 * this is used for CA URLs
 */
char *
secmoz_GetCertSNString(CERTCertificate *cert)
{
    SECItem *snitem;
    char *snstring, *str;
    int len;
    unsigned char *data;
    char t;
    
    snitem = &cert->serialNumber;
    
    len = snitem->len;
    data = snitem->data;
    
    str = snstring = PORT_Alloc(len * 2 + 1);
    
    if ( snstring ) {

	/* skip leading zeros */
	while ( ( len > 1 ) && ( *data == 0 ) ) {
	    data++;
	    len--;
	}

	/* convert to string */
	while (len) {
	    /* first digit */
	    t = (char)(*data >> 4);
	    *str++ = (t < 10 ) ? (t+'0') : (t - 10 + 'a');
	    /* second digit */
	    t = (char)(*data & 0xf);
	    *str++ = (t < 10 ) ? (t+'0') : (t - 10 + 'a');

	    data++;
	    len--;
	}
	*str = '\0';
    }
    
    return(snstring);
}

static void
fetchURL(void *closure)
{
    URL_Struct *fetchurl;
    MWContext *cx;
    
    fetchurl = (URL_Struct *)closure;
    cx = (MWContext *)fetchurl->sec_private;
    
    FE_GetURL(cx, fetchurl);
    return;
}

static void
secmoz_HandleInternalCertRevokeURL(URL_Struct *url, MWContext *cx,
				   CERTCertificate *cert)
{
    URL_Struct *fetchurl = NULL;
    char *serialnumber = NULL;
    char *fetchstring = NULL;
    char *urlstring = NULL;
    
    serialnumber = secmoz_GetCertSNString(cert);
    
    if ( serialnumber == NULL ) {
	goto loser;
    }

    urlstring = CERT_FindCertURLExtension(cert,
					 SEC_OID_NS_CERT_EXT_REVOCATION_URL,
					 SEC_OID_NS_CERT_EXT_CA_REVOCATION_URL);

    if ( urlstring == NULL ) {
	goto loser;
    }

    fetchstring = (char *)PORT_Alloc(PORT_Strlen(serialnumber) +
				   PORT_Strlen(urlstring) + 1);
    if ( fetchstring == NULL ) {
	goto loser;
    }

    /* copy the base url data */
    PORT_Strcpy(fetchstring, urlstring);

    /* copy the serial number */
    XP_STRCAT(fetchstring, serialnumber);
    
    fetchurl = NET_CreateURLStruct(fetchstring, NET_DONT_RELOAD);

    fetchurl->sec_private = (void *)cx;

    (void)FE_SetTimeout(fetchURL, (void *)fetchurl, 0);

    goto done;
    
loser:
    if ( fetchurl ) {
	NET_FreeURLStruct(fetchurl);
    }

done:
    if ( urlstring ) {
	PORT_Free(urlstring);
    }
    
    if ( serialnumber ) {
	PORT_Free(serialnumber);
    }

    if ( fetchstring ) {
	PORT_Free(fetchstring);
    }
    
    
    return;
}

static void
secmoz_HandleInternalCertPolicyURL(URL_Struct *url, MWContext *cx,
				   CERTCertificate *cert)
{
    URL_Struct *fetchurl = NULL;
    char *fetchstring = NULL;
    char *urlstring = NULL;
    
    urlstring = CERT_FindCertURLExtension(cert,
					 SEC_OID_NS_CERT_EXT_CA_POLICY_URL,
					 SEC_OID_NS_CERT_EXT_CA_POLICY_URL);

    if ( urlstring == NULL ) {
	goto loser;
    }

    fetchstring = (char *)PORT_Alloc(PORT_Strlen(urlstring) + 1);
    if ( fetchstring == NULL ) {
	goto loser;
    }

    /* copy the base url data */
    PORT_Strcpy(fetchstring, urlstring);

    fetchurl = NET_CreateURLStruct(fetchstring, NET_DONT_RELOAD);
    
    StrAllocCopy(fetchurl->window_target, "_CA_POLICY");
    
    fetchurl->sec_private = (void *)cx;

    (void)FE_SetTimeout(fetchURL, (void *)fetchurl, 0);

    goto done;
    
loser:
    if ( fetchurl ) {
	NET_FreeURLStruct(fetchurl);
    }

done:
    if ( urlstring ) {
	PORT_Free(urlstring);
    }
    
    if ( fetchstring ) {
	PORT_Free(fetchstring);
    }

    return;
}

static void
secmoz_HandleInternalCertURL(URL_Struct *url, MWContext *cx)
{
    CERTCertificate *cert;
    
    cert = secmoz_URLToCert(url->address);
    if ( cert == NULL ) {
	return;
    }
    
    if(!strncasecomp(url->address,"internal-security-cert-revoke",29)) {
	secmoz_HandleInternalCertRevokeURL(url, cx, cert);
    } else if(!strncasecomp(url->address,"internal-security-cert-policy",29)) {
	secmoz_HandleInternalCertPolicyURL(url, cx, cert);
    }

    return;
}

void
SECNAV_HandleInternalSecURL(URL_Struct *url, MWContext *cx)
{
    if(!strncasecomp(url->address,"internal-security-cert-",23)) {
	secmoz_HandleInternalCertURL(url, cx);
    }
    /* XXX - what to do if no match? */
    
    return;
}

#define POLICY_START_STRING "<form action=\"internal-security-cert-policy:"
#define POLICY_MID_STRING "\" method=post><input type=submit value=\""
#define POLICY_END_STRING "\"></form>"
    
char *
SECMOZ_MakeCertPolicyButtonString(CERTCertificate *cert)
{
    char *ret;
    int len;
    char *keystring;
    char *urlstring;
    char *intlstring;

    /* make sure that the extension is there */
    urlstring = CERT_FindCertURLExtension(cert,
					 SEC_OID_NS_CERT_EXT_CA_POLICY_URL,
					 SEC_OID_NS_CERT_EXT_CA_POLICY_URL);
    if ( urlstring ) {
	/* extension is there, but we don't need the string right now */
	PORT_Free(urlstring);
    } else {
	/* extension isn't there, so give up */
	return(NULL);
    }
    
    /* make the keystring */
    keystring = secmoz_CertToKeyString(cert);
    
    if ( keystring == NULL ) {
	return NULL;
    }

    intlstring = XP_GetString(XP_VIEW_CERT_POLICY);
    
    len = PORT_Strlen(keystring) + PORT_Strlen(intlstring) +
	sizeof(POLICY_START_STRING) + sizeof(POLICY_MID_STRING) +
	sizeof(POLICY_END_STRING); 
    
    ret = (char *)PORT_Alloc(len);

    /* make the full url */
    if ( ret ) {
	PORT_Strcpy(ret, POLICY_START_STRING);
	XP_STRCAT(ret, keystring);
	XP_STRCAT(ret, POLICY_MID_STRING);
	XP_STRCAT(ret, intlstring);
	XP_STRCAT(ret, POLICY_END_STRING);
    }

    PORT_Free(keystring);
    return(ret);
}
#define REVOKE_START_STRING "<form action=\"internal-security-cert-revoke:"
#define REVOKE_MID_STRING "\" method=post><input type=submit value=\""
#define REVOKE_END_STRING "\"></form>"
    
char *
SECMOZ_MakeCertRevokeButtonString(CERTCertificate *cert)
{
    char *ret;
    int len;
    char *keystring;
    char *urlstring;
    char *intlstring;
    
    /* make sure that the extension is there */
    urlstring = CERT_FindCertURLExtension(cert,
					 SEC_OID_NS_CERT_EXT_REVOCATION_URL,
					 SEC_OID_NS_CERT_EXT_CA_REVOCATION_URL);
    if ( urlstring ) {
	/* extension is there, but we don't need the string right now */
	PORT_Free(urlstring);
    } else {
	/* extension isn't there, so give up */
	return(NULL);
    }
    
    /* make the keystring */
    keystring = secmoz_CertToKeyString(cert);
    
    if ( keystring == NULL ) {
	return NULL;
    }

    intlstring = XP_GetString(XP_CHECK_CERT_STATUS);
    
    len = PORT_Strlen(keystring) + PORT_Strlen(intlstring) +
	sizeof(REVOKE_START_STRING) + sizeof(REVOKE_MID_STRING) +
	sizeof(REVOKE_END_STRING); 
    
    ret = (char *)PORT_Alloc(len);

    /* make the full url */
    if ( ret ) {
	PORT_Strcpy(ret, REVOKE_START_STRING);
	XP_STRCAT(ret, keystring);
	XP_STRCAT(ret, REVOKE_MID_STRING);
	XP_STRCAT(ret, intlstring);
	XP_STRCAT(ret, REVOKE_END_STRING);
    }

    PORT_Free(keystring);
    return(ret);
}

char *
SECNAV_MakeCertButtonString(CERTCertificate *cert)
{
    char *str = NULL;
    char *extstring;

    extstring = SECMOZ_MakeCertRevokeButtonString(cert);
    if ( extstring ) {
	StrAllocCat(str, extstring);
	PORT_Free(extstring);
    }
    extstring = SECMOZ_MakeCertPolicyButtonString(cert);
    if ( extstring ) {
	if ( str ) {
	    StrAllocCat(str, "</td><td>");
	}
	
	StrAllocCat(str, extstring);
	PORT_Free(extstring);
    }

    if ( str ) {
	extstring = NULL;
	StrAllocCat(extstring, "<table><tr><td>");
	StrAllocCat(extstring, str);
	StrAllocCat(extstring, "</td></tr></table>");
	PORT_Free(str);
    }

    return(extstring);
}

/* ----------------------------------------------------------------------
** stuff for cipher preferences dialogs
*/
static SECStatus
enable_SSL_cipher_prefs(void)
{
    SECNAVPolicyFind *snpf;
    long  protocolID;

    snpf = SECNAV_FindInit(CIPHER_FAMILY_SSL, PR_FALSE, PR_FALSE);
    if (!snpf)
    	return SECFailure;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
    	int pref = SECNAV_AlgPreferred(protocolID);
	SSL_EnableCipher(protocolID, pref);
    }

    SECNAV_FindEnd(snpf);

    return SECSuccess;
}

static SECStatus
enable_SSL_cipher_policies(void)
{
    SECNAVPolicyFind *snpf;
    long  protocolID;

    snpf = SECNAV_FindInit(CIPHER_FAMILY_SSL, PR_FALSE, PR_FALSE);
    if (!snpf)
    	return SECFailure;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
    	int allowed = SECNAV_AlgAllowed(protocolID);
	SSL_SetPolicy(protocolID, allowed);
    }
    SECNAV_FindEnd(snpf);

    return SECSuccess;
}

static SECStatus
enable_SMIME_cipher_prefs(void)
{
    SECNAVPolicyFind *snpf;
    long  protocolID;

    snpf = SECNAV_FindInit(CIPHER_FAMILY_SMIME, PR_FALSE, PR_FALSE);
    if (!snpf)
    	return SECFailure;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
    	int pref = SECNAV_AlgPreferred(protocolID);
	SECMIME_EnableCipher(protocolID, pref);
    }

    SECNAV_FindEnd(snpf);

    /* now tell secmime that we've sent it the last preference */
    SECMIME_EnableCipher(CIPHER_FAMILYID_MASK, 0);

    return SECSuccess;
}

static SECStatus
enable_SMIME_cipher_policies(void)
{
    SECNAVPolicyFind *snpf;
    long  protocolID;

    snpf = SECNAV_FindInit(CIPHER_FAMILY_SMIME, PR_FALSE, PR_FALSE);
    if (!snpf)
    	return SECFailure;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
    	int allowed = SECNAV_AlgAllowed(protocolID);
	SECMIME_SetPolicy(protocolID, allowed);
    }
    SECNAV_FindEnd(snpf);

    return SECSuccess;
}

static SECStatus
enable_PKCS12_cipher_policies(void)
{
    SECNAVPolicyFind *snpf;
    long  protocolID;

    snpf = SECNAV_FindInit(CIPHER_FAMILY_PKCS12, PR_FALSE, PR_FALSE);
    if (!snpf)
    	return SECFailure;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
    	int allowed = SECNAV_AlgAllowed(protocolID);
	SEC_PKCS12EnableCipher(protocolID, allowed);
    }
    SECNAV_FindEnd(snpf);

    return SECSuccess;
}


PRBool 
ciphers_dialog_handler(XPDialogState *dialog_state, char **argv,
			      int argc, unsigned int button)
{
    SECNAVPolicyFind *snpf;
    char *	family;
    long  	protocolID;
    int 	count = 0;

    if (button != XP_DIALOG_OK_BUTTON) 
    	goto loser;

    switch ((int)dialog_state->arg) {
    case SSL_LIBRARY_VERSION_2: 	family = CIPHER_FAMILY_SSL2;	break;
    case SSL_LIBRARY_VERSION_3_0:	family = CIPHER_FAMILY_SSL3;	break;
    case SMIME_LIBRARY_VERSION_1_0:	family = CIPHER_FAMILY_SMIME;	break;
    default:				goto loser;
    }

    /* get only the ciphers permitted by policy */
    snpf = SECNAV_FindInit(family, PR_TRUE, PR_FALSE);
    if (!snpf)
    	goto loser;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
	char * value;
	const char * protoName;
	PRBool boolVal;

	protoName = SECNAV_GetProtoName(protocolID);
	value = XP_FindValueInArgs((char *)protoName, argv, argc);
	/* if the checkbox was checked, then value will "on".
	 * if the checkbox was unchecked, then value will be NULL, not "off".
	 */
	boolVal = ((value != NULL) && !PORT_Strcasecmp(value, "on"));
	SECNAV_PrefBoolUpdate(protocolID, boolVal);
	++count;
    	
    }
    SECNAV_FindEnd(snpf);

    if (count) {
	PREF_SavePrefFile();
	if (((int)dialog_state->arg) == SMIME_LIBRARY_VERSION_1_0)
	    enable_SMIME_cipher_prefs();
	else
	    enable_SSL_cipher_prefs();
    }

loser:
    return PR_FALSE;	/* tells caller not to leave window on screen */
}

static XPDialogInfo ciphersDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    ciphers_dialog_handler,
    550,
    400,
};

void
SECNAV_MakeCiphersDialog(void *proto_win, int version)
{
    XPDialogStrings *	strings;
    char *		family;
    char *		toggles		= NULL;
    const char *	conditional 	= NULL;
    SECNAVPolicyFind *	snpf;
    long  		protocolID;

    switch (version) {
    case SSL_LIBRARY_VERSION_2:
	strings = XP_GetDialogStrings(XP_CONFIG_SSL2_CIPHERS_STRINGS);
     	family = CIPHER_FAMILY_SSL2;	
	break;
    case SSL_LIBRARY_VERSION_3_0:
	strings = XP_GetDialogStrings(XP_CONFIG_SSL3_CIPHERS_STRINGS);
    	family = CIPHER_FAMILY_SSL3;
	break;
    case SMIME_LIBRARY_VERSION_1_0:
	strings = XP_GetDialogStrings(XP_CONFIG_SMIME_CIPHERS_STRINGS);
    	family = CIPHER_FAMILY_SMIME;
	break;
    default:
	goto loser;
    }

    if (strings == NULL) 
	return;

    XP_CopyDialogString(strings, 0, "");	/* "" hack, sorry */

    /* get only the ciphers permitted by policy */
    snpf = SECNAV_FindInit(family, PR_TRUE, PR_FALSE);
    if (!snpf)
    	goto loser;

    while (0 <= (protocolID = SECNAV_FindNext(snpf))) {
    	PRBool allowed = SECNAV_AlgAllowed(protocolID);

	StrAllocCat(toggles, "<li><input type=checkbox name=");
	StrAllocCat(toggles, SECNAV_GetProtoName(protocolID));
	if (SECNAV_AlgPreferred(protocolID)) {
	    StrAllocCat(toggles, " checked");
	}
	StrAllocCat(toggles, "> ");
	StrAllocCat(toggles, SECNAV_GetProtoDesc(protocolID));
	if (allowed != PR_TRUE && allowed != PR_FALSE) {
	    if (conditional == NULL)
	    	conditional = XP_GetString(XP_CONFIG_MAYBE_CIPHERS_STRINGS);
	    if (conditional != NULL)
		StrAllocCat(toggles, conditional);
	}
    }
    SECNAV_FindEnd(snpf);

    if (toggles) {
	XP_CopyDialogString(strings, 1, toggles);
	PORT_Free(toggles);
    } else {
	/* there were no Ciphers permitted by Policy. */
	conditional = XP_GetString(XP_CONFIG_NO_CIPHERS_STRINGS);
	if (conditional != NULL)
	    XP_CopyDialogString(strings, 1, conditional);
    }
    XP_MakeHTMLDialog(proto_win, &ciphersDialog, XP_CONFCIPHERS_TITLE_STRING,
		      strings, (void *)version);
loser:
    XP_FreeDialogStrings(strings);

    return;
}

/* --------------------------------------------------------------------- */

extern int SA_ASK_SA_EVERY_TIME_LABEL;
extern int SA_SELECT_AUTO_LABEL;

CERTCertNicknames *
SECNAV_GetDefUserCertList(CERTCertDBHandle *handle,void *wincx)
{
    CERTCertNicknames *names;
    
    names = CERT_GetCertNicknames(handle, SEC_CERT_NICKNAMES_USER,wincx);

    if (names) {
	char *askUser, *pickAuto;
	char **newnames;
	
	askUser = XP_GetString(SA_ASK_SA_EVERY_TIME_LABEL);
	pickAuto = XP_GetString(SA_SELECT_AUTO_LABEL);

	newnames = PORT_ArenaGrow(names->arena, names->nicknames,
				  names->numnicknames * sizeof(char *),
				  names->numnicknames * sizeof(char *) + 2);
	if (newnames) {
	    /* Move first two certs to the end of the list, making
	       sure to avoid a UMR in case numnicknames is 1. */
	    names->nicknames = newnames;
	    if (names->numnicknames > 1) {
		names->nicknames[names->numnicknames] =
		    names->nicknames[0];
		names->nicknames[names->numnicknames + 1] =
		    names->nicknames[1];
	    } else if (names->numnicknames == 1) {
		names->nicknames[2] = names->nicknames[0];
	    }
	    names->nicknames[0] = askUser;
	    names->nicknames[1] = pickAuto;
	    names->numnicknames += 2;
	    names->totallen += PORT_Strlen(askUser) + PORT_Strlen(pickAuto);
	} else {
	    CERT_FreeNicknames(names);
	    names = NULL;
	}
    }

    return (names);
}

SECStatus
SECNAV_SetDefUserCertList(char *chosen)
{
    char *askUser, *pickAuto;

    askUser = XP_GetString(SA_ASK_SA_EVERY_TIME_LABEL);
    pickAuto = XP_GetString(SA_SELECT_AUTO_LABEL);

    if (UserCertChoice.chosen)
	PORT_Free(UserCertChoice.chosen);

    if ( chosen == NULL ) {
	UserCertChoice.chosen = NULL;
	UserCertChoice.mode = ASK;
    } else {
	UserCertChoice.chosen = PORT_Strdup(chosen);
	
	if (PORT_Strcmp(chosen, askUser) == 0) {
	    UserCertChoice.mode = ASK;
	} else if (PORT_Strcmp(chosen, pickAuto) == 0) {
	    UserCertChoice.mode = AUTO;
	} else {
	    UserCertChoice.mode = STR;
	}
    }
    return (PR_TRUE);
}

PRBool
SECNAV_SecurityDialog(MWContext *context, int state)
{
    XP_Bool value, *valueptr, savevalue;
    int ret;
    char *prefname = NULL;
    PRBool rv;
    
    valueptr = &value;
    
    switch (state) {
      case SD_INSECURE_POST_FROM_INSECURE_DOC:
	prefname = "security.warn_submit_insecure";
	break;
      case SD_ENTERING_SECURE_SPACE:
	prefname = "security.warn_entering_secure";
	break;
      case SD_LEAVING_SECURE_SPACE:
	prefname = "security.warn_leaving_secure";
	break;
      case SD_INSECURE_DOCS_WITHIN_SECURE_DOCS_NOT_SHOWN:
	prefname = "security.viewing_mixed";
	break;
      case SD_INSECURE_POST_FROM_SECURE_DOC:
      case SD_REDIRECTION_TO_INSECURE_DOC:
      case SD_REDIRECTION_TO_SECURE_SITE:
	valueptr = 0;
	break;
    }

    if ( prefname ) {
	ret = PREF_GetBoolPref(prefname, &value);
	savevalue = value;

	if ( !value ) {
	    return(PR_TRUE);
	}
    }
    
    ret = (int)FE_SecurityDialog(context, state
#ifndef AKBAR
				 , valueptr
#endif /* AKBAR */
				 );
    rv = (PRBool)ret;
    
    if ( prefname && ( value != savevalue ) ) {
	ret = PREF_SetBoolPref(prefname, value);
	ret = PREF_SavePrefFile();
    }

    return(rv);
}

void SECNAV_Logout(void) {
    PK11_LogoutAll();
    SSL_ClearSessionCache();
}

/* additions for importing and exporting PKCS 12 files */
struct secnav_ExportFileInfo{
    char *filename;	/* export filename */
    XP_File file_ptr;	/* pointer to file */
    PRBool error;	/* error occurred? */
    SECItem *data;	/* info to write */
};

static void
secnav_DestroyExportFileInfo(struct secnav_ExportFileInfo **exp_ptr)
{
    if((*exp_ptr)->data != NULL) {
	SECITEM_ZfreeItem((*exp_ptr)->data, PR_FALSE);
    }

    if((*exp_ptr)->filename != NULL) {
	PORT_Free((*exp_ptr)->filename);
    }

    if((*exp_ptr)->file_ptr != NULL) {
	XP_FileClose((*exp_ptr)->file_ptr);
    }

    PORT_Free(*exp_ptr);
}

static PRBool 
secnav_OpenExportFile(MWContext *ctxt,
		      struct secnav_ExportFileInfo *exp_info, 
		      PRBool file_read)
{
    if(exp_info == NULL)
	return PR_FALSE;

    if(file_read == PR_TRUE) {
#ifdef XP_MAC   
	/* XXX We are using xpMailFolder here because on the MAC, xpTemporary
	 * modifies the filename, while xpMailFolder does not.  This is
	 * a hack to accomplish the desired functionality and should be
	 * replaced with something cleaner later.
	 */
    	exp_info->file_ptr = XP_FileOpen(exp_info->filename, 
    					 xpMailFolder,
    					 XP_FILE_READ_BIN); 
#else   
    	exp_info->file_ptr = XP_FileOpen(exp_info->filename, 
    					 xpTemporary,
    					 XP_FILE_READ_BIN);
#endif    					 
    } else {
    	exp_info->file_ptr = XP_FileOpen(exp_info->filename,
    					 xpPKCS12File,
    					 XP_FILE_WRITE_BIN);
    }

    if(!exp_info->file_ptr) {
	exp_info->error = PR_TRUE;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return PR_FALSE;
    }

    return PR_TRUE;
}

static PRBool 
secnav_WriteExportFile(MWContext *ctxt,
		       struct secnav_ExportFileInfo *exp_info)
{
    int write_len;

    write_len = XP_FileWrite(exp_info->data->data, 
    			     (int32)exp_info->data->len,
    			     exp_info->file_ptr);
    XP_FileClose(exp_info->file_ptr);
    exp_info->file_ptr = NULL;
    
    if(write_len != (int)exp_info->data->len) {
	/* error occured -- remove file */
	XP_FileRemove(exp_info->filename, xpPKCS12File);
	PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_WRITE);
	exp_info->error = PR_TRUE;
	return PR_FALSE;
    }

    exp_info->error = PR_FALSE;
    return PR_TRUE;
}
    
static void
simple_copy(MWContext *context,
	    char *newFile,
	    void *closure)
{
    if(newFile == NULL) {
	return;
    }

    PORT_Strcpy((char *)closure, newFile);

    return;
}

PRBool
secnav_ReadExportFile(MWContext *ctxt,
		      struct secnav_ExportFileInfo *exp_info)
{
    XP_StatStruct exp_stat;
    PRBool ret_val = PR_TRUE;
    int read_len;
    char testBuf[2];

    /* get information about the file */
#ifdef XP_MAC
    if(!XP_Stat(exp_info->filename, &exp_stat, xpMailFolder)) { 
#else   
    if(!XP_Stat(exp_info->filename, &exp_stat, xpTemporary)) {
#endif

	/* rewind the file */
	XP_FileSeek(exp_info->file_ptr, 0L, SEEK_SET);

	/* is the file at least 2 bytes -- even an empty PFX file
	 * will contain a header byte and a length byte 
	 */
	if(exp_stat.st_size >= 2) {
	    XP_FileRead(testBuf, 2, exp_info->file_ptr);

	    /* check that the first two bytes are appropriate for the
	     * size and type of the file -- filter out people trying
	     * to import NETSCAPE.exe
	     */
	    if(SEC_PKCS12ValidData(testBuf, 2, (long int)exp_stat.st_size)) {
		/* reset the file pointer to the begining, allocate space 
		 * and read in the file 
		 */
		XP_FileSeek(exp_info->file_ptr, 0L, SEEK_SET);
		exp_info->data = (SECItem *)PORT_Alloc(sizeof(SECItem));
		if(exp_info->data != NULL) {
		    exp_info->data->data = 
			(unsigned char *)PORT_Alloc(exp_stat.st_size);
 		    if(exp_info->data->data != NULL) {
			exp_info->data->len = exp_stat.st_size;
			read_len = XP_FileRead(exp_info->data->data, 
				       (int32)exp_info->data->len, 
				       exp_info->file_ptr);

	    		if(read_len != (int)exp_info->data->len) {
		 	    PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_READ);
	        	    ret_val = PR_FALSE;
			}
	    	    }
		} else {
		    PORT_SetError(SEC_ERROR_PKCS12_DECODING_PFX);
		    ret_val = PR_FALSE;
		}
	    } else {
		PORT_SetError(SEC_ERROR_PKCS12_DECODING_PFX);
		ret_val = PR_FALSE;
	    }
	} else { 
	    PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_READ);
	    ret_val = PR_FALSE;
	}
    }
	            
    XP_FileClose(exp_info->file_ptr);
    exp_info->file_ptr = NULL;

    if(ret_val == PR_FALSE) {
	exp_info->error = PR_TRUE;
    } else {
	exp_info->error = PR_FALSE;
    }

    return ret_val;
}
    
SECItem * 
SECNAV_ReadExportFile(void *ctxt)
{
    PRBool ret_val = PR_FALSE;
    struct secnav_ExportFileInfo *exp_info = NULL;
    SECItem *ret_item = NULL;
    int fe_ret = 0;

    exp_info = PORT_ZAlloc(sizeof(struct secnav_ExportFileInfo));
    if(exp_info == NULL)
	goto loser;

    exp_info->data = NULL;
    exp_info->error = PR_FALSE;
    exp_info->filename = (char *)PORT_ZAlloc(2048);  /* XXX different length? */
    if(exp_info->filename == NULL) {
	goto loser;
    }

    fe_ret = FE_PromptForFileName(ctxt,
    			 	  XP_GetString(XP_PKCS12_IMPORT_FILE_PROMPT),
    			 	  "*.p12",	/* filename */
    			 	  PR_TRUE,	/* file need not exist */
    			 	  FALSE,	/* do not allow directories */
    			 	  simple_copy,	/* callback */
    			 	  (void *)exp_info->filename);	/* closure */

    /* properly check for cancel.  On UNIX/Win32 fe_ret is negative
     * when cancel is pressed.  On the MAC, it is not, in all cases
     * a zero length string indicates cancel 
     */
    if((fe_ret >= 0) && (PORT_Strlen(exp_info->filename) > 0)) {
	secnav_OpenExportFile((MWContext *)ctxt, exp_info, PR_TRUE);
	if(exp_info->error == PR_FALSE) {
	    ret_val = secnav_ReadExportFile((MWContext *)ctxt, exp_info);
	    if(exp_info->error == PR_FALSE) {
		ret_item = SECITEM_DupItem(exp_info->data);
		if(ret_item == NULL) {
		    ret_val = PR_FALSE;
		    exp_info->error = PR_TRUE;
		}
	    }
	}
	else {
	    ret_val = PR_FALSE;
	}
    } else {
        ret_val = PR_FALSE;
    }


    /* let stuff fall through */
loser:
    if((ret_val == PR_FALSE) || (fe_ret < 0)) {
	if((fe_ret < 0) || (PORT_Strlen(exp_info->filename) == 0)) {
	    PORT_SetError(SEC_ERROR_USER_CANCELLED);
	}
	if(ret_item != NULL) {
	    SECITEM_ZfreeItem(ret_item, PR_TRUE);
	    ret_item = NULL;
	}
    }

    secnav_DestroyExportFileInfo(&exp_info);

    return ret_item;
}

PRBool
SECNAV_WriteExportFile(void *ctxt, SECItem *data)
{
    PRBool ret_val = PR_FALSE;
    struct secnav_ExportFileInfo *exp_info = NULL;
    int fe_ret;

    if(data == NULL)
	return PR_FALSE;

    exp_info = PORT_ZAlloc(sizeof(struct secnav_ExportFileInfo));
    if(exp_info == NULL)
	goto loser;

    exp_info->data = data;
    exp_info->error = PR_FALSE;
    exp_info->filename = (char *)PORT_ZAlloc(2048);  /* XXX different length? */
    if(exp_info->filename == NULL) {
	goto loser;
    }

    fe_ret = FE_PromptForFileName(ctxt,
    				  XP_GetString(XP_PKCS12_EXPORT_FILE_PROMPT),
    				  "*.p12",		/* filename */
    				  FALSE,	/* file need not exist */
    				  FALSE,	/* do not allow directories */
    				  simple_copy,	/* callback */
    				  (void *)exp_info->filename);	/* closure */
    			
    /* properly catch a cancel press.  on mac fe_ret is non-negative
     * when cancel is pressed.  on unix/win32 it is positive.  so,
     * we check for a 0 length filename as well as checking fe_ret.
     */
    if((fe_ret >= 0) && (PORT_Strlen(exp_info->filename) > 0)) {
	secnav_OpenExportFile((MWContext *)ctxt, exp_info, PR_FALSE);
	if(exp_info->error == PR_FALSE) {
	    ret_val = secnav_WriteExportFile((MWContext *)ctxt, exp_info);
	}
	else {
	    ret_val = PR_FALSE;
	}
    } else {
	PORT_SetError(SEC_ERROR_USER_CANCELLED);
        ret_val = PR_FALSE;
    }


    /* let stuff fall through */
loser:
    secnav_DestroyExportFileInfo(&exp_info);

    return ret_val;
}

SECItem *SECNAV_GetExportPassword(void *arg)
{
    char *pw, *confirm_pw;
    MWContext *cx;
    SECItem *pwitem;
    SECNAVPKCS12PwdPromptArg *pwd_arg;
    int prompt_val;

    if(arg == NULL) {
	return NULL;
    } 

    pwd_arg = (SECNAVPKCS12PwdPromptArg *)arg;
    cx = (MWContext *)pwd_arg->wincx;

    switch(pwd_arg->prompt_type) {
	case pwdGenericExport:
	    prompt_val = XP_SEC_ENTER_EXPORT_PWD;
	    break;
	case pwdGenericImport:
	    prompt_val = XP_SEC_ENTER_IMPORT_PWD;
	    break;
	default:
	    /* handle other prompts when they are needed... */
	    return NULL;
    }

    while(1)
    {
	/* get the password */
	pw = FE_PromptPassword(cx, XP_GetString(prompt_val));
	if(pw == NULL) {
	    PORT_SetError(SEC_ERROR_USER_CANCELLED);
	    goto loser;
	}

	/* on export, we want to reconfirm passwords */
	if(pwd_arg->prompt_type == pwdGenericExport) {
	    confirm_pw = FE_PromptPassword(cx, 	
			XP_GetString(XP_SEC_REENTER_TO_CONFIRM_PWD));
	    if(confirm_pw == NULL) {
		PORT_Free(pw);
		PORT_SetError(SEC_ERROR_USER_CANCELLED);
		goto loser;
	    }

	    if(PORT_Strcmp(pw, confirm_pw)) {
		prompt_val = XP_SEC_BAD_CONFIRM_EXPORT_PWD;
		PORT_Free(confirm_pw);
		PORT_Free(pw);
		continue;
	    } else {
		PORT_Free(confirm_pw);
	    }
	}
		
	pwitem = (SECItem *)PORT_Alloc(sizeof(SECItem));
	if(pwitem == NULL)
	{
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    SECNAV_ZeroPassword(pw);
	    PORT_Free(pw);
	    goto loser;
	}

	pwitem->data = PORT_Alloc(PORT_Strlen(pw) + 1);
	if(pwitem != NULL)
	{
	    PORT_Memcpy(pwitem->data, pw, PORT_Strlen(pw));
	    pwitem->len = PORT_Strlen(pw);
	    pwitem->data[PORT_Strlen(pw)] = 0;
	}
	else
	{
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    SECNAV_ZeroPassword(pw);
	    PORT_Free(pw);
	    PORT_Free(pwitem);
	    goto loser;
	}

	return pwitem;
    }

    PORT_Assert(0);	/* should never get here */

loser:
    return NULL;
}

SECItem *
SECNAV_NicknameCollisionCallback(SECItem *old_nick,
				 PRBool *cancel,
				 void *wincx)
{
    char *nick = NULL;
    SECItem *ret_nick = NULL;
    MWContext *cx;

    if((old_nick == NULL) || (cancel == NULL)){
	return NULL;
    }

    cx = wincx;
    nick = FE_Prompt(cx, 
		     XP_GetString(XP_SEC_PROMPT_NICKNAME_COLLISION),
		     (char *)old_nick->data);
    if(nick == NULL) {
	*cancel = PR_TRUE;
	return NULL;
    }

    if(PORT_Strcmp((char *)old_nick->data, nick)) {
	ret_nick = (SECItem *)PORT_ZAlloc(sizeof(SECItem));
	if(ret_nick == NULL) {
	    PORT_Free(nick);
	    return NULL;
	} else {
	    ret_nick->data = (unsigned char *)nick;
	    ret_nick->len = PORT_Strlen(nick);
	}
    } else {
	PORT_Free(nick);
	return NULL;
    }

    return ret_nick;
}
/* Returns a string, a comma separated list of the country codes in all
 * the "user certs".  Or returns NULL if no user certs exists.
 */
char * 
SECNAV_GetUserCertCountries(void)
{
    return NULL;	/* XXX FIX ME - jsw, please fill this in. */
}

static void 
secnav_DoPKCS12ImportErrors(void *proto_win)
{
    int error_value;

    error_value = PORT_GetError();
    if((error_value == SEC_ERROR_PKCS12_DECODING_PFX) ||
	(error_value == SEC_ERROR_PKCS12_INVALID_MAC) ||
	(error_value == SEC_ERROR_PKCS12_UNSUPPORTED_MAC_ALGORITHM) ||
	(error_value == SEC_ERROR_PKCS12_UNSUPPORTED_TRANSPORT_MODE) ||
	(error_value == SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE) ||
	(error_value == SEC_ERROR_PKCS12_UNSUPPORTED_PBE_ALGORITHM) ||
	(error_value == SEC_ERROR_PKCS12_UNSUPPORTED_VERSION) || 
	(error_value == SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT) ||
	(error_value == SEC_ERROR_PKCS12_CERT_COLLISION) ||
	(error_value == SEC_ERROR_PKCS12_DUPLICATE_DATA) ||
	(error_value == SEC_ERROR_BAD_EXPORT_ALGORITHM) ||
	(error_value == SEC_ERROR_PKCS12_UNABLE_TO_READ)) {
	FE_Alert(proto_win, XP_GetString(error_value));
    } else if(error_value == SEC_ERROR_USER_CANCELLED) {
	;
    } else {
	FE_Alert(proto_win, XP_GetString(SEC_ERROR_IMPORTING_CERTIFICATES));
    }
}
   
typedef struct {
    void *state;
    void *dlgstate;
} PKCS12RedrawCallbackArg;

typedef void (*refreshCallback)(void *);

SECStatus 
SECNAV_ImportPKCS12Object(void *arg, PRBool cancel)
{
    SECNAVPKCS12ImportArg *import_arg = NULL;
    PKCS12RedrawCallbackArg *cbarg = NULL;
    SECItem *pfx = NULL;
    SECStatus rv;
    refreshCallback callback; 
    PK11SlotInfo *slot = NULL;

    if(arg == NULL) {
	return SECFailure;
    }

    import_arg = (SECNAVPKCS12ImportArg *)arg;
    slot = (PK11SlotInfo *)import_arg->slot_ptr;

    if(cancel == PR_TRUE) {
	PORT_SetError(SEC_ERROR_USER_CANCELLED);
	goto loser;
    }

    if(PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
	SECStatus rv = PK11_DoPassword(slot, import_arg->proto_win);
	if(rv != SECSuccess) {
	    goto loser;
	}
    }

    cbarg = (PKCS12RedrawCallbackArg *)PORT_Alloc(sizeof(PKCS12RedrawCallbackArg));
    if(cbarg) {
	cbarg->dlgstate = import_arg->dlg_state;
	cbarg->state = import_arg->sa_state;

	pfx = SECNAV_ReadExportFile((void *)import_arg->proto_win);
	if(pfx != NULL) {
	    SECNAVPKCS12PwdPromptArg pwd_arg;
	    pwd_arg.prompt_type = pwdGenericImport;
	    pwd_arg.wincx = import_arg->proto_win;

	    rv = SEC_PKCS12PutPFX(pfx,
	    			  SECNAV_GetExportPassword,
	    			  (void *)&pwd_arg,
	    			  SECNAV_NicknameCollisionCallback,
	    			  (PK11SlotInfo *)import_arg->slot_ptr,
	    			  import_arg->proto_win);
	    SECITEM_ZfreeItem(pfx, PR_TRUE);
	    if(rv != SECSuccess) {
		secnav_DoPKCS12ImportErrors(import_arg->proto_win);
		goto loser;
	    } else {
		callback = (refreshCallback)import_arg->redrawCallback;
		(*callback)(cbarg);
		FE_Alert(import_arg->proto_win, 
			 XP_GetString(XP_PKCS12_SUCCESSFUL_IMPORT));
		cbarg = NULL;
		rv = SECSuccess;
	    }
	} else {
	    secnav_DoPKCS12ImportErrors(import_arg->proto_win);
	    rv = SECFailure;
	}
    } else {
	rv = SECFailure;
    }

    if(cbarg != NULL) {
	PORT_Free(cbarg);
    }

    PK11_FreeSlot((PK11SlotInfo *)import_arg->slot_ptr);

    PORT_Free(import_arg);
    return rv;

loser:
    if(cbarg != NULL) {
	PORT_Free(cbarg);
    }

    if(import_arg != NULL) {
	if(import_arg->slot_ptr != NULL) {
	    PK11_FreeSlot((PK11SlotInfo *)import_arg->slot_ptr);
	}
	PORT_Free(import_arg);
    }
    return SECFailure;
}

static void 
secnav_DoPKCS12ExportErrors(void *proto_win)
{
    int error_value;

    error_value = PORT_GetError();
    if((error_value == SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY) ||
	(error_value == SEC_ERROR_PKCS12_UNABLE_TO_LOCATE_OBJECT_BY_NAME) ||
	(error_value == SEC_ERROR_PKCS12_UNABLE_TO_WRITE)) {
	FE_Alert(proto_win, XP_GetString(error_value));
    } else if(error_value == SEC_ERROR_USER_CANCELLED) {
	;
    } else {
	FE_Alert(proto_win, XP_GetString(SEC_ERROR_EXPORTING_CERTIFICATES));
    }
}

void
SECNAV_ExportPKCS12Object(CERTCertificate *cert, void *proto_win,
			  void *arg)
{
    char *cert_nick[2];
    CERTCertificate *certs[2];
    PK11SlotInfo *slot;
    SECItem *pfx;
    SECNAVPKCS12PwdPromptArg pwd_arg;

    if((cert == NULL) || (arg == NULL)) {
	/* XXX set error ? */
	return;
    }

    cert_nick[0] = (char *)arg;
    cert_nick[1] = NULL;
    certs[0] = cert;
    certs[1] = NULL;
    pwd_arg.prompt_type = pwdGenericExport;
    pwd_arg.wincx = proto_win;

    if(cert->slot == NULL) {
	slot = PK11_GetInternalKeySlot();
    } else {
	slot = cert->slot;
    }

    if(slot && PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
	SECStatus rv = PK11_DoPassword(slot, proto_win);
	PK11_FreeSlot(slot);
	if(rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_USER_CANCELLED);
	    goto process_errors;
	}
    }

    pfx = SEC_PKCS12GetPFX(cert_nick, certs, 
    			   PR_TRUE, /* shroud keys */
    			   SECNAV_GetExportPassword,
    			   (void *)&pwd_arg,
    			   SEC_OID_PKCS12_OFFLINE_TRANSPORT_MODE,
    			   proto_win);
    if(pfx != NULL) {
	PRBool write_success;
	write_success = SECNAV_WriteExportFile(proto_win, pfx);
	if(write_success == PR_TRUE) {
	    FE_Alert(proto_win, XP_GetString(XP_PKCS12_SUCCESSFUL_EXPORT));
	} else {
	    secnav_DoPKCS12ExportErrors(proto_win);
	}
	goto done;
    }

process_errors:
    secnav_DoPKCS12ExportErrors(proto_win);

done:
    return;
}
