#ifndef _CERT_H_
#define _CERT_H_
/*
 * cert.h - public data structures and prototypes for the certificate library
 *
 * $Id: cert.h,v 1.9.2.11 1997/05/24 00:23:00 jwz Exp $
 */

#include "prarena.h"
#include "mcom_db.h"
#include "prlong.h"
#include "prlog.h"

#include "seccomon.h"
#include "secdert.h"
#include "secoidt.h"
#include "cryptot.h"
#include "keyt.h"
#include "certt.h"

SEC_BEGIN_PROTOS
   
/****************************************************************************
 *
 * RFC1485 ascii to/from X.??? RelativeDistinguishedName (aka CERTName)
 *
 ****************************************************************************/

/*
** Convert an ascii RFC1485 encoded name into its CERTName equivalent.
*/
extern CERTName *CERT_AsciiToName(char *string);

/*
** Convert an CERTName into its RFC1485 encoded equivalent.
*/
extern char *CERT_NameToAscii(CERTName *name);

extern CERTAVA *CERT_CopyAVA(PRArenaPool *arena, CERTAVA *src);

/*
** Examine an AVA and return the tag that refers to it. The AVA tags are
** defined as SEC_OID_AVA*.
*/
extern SECOidTag CERT_GetAVATag(CERTAVA *ava);

/*
** Compare two AVA's, returning the difference between them.
*/
extern SECComparison CERT_CompareAVA(CERTAVA *a, CERTAVA *b);

/*
** Create an RDN (relative-distinguished-name). The argument list is a
** NULL terminated list of AVA's.
*/
extern CERTRDN *CERT_CreateRDN(PRArenaPool *arena, CERTAVA *avas, ...);

/*
** Make a copy of "src" storing it in "dest".
*/
extern SECStatus CERT_CopyRDN(PRArenaPool *arena, CERTRDN *dest, CERTRDN *src);

/*
** Destory an RDN object.
**	"rdn" the RDN to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void CERT_DestroyRDN(CERTRDN *rdn, PRBool freeit);

/*
** Add an AVA to an RDN.
**	"rdn" the RDN to add to
**	"ava" the AVA to add
*/
extern SECStatus CERT_AddAVA(PRArenaPool *arena, CERTRDN *rdn, CERTAVA *ava);

/*
** Compare two RDN's, returning the difference between them.
*/
extern SECComparison CERT_CompareRDN(CERTRDN *a, CERTRDN *b);

/*
** Create an X.500 style name using a NULL terminated list of RDN's.
*/
extern CERTName *CERT_CreateName(CERTRDN *rdn, ...);

/*
** Make a copy of "src" storing it in "dest". Memory is allocated in
** "dest" for each of the appropriate sub objects. Memory is not freed in
** "dest" before allocation is done (use CERT_DestroyName(dest, PR_FALSE) to
** do that).
*/
extern SECStatus CERT_CopyName(PRArenaPool *arena, CERTName *dest, CERTName *src);

/*
** Destroy a Name object.
**	"name" the CERTName to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void CERT_DestroyName(CERTName *name);

/*
** Add an RDN to a name.
**	"name" the name to add the RDN to
**	"rdn" the RDN to add to name
*/
extern SECStatus CERT_AddRDN(CERTName *name, CERTRDN *rdn);

/*
** Compare two names, returning the difference between them.
*/
extern SECComparison CERT_CompareName(CERTName *a, CERTName *b);

/*
** Convert a CERTName into something readable
*/
extern char *CERT_FormatName (CERTName *name);

/*
** Convert a der-encoded integer to a hex printable string form.
** Perhaps this should be a SEC function but it's only used for certs.
*/
extern char *CERT_Hexify (SECItem *i, int do_colon);

/**************************************************************************************
 *
 * Certificate handling operations
 *
 **************************************************************************************/

