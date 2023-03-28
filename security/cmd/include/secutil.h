#ifndef _SEC_UTIL_H_
#define _SEC_UTIL_H_

#include "base64.h"
#include "cert.h"
#include "crypto.h"
#include "sechash.h"
#include "key.h"
#include "secpkcs7.h"
#include "secasn1.h"
#include "rsa.h"
#include "seccomon.h"
#include "secder.h"
#include "secitem.h"
#include "secoid.h"
#include "secport.h"
#include "secrng.h"
#include "ssl.h"
#include "sslerr.h"
#include "sslproto.h"

#include "secnew.h"
#include "certdb.h"

#include "xp_core.h"
#include "xp_str.h"
#include "xp_mcom.h"
#include "xp_file.h"
#include "prglobal.h"

#define SEC_CT_PRIVATE_KEY		"private-key"
#define SEC_CT_PUBLIC_KEY		"public-key"
#define SEC_CT_CERTIFICATE		"certificate"
#define SEC_CT_CERTIFICATE_REQUEST	"certificate-request"
#define SEC_CT_PKCS7			"pkcs7"
#define SEC_CT_CRL			"crl"

#define NS_CERTREQ_HEADER "-----BEGIN NEW CERTIFICATE REQUEST-----"
#define NS_CERTREQ_TRAILER "-----END NEW CERTIFICATE REQUEST-----"

#define NS_CERT_HEADER "-----BEGIN CERTIFICATE-----"
#define NS_CERT_TRAILER "-----END CERTIFICATE-----"

/* From libsec/pcertdb.c --- it's not declared in sec.h */
extern SECStatus SEC_AddPermCertificate(CERTCertDBHandle *handle,
		SECItem *derCert, char *nickname, CERTCertTrust *trust);


typedef int (*SECU_PPFunc)(FILE *out, SECItem *item, char *msg, int level);


/*  These were stolen from the old sec.h... */
/*
** Check a password for legitimacy. Passwords must be at least 8
** characters long and contain one non-alphabetic. Return DSTrue if the
** password is ok, DSFalse otherwise.
*/
extern PRBool SEC_CheckPassword(unsigned char *password);

/*
** Blind check of a password. Complement to SEC_CheckPassword which 
** ignores length and content type, just retuning DSTrue is the password
** exists, DSFalse if NULL
*/
extern PRBool SEC_BlindCheckPassword(unsigned char *password);

/*
** Get a password.
** First prompt with "msg" on "out", then read the password from "in".
** The password is then checked using "chkpw".
*/
extern unsigned char *SEC_GetPassword(FILE *in, FILE *out, char *msg,
				      PRBool (*chkpw)(unsigned char *));

char *SECU_GetPasswordString(void *arg, char *prompt);

/*
** Write a dongle password.
** Uses MD5 to hash constant system data (hostname, etc.), and then
** creates RC4 key to encrypt a password "pw" into a file "fd".
*/
extern SECStatus SEC_WriteDongleFile(int fd, unsigned char *pw);

/*
** Get a dongle password.
** Uses MD5 to hash constant system data (hostname, etc.), and then
** creates RC4 key to decrypt and return a password from file "fd".
*/
extern unsigned char *SEC_ReadDongleFile(int fd);


/* End stolen headers */


/* Change the key db password in the database */
extern SECStatus SECU_ChangeKeyDBPassword(SECKEYKeyDBHandle *kdbh);

/* Check if a key name exists. Return PR_TRUE if true, PR_FALSE if not */
extern PRBool SECU_CheckKeyNameExists(SECKEYKeyDBHandle *handle, char *nickname);

/* Find a key by a nickname. Calls SECKEY_FindKeyByName */
extern SECKEYLowPrivateKey *SECU_GetPrivateKey(SECKEYKeyDBHandle *kdbh, char *nickname);

/* Put a key into the database. Calls SECKEY_StoreKeyByName */
extern SECStatus SECU_PutPrivateKey(SECKEYKeyDBHandle *kdbh, char *nickname,
				   SECKEYLowPrivateKey *key);

/* Get key encrypted with dongle file in "pathname" */
extern SECKEYLowPrivateKey *SECU_GetPrivateDongleKey(SECKEYKeyDBHandle *handle, 
                                               char *nickname, char *pathname);

extern SECItem *SECU_GetPassword(void *arg, SECKEYKeyDBHandle *handle);

/* Just sticks the two strings together with a / if needed */
char *SECU_AppendFilenameToDir(char *dir, char *filename);

/* Returns result of getenv("SSL_DIR") or NULL */
extern char *SECU_DefaultSSLDir(void);

/*
** Should be called once during initialization to set the default 
**    directory for looking for cert.db, key.db, and cert-nameidx.db files
** Removes trailing '/' in 'base' 
** If 'base' is NULL, defaults to set to .netscape in home directory.
*/
extern char *SECU_ConfigDirectory(const char* base);

/*
** Depending on 'type' this will return the full pathname of requested
** database.
**	xpKeyDB 	: key.db
**	xpCertDB	: cert5.db (cert.db is the old, pre-3.0 db name)
**	xpCertDBNameIDX : Not used (cert-nameidx.db used in pre-3.0)
*/
extern char *SECU_DatabaseFileName(XP_FileType type);

extern char *SECU_CertDBNameCallback(void *arg, int dbVersion);
extern char *SECU_KeyDBNameCallback(void *arg, int dbVersion);

extern SECKEYPrivateKey *SECU_FindPrivateKeyFromNickname(char *name);
extern SECKEYLowPrivateKey *SECU_FindLowPrivateKeyFromNickname(char *name);
extern SECStatus SECU_DeleteKeyByName(SECKEYKeyDBHandle *handle, char *nickname);