/*
** Create a new validity object given two unix time values.
**	"notBefore" the time before which the validity is not valid
**	"notAfter" the time after which the validity is not valid
*/
extern CERTValidity *CERT_CreateValidity(int64 notBefore, int64 notAfter);

/*
** Destroy a validity object.
**	"v" the validity to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void CERT_DestroyValidity(CERTValidity *v);

/*
** Copy the "src" object to "dest". Memory is allocated in "dest" for
** each of the appropriate sub-objects. Memory in "dest" is not freed
** before memory is allocated (use CERT_DestroyValidity(v, PR_FALSE) to do
** that).
*/
extern SECStatus CERT_CopyValidity
   (PRArenaPool *arena, CERTValidity *dest, CERTValidity *src);

/*
** Create a new certificate object. The result must be wrapped with an
** CERTSignedData to create a signed certificate.
**	"serialNumber" the serial number
**	"issuer" the name of the certificate issuer
**	"validity" the validity period of the certificate
**	"req" the certificate request that prompted the certificate issuance
*/
extern CERTCertificate *
CERT_CreateCertificate (unsigned long serialNumber, CERTName *issuer,
			CERTValidity *validity, CERTCertificateRequest *req);

/*
** Destroy a certificate object
**	"cert" the certificate to destroy
** NOTE: certificate's are reference counted. This call decrements the
** reference count, and if the result is zero, then the object is destroyed
** and optionally freed.
*/
extern void CERT_DestroyCertificate(CERTCertificate *cert);

/*
** Make a shallow copy of a certificate "c". Just increments the
** reference count on "c".
*/
extern CERTCertificate *CERT_DupCertificate(CERTCertificate *c);

/*
** Create a new certificate request. This result must be wrapped with an
** CERTSignedData to create a signed certificate request.
**	"name" the subject name (who the certificate request is from)
**	"spki" describes/defines the public key the certificate is for
**	"attributes" if non-zero, some optional attribute data
*/
extern CERTCertificateRequest *
CERT_CreateCertificateRequest (CERTName *name, CERTSubjectPublicKeyInfo *spki,
			       SECItem *attributes);

/*
** Destroy a certificate-request object
**	"r" the certificate-request to destroy
**	"freeit" if PR_TRUE then free the object as well as its sub-objects
*/
extern void CERT_DestroyCertificateRequest(CERTCertificateRequest *r);

/*
** Extract a public key object from a SubjectPublicKeyInfo
*/
extern SECKEYPublicKey *CERT_ExtractPublicKey(CERTSubjectPublicKeyInfo *spki);

/*
** Initialize the certificate database.  This is called to create
**  the initial list of certificates in the database.
*/
extern SECStatus CERT_InitCertDB(CERTCertDBHandle *handle);

/*
** Default certificate database routines
*/
extern void CERT_SetDefaultCertDB(CERTCertDBHandle *handle);

extern CERTCertDBHandle *CERT_GetDefaultCertDB(void);

/************************************************************************************
 *
 * X.500 Name handling operations
 *
 ************************************************************************************/

/*
** Create an AVA (attribute-value-assertion)
**	"arena" the memory arena to alloc from
**	"kind" is one of SEC_OID_AVA_*
**	"valueType" is one of DER_PRINTABLE_STRING, DER_IA5_STRING, or
**	   DER_T61_STRING
**	"value" is the null terminated string containing the value
*/
extern CERTAVA *CERT_CreateAVA
   (PRArenaPool *arena, SECOidTag kind, int valueType, char *value);

/*
** Extract the Distinguished Name from a DER encoded certificate
**	"derCert" is the DER encoded certificate
**	"derName" is the SECItem that the name is returned in
*/
extern SECStatus CERT_NameFromDERCert(SECItem *derCert, SECItem *derName);

/*
** Generate a database search key for a certificate, based on the
** issuer and serial number.
**	"arena" the memory arena to alloc from
**	"derCert" the DER encoded certificate
**	"key" the returned key
*/
extern SECStatus CERT_KeyFromDERCert(PRArenaPool *arena, SECItem *derCert, SECItem *key);

extern SECStatus CERT_KeyFromIssuerAndSN(PRArenaPool *arena, SECItem *issuer,
					 SECItem *sn, SECItem *key);

/*
** Generate a database search key for a crl, based on the
** issuer.
**	"arena" the memory arena to alloc from
**	"derCrl" the DER encoded crl
**	"key" the returned key
*/
extern SECStatus CERT_KeyFromDERCrl(PRArenaPool *arena, SECItem *derCrl, SECItem *key);

/*
** Open the certificate database.  Use callback to get name of database.
*/
extern SECStatus CERT_OpenCertDB(CERTCertDBHandle *handle, PRBool readonly,
				 CERTDBNameFunc namecb, void *cbarg);

/* Open the certificate database.  Use given filename for database. */
extern SECStatus CERT_OpenCertDBFilename(CERTCertDBHandle *handle,
					 char *certdbname, PRBool readonly);

/*
** Open and initialize a cert database that is entirely in memory.  This
** can be used when the permanent database can not be opened or created.
*/
extern SECStatus CERT_OpenVolatileCertDB(CERTCertDBHandle *handle);

/*
** Check the hostname to make sure that it matches the shexp that
** is given in the common name of the certificate.
*/
extern SECStatus CERT_VerifyCertName(CERTCertificate *cert, char *hostname);

/*
** Decode a DER encoded certificate into an CERTCertificate structure
**	"derSignedCert" is the DER encoded signed certificate
**	"copyDER" is true if the DER should be copied, false if the
**		existing copy should be referenced
**	"nickname" is the nickname to use in the database.  If it is NULL
**		then a temporary nickname is generated.
*/
extern CERTCertificate *
CERT_DecodeDERCertificate (SECItem *derSignedCert, PRBool copyDER, char *nickname);
/*
** Decode a DER encoded CRL/KRL into an CERTSignedCrl structure
**	"derSignedCrl" is the DER encoded signed crl/krl.
**	"type" is this a CRL or KRL.
*/
#define SEC_CRL_TYPE	1
#define SEC_KRL_TYPE	0

extern CERTSignedCrl *
CERT_DecodeDERCrl (PRArenaPool *arena, SECItem *derSignedCrl,int type);

/* Validate CRL then import it to the dbase.  If there is already a CRL with the
 * same CA in the dbase, it will be replaced if derCRL is more up to date.  
 * If the process successes, a CRL will be returned.  Otherwise, a NULL will 
 * be returned. The caller should call PORT_GetError() for the exactly error 
 * code.
 */
extern CERTSignedCrl *
CERT_ImportCRL (CERTCertDBHandle *handle, SECItem *derCRL, char *url, 
						int type, void * wincx);

extern void CERT_DestroyCrl (CERTSignedCrl *crl);

/*
** Decode a certificate and put it into the temporary certificate database
*/
extern CERTCertificate *
CERT_NewTempCertificate (CERTCertDBHandle *handle, SECItem *derCert,
			 char *nickname, PRBool isperm, PRBool copyDER);

/*
** Add a certificate to the temporary database.
**	"dbCert" is the certificate from the perm database.
**	"isperm" indicates if the cert is in the permanent database.
*/
extern CERTCertificate *
CERT_AddTempCertificate (CERTCertDBHandle *handle, certDBEntryCert *entry,
			 PRBool isperm);

/*
** Add a temporary certificate to the permanent database.
** 	"cert" is the temporary cert
**	"nickname" is the permanent nickname to use
**	"trust" is the certificate trust parameters to assign to the cert
*/
extern SECStatus
CERT_AddTempCertToPerm (CERTCertificate *cert, char *nickname, CERTCertTrust *trust);

/*
** Find a certificate in the database
**	"key" is the database key to look for
*/
extern CERTCertificate *CERT_FindCertByKey(CERTCertDBHandle *handle, SECItem *key);