extern SECKEYKeyDBHandle *SECU_OpenKeyDB(void);

/* 
** Basic callback function for SSL_GetClientAuthDataHook
*/
extern int
SECU_GetClientAuthData(void *arg, int fd, struct CERTDistNamesStr *caNames,
                      struct CERTCertificateStr **pRetCert,
                      struct SECKEYPrivateKeyStr **pRetKey);

/* print out an error message */
extern void SECU_PrintError(char *progName, char *msg, ...);

/* print out a system error message */
extern void SECU_PrintSystemError(char *progName, char *msg, ...);

/* Return informative error string */
extern char *SECU_ErrorString(int16 err);

/* Return informative error string. Does not call XP_GetString */
extern char *SECU_ErrorStringRaw(int16 err);

/* Read in DER data */
extern SECStatus SECU_DER_Read(SECItem *dst, FILE *src);

/* Print integer value and hex */
extern void SECU_PrintInteger(FILE *out, SECItem *i, char *m, int level);

/* Print SECItem as hex */
extern void SECU_PrintAsHex(FILE *out, SECItem *i, char *m, int level);

/* Print SECItem as UTC Time */
extern int SECU_PrintUTCTime(FILE *out, SECItem *t, char *m, int level);

/* Dump all key nicknames */
extern int SECU_PrintKeyNames(SECKEYKeyDBHandle *handle, FILE *out);

/* Dump all certificate nicknames in a database */
extern int SECU_PrintCertificateNames(CERTCertDBHandle *handle, FILE *out);

/* See if nickname already in database. Return 1 true, 0 false, -1 error */
int SECU_CheckCertNameExists(CERTCertDBHandle *handle, char *nickname);

/* Dump contents of cert req */
extern int SECU_PrintCertificateRequest(FILE *out, SECItem *der, char *m,
	int level);

/* Dump contents of certificate */
extern int SECU_PrintCertificate(FILE *out, SECItem *der, char *m, int level);

/* print trust flags on a cert */
extern void SECU_PrintTrustFlags(FILE *out, CERTCertTrust *trust, char *m, int level);

/* Dump contents of public key */
extern int SECU_PrintPublicKey(FILE *out, SECItem *der, char *m, int level);

/* Dump contents of private key */
extern int SECU_PrintPrivateKey(FILE *out, SECItem *der, char *m, int level);

/* Pretty-print any PKCS7 thing */
extern int SECU_PrintPKCS7ContentInfo(FILE *out, SECItem *der, char *m, 
				      int level);

/* Init PKCS11 stuff */
extern SECStatus SECU_PKCS11Init(void);

/* Dump contents of signed data */
extern int SECU_PrintSignedData(FILE *out, SECItem *der, char *m, int level,
				SECU_PPFunc inner);

/* Return a malloc'ed string for a file */
extern char *SECU_FileToString(FILE *fp);

extern int SECU_PrintCrl(FILE *out, SECItem *der, char *m, int level);

extern void
SECU_PrintCRLInfo(FILE *out, CERTCrl *crl, char *m, int level);

/* Convert a High public Key to a Low public Key */
extern SECKEYLowPublicKey *SECU_ConvHighToLow(SECKEYPublicKey *pubHighKey);

extern SECItem *SECU_GetPBEPassword(void *arg);

/* error codes */
extern int SEC_ERROR_BAD_DATA;
extern int SEC_ERROR_BAD_DATABASE;
extern int SEC_ERROR_BAD_DER;
extern int SEC_ERROR_BAD_KEY;
extern int SEC_ERROR_BAD_PASSWORD;
extern int SEC_ERROR_BAD_SIGNATURE;
extern int SEC_ERROR_CA_CERT_INVALID;
extern int SEC_ERROR_CERT_NOT_VALID;
extern int SEC_ERROR_CERT_USAGES_INVALID;
extern int SEC_ERROR_EXTENSION_NOT_FOUND;
extern int SEC_ERROR_EXTENSION_VALUE_INVALID;
extern int SEC_ERROR_EXPIRED_CERTIFICATE;
extern int SEC_ERROR_INPUT_LEN;
extern int SEC_ERROR_INVALID_ALGORITHM;
extern int SEC_ERROR_INVALID_ARGS;
extern int SEC_ERROR_INVALID_AVA;
extern int SEC_ERROR_INVALID_TIME;
extern int SEC_ERROR_IO;
extern int SEC_ERROR_LIBRARY_FAILURE;
extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_OUTPUT_LEN;
extern int SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID;
extern int SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION;
extern int SEC_ERROR_UNKNOWN_ISSUER;
extern int SEC_ERROR_UNTRUSTED_CERT;
extern int SEC_ERROR_UNTRUSTED_ISSUER;

extern int SSL_ERROR_BAD_CERTIFICATE;
extern int SSL_ERROR_BAD_CLIENT;
extern int SSL_ERROR_BAD_SERVER;
extern int SSL_ERROR_EXPORT_ONLY_SERVER;
extern int SSL_ERROR_NO_CERTIFICATE;
extern int SSL_ERROR_NO_CYPHER_OVERLAP;
extern int SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE;
extern int SSL_ERROR_UNSUPPORTED_VERSION;
extern int SSL_ERROR_US_ONLY_SERVER;
extern int SEC_ERROR_EXTENSION_VALUE_INVALID;
extern int SEC_ERROR_OLD_CRL;

extern int XP_ERRNO_EIO;

#endif /* _SEC_UTIL_H_ */