/*
** Find a certificate in the database by name
**	"name" is the distinguished name to look up
*/
extern CERTCertificate *
CERT_FindCertByName (CERTCertDBHandle *handle, SECItem *name);

/*
** Find a certificate in the database by name
**	"name" is the distinguished name to look up (in ascii)
*/
extern CERTCertificate *
CERT_FindCertByNameString (CERTCertDBHandle *handle, char *name);

/*
** Find a certificate in the database by name and keyid
**	"name" is the distinguished name to look up
**	"keyID" is the value of the subjectKeyID to match
*/
extern CERTCertificate *
CERT_FindCertByKeyID (CERTCertDBHandle *handle, SECItem *name, SECItem *keyID);

/*
** Generate a certificate key from the issuer and serialnumber, then look it
** up in the database.  Return the cert if found.
**	"issuerAndSN" is the issuer and serial number to look for
*/
extern CERTCertificate *
CERT_FindCertByIssuerAndSN (CERTCertDBHandle *handle, CERTIssuerAndSN *issuerAndSN);

/*
** Find a certificate in the database by a nickname
**	"nickname" is the ascii string nickname to look for
*/
extern CERTCertificate *
CERT_FindCertByNickname (CERTCertDBHandle *handle, char *nickname);
/*
** Find a certificate in the database by a DER encoded certificate
**	"derCert" is the DER encoded certificate
*/
extern CERTCertificate *
CERT_FindCertByDERCert(CERTCertDBHandle *handle, SECItem *derCert);

/*
** Find a certificate in the database by a email address
**	"emailAddr" is the email address to look up
*/
CERTCertificate *
CERT_FindCertByEmailAddr(CERTCertDBHandle *handle, char *emailAddr);

/*
** Find a certificate in the database by a email address or nickname
**	"name" is the email address or nickname to look up
*/
CERTCertificate *
CERT_FindCertByNicknameOrEmailAddr(CERTCertDBHandle *handle, char *name);

/*
 * Find the issuer of a cert
 */
CERTCertificate *
CERT_FindCertIssuer(CERTCertificate *cert);

/*
** Delete a certificate from the temporary database
**	"cert" is the certificate to be deleted
*/
extern SECStatus CERT_DeleteTempCertificate(CERTCertificate *cert);

/*
** Flush and close the permanent database.
*/
extern void CERT_ClosePermCertDB(CERTCertDBHandle *handle);

/*
** Check the validity times of a certificate vs. time 't', allowing
** some slop for broken clocks and stuff.
**	"cert" is the certificate to be checked
**	"t" is the time to check against
*/
extern SECCertTimeValidity CERT_CheckCertValidTimes(CERTCertificate *cert,
								    int64 t);

/*
** WARNING - this function is depricated, and will either go away or have
**		a new API in the near future.
**
** Check the validity times of a certificate vs. the current time, allowing
** some slop for broken clocks and stuff.
**	"cert" is the certificate to be checked
*/
extern SECStatus CERT_CertTimesValid(CERTCertificate *cert);

/*
** Extract the validity times from a certificate
**	"c" is the certificate
**	"notBefore" is the start of the validity period
**	"notAfter" is the end of the validity period
*/
extern SECStatus
CERT_GetCertTimes (CERTCertificate *c, int64 *notBefore, int64 *notAfter);

/*
** Extract the issuer and serial number from a certificate
*/
extern CERTIssuerAndSN *CERT_GetCertIssuerAndSN(PRArenaPool *, 
							CERTCertificate *);

/*
** verify the signature of a signed data object with a given certificate
**	"sd" the signed data object to be verified
**	"cert" the certificate to use to check the signature
*/
extern SECStatus CERT_VerifySignedData(CERTSignedData *sd,
				       CERTCertificate *cert,
				       int64 t,
				       void *wincx);

/*
** verify a certificate by checking validity times against a certain time,
** that we trust the issuer, and that the signature on the certificate is
** valid.
**	"cert" the certificate to verify
**	"checkSig" only check signatures if true
*/
extern SECStatus
CERT_VerifyCert(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertUsage certUsage, int64 t,
		void *wincx, CERTVerifyLog *log);

/* same as above, but uses current time */
extern SECStatus
CERT_VerifyCertNow(CERTCertDBHandle *handle, CERTCertificate *cert,
		   PRBool checkSig, SECCertUsage certUsage, void *wincx);

/*
** This must only be called on a cert that is known to have an issuer
** with an invalid time
*/
extern CERTCertificate *
CERT_FindExpiredIssuer (CERTCertDBHandle *handle, CERTCertificate *cert);

/*
** Read a base64 ascii encoded DER certificate and convert it to our
** internal format.
**	"certstr" is a null-terminated string containing the certificate
*/
extern CERTCertificate *CERT_ConvertAndDecodeCertificate(char *certstr);

/*
** Read a certificate in some foreign format, and convert it to our
** internal format.
**	"certbuf" is the buffer containing the certificate
**	"certlen" is the length of the buffer
** NOTE - currently supports netscape base64 ascii encoded raw certs
**  and netscape binary DER typed files.
*/
extern CERTCertificate *CERT_DecodeCertFromPackage(char *certbuf, int certlen);

extern SECStatus
CERT_ImportCAChain (SECItem *certs, int numcerts, SECCertUsage certUsage);

/*
** Read a certificate chain in some foreign format, and pass it to a 
** callback function.
**	"certbuf" is the buffer containing the certificate
**	"certlen" is the length of the buffer
**	"f" is the callback function
**	"arg" is the callback argument
*/
typedef SECStatus (*CERTImportCertificateFunc)
   (void *arg, SECItem **certs, int numcerts);

extern SECStatus
CERT_DecodeCertPackage(char *certbuf, int certlen, CERTImportCertificateFunc f,
		       void *arg);

/*
** Pretty print a certificate in HTML
**	"cert" is the certificate to print
**	"showImages" controls whether or not to use about:security URLs
**		for subject and issuer images.  This should only be true
**		in the navigator.
*/
extern char *CERT_HTMLCertInfo(CERTCertificate *cert, PRBool showImages);

/*
** extract various element strings from a distinguished name.
**	"name" the distinguished name
*/
extern char *CERT_GetCommonName(CERTName *name);

extern char *CERT_GetCertEmailAddress(CERTName *name);

extern char *CERT_GetCommonName(CERTName *name);

extern char *CERT_GetCountryName(CERTName *name);

extern char *CERT_GetLocalityName(CERTName *name);

extern char *CERT_GetStateName(CERTName *name);

extern char *CERT_GetOrgName(CERTName *name);

extern char *CERT_GetOrgUnitName(CERTName *name);

extern char *CERT_GetCertUid(CERTName *name);

/* manipulate the trust parameters of a certificate */

extern SECStatus CERT_GetCertTrust(CERTCertificate *cert, CERTCertTrust *trust);

extern SECStatus
CERT_ChangeCertTrust (CERTCertDBHandle *handle, CERTCertificate *cert,
		      CERTCertTrust *trust);

extern SECStatus
CERT_ChangeCertTrustByUsage(CERTCertDBHandle *certdb, CERTCertificate *cert,
			    SECCertUsage usage);

/*************************************************************************
 *
 * manipulate the extensions of a certificate
 *
 ************************************************************************/

/*
** Set up a cert for adding X509v3 extensions.  Returns an opaque handle
** used by the next two routines.
**	"cert" is the certificate we are adding extensions to
*/
extern void *CERT_StartCertExtensions(CERTCertificate *cert);

/*
** Add an extension to a certificate.
**	"exthandle" is the handle returned by the previous function
**	"idtag" is the integer tag for the OID that should ID this extension
**	"value" is the value of the extension
**	"critical" is the critical extension flag
**	"copyData" is a flag indicating whether the value data should be
**		copied.
*/
extern SECStatus CERT_AddExtension (void *exthandle, int idtag, 
			SECItem *value, PRBool critical, PRBool copyData);

extern SECStatus CERT_AddExtensionByOID (void *exthandle, SECItem *oid,
			 SECItem *value, PRBool critical, PRBool copyData);

extern SECStatus CERT_EncodeAndAddExtension
   (void *exthandle, int idtag, SECItem *value, PRBool critical, int dertype);

extern SECStatus CERT_EncodeAndAddBitStrExtension
   (void *exthandle, int idtag, SECItem *value, PRBool critical);

/*
** Finish adding cert extensions.  Does final processing on extension
** data, putting it in the right format, and freeing any temporary
** storage.
**	"exthandle" is the handle used to add extensions to a certificate
*/
extern SECStatus CERT_FinishExtensions(void *exthandle);


/* If the extension is found, return its criticality and value.
** This allocate storage for the returning extension value.
*/
extern SECStatus CERT_GetExtenCriticality
   (CERTCertExtension **extensions, int tag, PRBool *isCritical);

/****************************************************************************
 *
 * DER encode and decode extension values
 *
 ****************************************************************************/

/* Encode the value of the basicConstraint extension.
**	arena - where to allocate memory for the encoded value.
**	value - extension value to encode
**	encodedValue - output encoded value
*/
extern SECStatus CERT_EncodeBasicConstraintValue
   (PRArenaPool *arena, CERTBasicConstraints *value, SECItem *encodedValue);

/*
** Encode the value of the authorityKeyIdentifier extension.
*/
extern SECStatus CERT_EncodeAuthKeyID
   (PRArenaPool *arena, CERTAuthKeyID *value, SECItem *encodedValue);

/*
** Encode the value of the crlDistributionPoints extension.
*/
extern SECStatus CERT_EncodeCRLDistributionPoints
   (PRArenaPool *arena, CERTCrlDistributionPoints *value,SECItem *derValue);

/*
** Decodes a DER encoded basicConstaint extension value into a readable format
**	value - decoded value
**	encodedValue - value to decoded
*/
extern SECStatus CERT_DecodeBasicConstraintValue
   (CERTBasicConstraints *value, SECItem *encodedValue);

/* Decodes a DER encoded authorityKeyIdentifier extension value into a
** readable format.
**	arena - where to allocate memory for the decoded value
**	encodedValue - value to be decoded
**	Returns a CERTAuthKeyID structure which contains the decoded value
*/
extern CERTAuthKeyID *CERT_DecodeAuthKeyID 
			(PRArenaPool *arena, SECItem *encodedValue);


/* Decodes a DER encoded crlDistributionPoints extension value into a 
** readable format.
**	arena - where to allocate memory for the decoded value
**	der - value to be decoded
**	Returns a CERTCrlDistributionPoints structure which contains the 
**          decoded value
*/
extern CERTCrlDistributionPoints * CERT_DecodeCRLDistributionPoints
   (PRArenaPool *arena, SECItem *der);

/* Extract certain name type from a generalName */
extern void *CERT_GetGeneralNameByType
   (CERTGeneralName **genNames, CERTGeneralNameType type, PRBool derFormat);

/****************************************************************************
 *
 * Find extension values of a certificate 
 *
 ***************************************************************************/

extern SECStatus CERT_FindCertExtension
   (CERTCertificate *cert, int tag, SECItem *value);

extern SECStatus CERT_FindNSCertTypeExtension
   (CERTCertificate *cert, SECItem *value);

extern char * CERT_FindNSStringExtension (CERTCertificate *cert, int oidtag);

extern SECStatus CERT_FindIssuerCertExtension
   (CERTCertificate *cert, int tag, SECItem *value);

extern SECStatus CERT_FindCertExtensionByOID
   (CERTCertificate *cert, SECItem *oid, SECItem *value);

extern char *CERT_FindCertURLExtension (CERTCertificate *cert, int tag, 
								int catag);

/* Returns the decoded value of the authKeyID extension.
**   Note that this use the arena in cert to allocate storage for the result
*/
extern CERTAuthKeyID * CERT_FindAuthKeyIDExten (CERTCertificate *cert);

/* Returns the decoded value of the basicConstraint extension.
 */
extern SECStatus CERT_FindBasicConstraintExten
   (CERTCertificate *cert, CERTBasicConstraints *value);

/* Returns the decoded value of the crlDistributionPoints extension.
**  Note that the arena in cert is used to allocate storage for the result
*/
extern CERTCrlDistributionPoints * CERT_FindCRLDistributionPoints
   (CERTCertificate *cert);

/* Returns value of the keyUsage extension.  This uses PR_Alloc to allocate 
** buffer for the decoded value, The caller should free up the storage 
** allocated in value->data.
*/
extern SECStatus CERT_FindKeyUsageExtension (CERTCertificate *cert, 
							SECItem *value);

/* Return the decoded value of the subjectKeyID extension. The caller should 
** free up the storage allocated in retItem->data.
*/
extern SECStatus CERT_FindSubjectKeyIDExten (CERTCertificate *cert, 
							   SECItem *retItem);

/*
** If cert is a v3 certificate, and a critical keyUsage extension is included,
** then check the usage against the extension value.  If a non-critical 
** keyUsage extension is included, this will return SECSuccess without 
** checking, since the extension is an advisory field, not a restriction.  
** If cert is not a v3 certificate, this will return SECSuccess.
**	cert - certificate
**	usage - one of the x.509 v3 the Key Usage Extension flags
*/
extern SECStatus CERT_CheckCertUsage (CERTCertificate *cert, 
							unsigned char usage);

/****************************************************************************
 *
 *  CRL v2 Extensions supported routines
 *
 ****************************************************************************/

extern SECStatus CERT_FindCRLExtensionByOID
   (CERTCrl *crl, SECItem *oid, SECItem *value);

extern SECStatus CERT_FindCRLExtension
   (CERTCrl *crl, int tag, SECItem *value);

extern SECStatus
   CERT_FindInvalidDateExten (CERTCrl *crl, int64 *value);

extern void *CERT_StartCRLExtensions (CERTCrl *crl);

extern CERTCertNicknames *CERT_GetCertNicknames (CERTCertDBHandle *handle,
						 int what, void *wincx);

/*
** Finds the crlNumber extension and decodes its value into 'value'
*/
extern SECStatus CERT_FindCRLNumberExten (CERTCrl *crl, CERTCrlNumber *value);

extern void CERT_FreeNicknames(CERTCertNicknames *nicknames);

extern PRBool CERT_CompareCerts(CERTCertificate *c1, CERTCertificate *c2);

extern PRBool CERT_CompareCertsForRedirection(CERTCertificate *c1,
							 CERTCertificate *c2);

/*
** Generate an array of the Distinguished Names that the given cert database
** "trusts"
*/
extern CERTDistNames *CERT_GetSSLCACerts(CERTCertDBHandle *handle);

extern void CERT_FreeDistNames(CERTDistNames *names);

/*
** Generate an array of Distinguished names from an array of nicknames
*/
extern CERTDistNames *CERT_DistNamesFromNicknames
   (CERTCertDBHandle *handle, char **nicknames, int nnames);

/*
** Generate a certificate chain from a certificate.
*/
extern CERTCertificateList *CERT_CertChainFromCert
   (CERTCertDBHandle *handle, CERTCertificate *cert);

extern void CERT_DestroyCertificateList(CERTCertificateList *list);

/* is cert a newer than cert b? */
PRBool CERT_IsNewer(CERTCertificate *certa, CERTCertificate *certb);

typedef SECStatus (* CERTCertCallback)(CERTCertificate *cert, void *arg);

SECStatus
CERT_TraversePermCertsForSubject(CERTCertDBHandle *handle, SECItem *derSubject,
				 CERTCertCallback cb, void *cbarg);
int
CERT_NumPermCertsForSubject(CERTCertDBHandle *handle, SECItem *derSubject);

SECStatus
CERT_TraversePermCertsForNickname(CERTCertDBHandle *handle, char *nickname,
				  CERTCertCallback cb, void *cbarg);

int
CERT_NumPermCertsForNickname(CERTCertDBHandle *handle, char *nickname);

int
CERT_NumCertsForCertSubject(CERTCertificate *cert);

int
CERT_NumPermCertsForCertSubject(CERTCertificate *cert);

SECStatus
CERT_TraverseCertsForSubject(CERTCertDBHandle *handle,
			     CERTSubjectList *subjectList,
			     CERTCertCallback cb, void *cbarg);

/* currently a stub for address book */
PRBool
CERT_IsCertRevoked(CERTCertificate *cert);

void
CERT_DestroyCertArray(CERTCertificate **certs, unsigned int ncerts);

/* convert an email address to lower case */
char *CERT_FixupEmailAddr(char *emailAddr);

/* decode string representation of trust flags into trust struct */
SECStatus
CERT_DecodeTrustString(CERTCertTrust *trust, char *trusts);

/* encode trust struct into string representation of trust flags */
char *
CERT_EncodeTrustString(CERTCertTrust *trust);

/* find the next or prev cert in a subject list */
CERTCertificate *
CERT_PrevSubjectCert(CERTCertificate *cert);
CERTCertificate *
CERT_NextSubjectCert(CERTCertificate *cert);

/*
 * import a collection of certs into the temporary or permanent cert
 * database
 */
SECStatus
CERT_ImportCerts(CERTCertDBHandle *certdb, SECCertUsage usage,
		 unsigned int ncerts, SECItem **derCerts,
		 CERTCertificate ***retCerts, PRBool keepCerts,
		 PRBool caOnly, char *nickname);

SECStatus
CERT_SaveImportedCert(CERTCertificate *cert, SECCertUsage usage,
		      PRBool caOnly, char *nickname);

char *
CERT_MakeCANickname(CERTCertificate *cert);

PRBool
CERT_IsCACert(CERTCertificate *cert);

SECStatus
CERT_SaveSMimeProfile(CERTCertificate *cert, SECItem *emailProfile,
		      SECItem *profileTime);

/*
 * find the smime symmetric capabilities profile for a given cert
 */
SECItem *
CERT_FindSMimeProfile(CERTCertificate *cert);

int
CERT_GetDBContentVersion(CERTCertDBHandle *handle);

void
CERT_SetDBContentVersion(int version, CERTCertDBHandle *handle);

SECStatus
CERT_AddNewCerts(CERTCertDBHandle *handle);

CERTPackageType
CERT_CertPackageType(SECItem *package, SECItem *certitem);

CERTCertificatePolicies *
CERT_DecodeCertificatePoliciesExtension(SECItem *extnValue);

void
CERT_DestroyCertificatePoliciesExtension(CERTCertificatePolicies *policies);

CERTUserNotice *
CERT_DecodeUserNotice(SECItem *noticeItem);

void
CERT_DestroyUserNotice(CERTUserNotice *userNotice);

typedef char * (* CERTPolicyStringCallback)(char *org,
					       unsigned long noticeNumber,
					       void *arg);
void
CERT_SetCAPolicyStringCallback(CERTPolicyStringCallback cb, void *cbarg);

char *
CERT_GetCertCommentString(CERTCertificate *cert);

PRBool
CERT_GovtApprovedBitSet(CERTCertificate *cert);

SEC_END_PROTOS

#endif /* _CERT_H_ */
