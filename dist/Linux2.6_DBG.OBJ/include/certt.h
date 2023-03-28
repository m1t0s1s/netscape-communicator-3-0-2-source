#ifndef _CERTT_H_
#define _CERTT_H_
/*
 * certt.h - public data structures for the certificate library
 *
 * $Id: certt.h,v 1.5.2.11 1997/05/24 00:23:03 jwz Exp $
 */
#include "pkcs11t.h"
#include "secmodt.h"

/* Non-opaque objects */
typedef struct CERTAVAStr CERTAVA;
typedef struct CERTCertificateListStr CERTCertificateList;
typedef struct CERTCertificateRequestStr CERTCertificateRequest;
typedef struct CERTCertificateStr CERTCertificate;
typedef struct CERTDERCertsStr CERTDERCerts;
typedef struct CERTCertExtensionStr CERTCertExtension;
typedef struct CERTCrlEntryStr CERTCrlEntry;
typedef struct CERTCrlStr CERTCrl;
typedef struct CERTCrlKeyStr CERTCrlKey;
typedef struct CERTSignedCrlStr CERTSignedCrl;
typedef struct CERTCrlHeadNodeStr CERTCrlHeadNode;
typedef struct CERTCrlNodeStr CERTCrlNode;
typedef struct CERTNameStr CERTName;
typedef struct CERTRDNStr CERTRDN;
typedef struct CERTSubjectPublicKeyInfoStr CERTSubjectPublicKeyInfo;
typedef struct CERTValidityStr CERTValidity;
typedef struct CERTCertKeyStr CERTCertKey;
typedef struct CERTCertTrustStr CERTCertTrust;
typedef struct CERTCertDBHandleStr CERTCertDBHandle;
typedef struct CERTCertNicknamesStr CERTCertNicknames;
typedef struct CERTDistNamesStr CERTDistNames;
typedef struct CERTPublicKeyAndChallengeStr CERTPublicKeyAndChallenge;
typedef struct CERTIssuerAndSNStr CERTIssuerAndSN;
typedef struct CERTSignedDataStr CERTSignedData;
typedef struct CERTBasicConstraintsStr CERTBasicConstraints;
typedef struct CERTGeneralNameStr CERTGeneralName;
typedef struct CERTAuthKeyIDStr CERTAuthKeyID;
typedef struct CRLDistributionPointStr CRLDistributionPoint;
typedef struct CERTCrlDistributionPointsStr CERTCrlDistributionPoints; 
typedef struct CERTSubjectNodeStr CERTSubjectNode;
typedef struct CERTSubjectListStr CERTSubjectList;
typedef struct CERTVerifyLogStr CERTVerifyLog;
typedef struct CERTVerifyLogNodeStr CERTVerifyLogNode;

/* CRL extensions type */
typedef unsigned long CERTCrlNumber;

/*
** An X.500 AVA object
*/
struct CERTAVAStr {
    SECItem type;
    SECItem value;
};

/*
** An X.500 RDN object
*/
struct CERTRDNStr {
    CERTAVA **avas;
};

/*
** An X.500 name object
*/
struct CERTNameStr {
    PRArenaPool *arena;
    CERTRDN **rdns;
};

/*
** An X.509 validity object
*/
struct CERTValidityStr {
    PRArenaPool *arena;
    SECItem notBefore;
    SECItem notAfter;
};

/*
 * A serial number and issuer name, which is used as a database key
 */
struct CERTCertKeyStr {
    SECItem serialNumber;
    SECItem derIssuer;
};

/*
** A signed data object. Used to implement the "signed" macro used
** in the X.500 specs.
*/
struct CERTSignedDataStr {
    SECItem data;
    SECAlgorithmID signatureAlgorithm;
    SECItem signature;
};

/*
** An X.509 subject-public-key-info object
*/
struct CERTSubjectPublicKeyInfoStr {
    PRArenaPool *arena;
    SECAlgorithmID algorithm;
    SECItem subjectPublicKey;
};

struct CERTPublicKeyAndChallengeStr {
    SECItem spki;
    SECItem challenge;
};

typedef struct _certDBEntryCert certDBEntryCert;
typedef struct _certDBEntryRevocation certDBEntryRevocation;

struct CERTCertTrustStr {
    unsigned int sslFlags;
    unsigned int emailFlags;
    unsigned int objectSigningFlags;
};

/*
 * defined the types of trust that exist
 */
typedef enum {
    trustSSL,
    trustEmail,
    trustObjectSigning
} SECTrustType;

#define SEC_GET_TRUST_FLAGS(trust,type) \
        (((type)==trustSSL)?((trust)->sslFlags): \
	 (((type)==trustEmail)?((trust)->emailFlags): \
	  (((type)==trustObjectSigning)?((trust)->objectSigningFlags):0)))

/*
** An X.509.3 certificate extension
*/
struct CERTCertExtensionStr {
    SECItem id;
    SECItem critical;
    SECItem value;
};

struct CERTSubjectNodeStr {
    struct CERTSubjectNodeStr *next;
    struct CERTSubjectNodeStr *prev;
    SECItem certKey;
    SECItem keyID;
};

struct CERTSubjectListStr {
    PRArenaPool *arena;
    int ncerts;
    char *emailAddr;
    CERTSubjectNode *head;
    CERTSubjectNode *tail; /* do we need tail? */
    struct _certDBEntrySubject *entry;
};

/*
** An X.509 certificate object (the unsigned form)
*/
struct CERTCertificateStr {
    PRArenaPool *arena;
    char *subjectName;
    char *issuerName;
    PRBool isperm;
    PRBool istemp;
    PRBool keepSession;			/* keep this cert for entire session*/
    PRBool domainOK;			/* is the bad domain ok? */
    PRBool timeOK;			/* is the bad validity time ok? */
    certDBEntryCert *dbEntry;		/* database entry struct */
    CERTSignedData signatureWrap;	/* XXX */
    SECItem derCert;			/* original DER for the cert */
    SECItem derIssuer;			/* DER for issuer name */
    SECItem derSubject;			/* DER for subject name */
    SECItem derPublicKey;		/* DER for the public key */
    SECItem certKey;			/* database key for this cert */
    SECItem version;
    SECItem serialNumber;
    SECAlgorithmID signature;
    CERTName issuer;
    CERTValidity validity;
    CERTName subject;
    CERTSubjectPublicKeyInfo subjectPublicKeyInfo;
    SECItem issuerID;
    SECItem subjectID;
    CERTCertExtension **extensions;
    char *nickname;
    char *dbnickname;
    char *emailAddr;
    CERTCertTrust *trust;
    int referenceCount;
    CERTCertDBHandle *dbhandle;
    SECItem subjectKeyID;	/* x509v3 subject key identifier */
    PRBool keyIDGenerated;	/* was the keyid generated? */
    unsigned int keyUsage;	/* what uses are allowed for this cert */
    unsigned int rawKeyUsage;	/* value of the key usage extension */
    PRBool keyUsagePresent;	/* was the key usage extension present */
    unsigned int nsCertType;	/* value of the ns cert type extension */
    CERTSubjectList *subjectList;
    struct SECSocketNode *socketlist;
    int socketcount;
    struct SECSocketNode *authsocketlist;
    int authsocketcount;
    PK11SlotInfo *slot;		/*if this cert came of a token, which is it*/
    CK_OBJECT_HANDLE pkcs11ID;	/*and which object on that token is it */
};
#define SEC_CERTIFICATE_VERSION_1		0	/* default created */
#define SEC_CERTIFICATE_VERSION_2		1	/* v2 */
#define SEC_CERTIFICATE_VERSION_3		2	/* v3 extensions */

#define SEC_CRL_VERSION_1		0	/* default */
#define SEC_CRL_VERSION_2		1	/* v2 extensions */

/*
 * used to identify class of cert in mime stream code
 */
#define SEC_CERT_CLASS_CA	1
#define SEC_CERT_CLASS_SERVER	2
#define SEC_CERT_CLASS_USER	3
#define SEC_CERT_CLASS_EMAIL	4

struct CERTDERCertsStr {
    PRArenaPool *arena;
    int numcerts;
    SECItem *rawCerts;
};

/*
** A PKCS#10 certificate-request object (the unsigned form)
*/
struct CERTCertificateRequestStr {
    PRArenaPool *arena;
    SECItem version;
    CERTName subject;
    CERTSubjectPublicKeyInfo subjectPublicKeyInfo;
    SECItem attributes;
};
#define SEC_CERTIFICATE_REQUEST_VERSION		0	/* what we *create* */

/*
** A certificate list object.
*/
struct CERTCertificateListStr {
    SECItem *certs;
    int len;					/* number of certs */
    PRArenaPool *arena;
};

struct CERTCrlEntryStr {
    SECItem serialNumber;
    SECItem revocationDate;
    CERTCertExtension **extensions;    
};

struct CERTCrlStr {
    PRArenaPool *arena;
    SECItem version;
    SECAlgorithmID signatureAlg;
    SECItem derName;
    CERTName name;
    SECItem lastUpdate;
    SECItem nextUpdate;				/* optional for x.509 CRL  */
    CERTCrlEntry **entries;
    CERTCertExtension **extensions;    
};

struct CERTCrlKeyStr {
    SECItem derName;
    SECItem dummy;			/* The decoder can not skip a primitive,
					   this serves as a place holder for the
					   decoder to finish its task only
					*/
};

struct CERTSignedCrlStr {
    PRArenaPool *arena;
    CERTCrl crl;
    certDBEntryRevocation *dbEntry;	/* database entry struct */
    PRBool keep;			/* keep this crl in the cache for the  session*/
    PRBool isperm;
    PRBool istemp;
    int referenceCount;
    CERTCertDBHandle *dbhandle;
    CERTSignedData signatureWrap;	/* XXX */
    char *url;
};

struct CERTCrlHeadNodeStr {
    PRArenaPool *arena;
    CERTCrlNode *first;
    CERTCrlNode *last;
};

struct CERTCrlNodeStr {
    CERTCrlNode *next;
    int 	type;
    CERTSignedCrl *crl;
};

/*
 * Handle structure for open certificate databases
 */
struct CERTCertDBHandleStr {
    DB *permCertDB;
    DB *tempCertDB;
};

/*
 * Array of X.500 Distinguished Names
 */
struct CERTDistNamesStr {
    PRArenaPool *arena;
    int nnames;
    SECItem  *names;
    void *head; /* private */
};


#define NS_CERT_TYPE_SSL_CLIENT		(0x80)	/* bit 0 */
#define NS_CERT_TYPE_SSL_SERVER		(0x40)  /* bit 1 */
#define NS_CERT_TYPE_EMAIL		(0x20)  /* bit 2 */
#define NS_CERT_TYPE_OBJECT_SIGNING	(0x10)  /* bit 3 */
#define NS_CERT_TYPE_RESERVED		(0x08)  /* bit 4 */
#define NS_CERT_TYPE_SSL_CA		(0x04)  /* bit 5 */
#define NS_CERT_TYPE_EMAIL_CA		(0x02)  /* bit 6 */
#define NS_CERT_TYPE_OBJECT_SIGNING_CA	(0x01)  /* bit 7 */

#define NS_CERT_TYPE_APP ( NS_CERT_TYPE_SSL_CLIENT | \
			  NS_CERT_TYPE_SSL_SERVER | \
			  NS_CERT_TYPE_EMAIL | \
			  NS_CERT_TYPE_OBJECT_SIGNING )

#define NS_CERT_TYPE_CA ( NS_CERT_TYPE_SSL_CA | \
			 NS_CERT_TYPE_EMAIL_CA | \
			 NS_CERT_TYPE_OBJECT_SIGNING_CA )
typedef enum {
    certUsageSSLClient,
    certUsageSSLServer,
    certUsageSSLServerWithStepUp,
    certUsageSSLCA,
    certUsageEmailSigner,
    certUsageEmailRecipient,
    certUsageObjectSigner,
    certUsageUserCertImport,
    certUsageVerifyCA
} SECCertUsage;

/*
 * This enum represents the state of validity times of a certificate
 */
typedef enum {
    secCertTimeValid,
    secCertTimeExpired,
    secCertTimeNotValidYet
} SECCertTimeValidity;

/*
 * Interface for getting certificate nickname strings out of the database
 */

/* these are values for the what argument below */
#define SEC_CERT_NICKNAMES_ALL		1
#define SEC_CERT_NICKNAMES_USER		2
#define SEC_CERT_NICKNAMES_SERVER	3
#define SEC_CERT_NICKNAMES_CA		4

struct CERTCertNicknamesStr {
    PRArenaPool *arena;
    void *head;
    int numnicknames;
    char **nicknames;
    int what;
    int totallen;
};

struct CERTIssuerAndSNStr {
    SECItem derIssuer;
    CERTName issuer;
    SECItem serialNumber;
};


/* X.509 v3 Key Usage Extension flags */
#define KU_DIGITAL_SIGNATURE		(0x80)	/* bit 0 */
#define KU_NON_REPUDIATION		(0x40)  /* bit 1 */
#define KU_KEY_ENCIPHERMENT		(0x20)  /* bit 2 */
#define KU_DATA_ENCIPHERMENT		(0x10)  /* bit 3 */
#define KU_KEY_AGREEMENT		(0x08)  /* bit 4 */
#define KU_KEY_CERT_SIGN		(0x04)  /* bit 5 */
#define KU_CRL_SIGN			(0x02)  /* bit 6 */
#define KU_ALL				(KU_DIGITAL_SIGNATURE | \
					 KU_NON_REPUDIATION | \
					 KU_KEY_ENCIPHERMENT | \
					 KU_DATA_ENCIPHERMENT | \
					 KU_KEY_AGREEMENT | \
					 KU_KEY_CERT_SIGN | \
					 KU_CRL_SIGN)

/* internal bits that do not match bits in the x509v3 spec, but are used
 * for similar purposes
 */
#define KU_NS_GOVT_APPROVED		(0x8000) /*don't make part of KU_ALL!*/
/*
 * x.509 v3 Basic Constraints Extension
 * If isCA is false, the pathLenConstraint is ignored.
 * Otherwise, the following pathLenConstraint values will apply:
 *	< 0 - there is no limit to the certificate path
 *	0   - CA can issues end-entity certificates only
 *	> 0 - the number of certificates in the certificate path is
 *	      limited to this number
 */
#define CERT_UNLIMITED_PATH_CONSTRAINT -2

struct CERTBasicConstraintsStr {
    PRBool isCA;			/* on if is CA */
    int pathLenConstraint;		/* maximum number of certificates that can be
					   in the cert path.  Only applies to a CA
					   certificate; otherwise, it's ignored.
					 */
};

/* x.509 v3 Reason Falgs, used in CRLDistributionPoint Extension */
#define RF_UNUSED			(0x80)	/* bit 0 */
#define RF_KEY_COMPROMISE		(0x40)  /* bit 1 */
#define RF_CA_COMPROMISE		(0x20)  /* bit 2 */
#define RF_AFFILIATION_CHANGED		(0x10)  /* bit 3 */
#define RF_SUPERSEDED			(0x08)  /* bit 4 */
#define RF_CESSATION_OF_OPERATION	(0x04)  /* bit 5 */
#define RF_CERTIFICATE_HOLD		(0x02)  /* bit 6 */

/* If we needed to extract the general name field, use this */
/* General Name types */
typedef enum {
    certOtherName = 1,
    certRFC822Name = 2,
    certDNSName = 3,
    certX400Address = 4,
    certDirectoryName = 5,
    certEDIPartyName = 6,
    certURI = 7,
    certIPAddress = 8,
    certRegisterID = 9
} CERTGeneralNameType;

/* This support the General Name type.  For now, we only support the URI
   name type.
 */
struct CERTGeneralNameStr {
    CERTGeneralNameType type;		/* name type */
    union {
	CERTName directoryName;         /* distinguish name */
	SECItem uri;			/* uniform resource identifier */
	SECItem other;			/* other name forms - save as an encoded blob */
    } name;
    SECItem derDirectoryName;		/* this is saved to simplify directory name
					   comparison */
};


/* X.509 v3 Authority Key Identifier extension.  For the authority certificate
   issuer field, we only support URI now.
 */
struct CERTAuthKeyIDStr {
    SECItem keyID;			/* unique key identifier */
    CERTGeneralName **authCertIssuer;	/* CA's issuer name.  End with a NULL */
    SECItem authCertSerialNumber;	/* CA's certificate serial number */
    SECItem **DERAuthCertIssuer;	/* This holds the DER encoded format of
					   the authCertIssuer field. It is used
					   by the encoding engine. It should be
					   used as a read only field by the caller.
					*/
};

/* x.509 v3 CRL Distributeion Point */

/*
 * defined the types of CRL Distribution points
 */
typedef enum {
    generalName = 1,			/* only support this for now */
    relativeDistinguishedName = 2
} DistributionPointTypes;

struct CRLDistributionPointStr {
    DistributionPointTypes distPointType;
    union {
	CERTGeneralName **fullName;
	CERTRDN relativeName;
    } distPoint;
    SECItem reasons;
    CERTGeneralName **crlIssuer;
    
    /* Reserved for internal use only*/
    SECItem derDistPoint;
    SECItem derRelativeName;
    SECItem **derCrlIssuer;
    SECItem **derFullName;
    SECItem bitsmap;
};

struct CERTCrlDistributionPointsStr {
    CRLDistributionPoint **distPoints;
};

/*
 * This structure is used to keep a log of errors when verifying
 * a cert chain.  This allows multiple errors to be reported all at
 * once.
 */
struct CERTVerifyLogNodeStr {
    CERTCertificate *cert;	/* what cert had the error */
    long error;			/* what error was it? */
    unsigned int depth;		/* how far up the chain are we */
    void *arg;			/* error specific argument */
    struct CERTVerifyLogNodeStr *next; /* next in the list */
    struct CERTVerifyLogNodeStr *prev; /* next in the list */
};

struct CERTVerifyLogStr {
    PRArenaPool *arena;
    unsigned int count;
    struct CERTVerifyLogNodeStr *head;
    struct CERTVerifyLogNodeStr *tail;
};

/* This is the typedef for the callback passed to CERT_OpenCertDB() */
/* callback to return database name based on version number */
typedef char * (*CERTDBNameFunc)(void *arg, int dbVersion);

/*
 * types of cert packages that we can decode
 */
typedef enum {
    certPackageNone,
    certPackageCert,
    certPackagePKCS7,
    certPackageNSCertSeq,
    certPackageNSCertWrap
} CERTPackageType;

/*
 * these types are for the PKIX Certificate Policies extension
 */
typedef struct {
    SECOidTag oid;
    SECItem qualifierID;
    SECItem qualifierValue;
} CERTPolicyQualifier;

typedef struct {
    SECOidTag oid;
    SECItem policyID;
    CERTPolicyQualifier **policyQualifiers;
} CERTPolicyInfo;

typedef struct {
    PRArenaPool *arena;
    CERTPolicyInfo **policyInfos;
} CERTCertificatePolicies;

typedef struct {
    SECItem organization;
    SECItem **noticeNumbers;
} CERTNoticeReference;

typedef struct {
    PRArenaPool *arena;
    CERTNoticeReference noticeReference;
    SECItem displayText;
} CERTUserNotice;

/* XXX Lisa thinks the template declarations belong in cert.h, not here? */

/* XXX OLD templates; this should go away as soon as we can convert
 * everything over to using the new SEC_ASN1 versions.
 */
extern DERTemplate CERTCertificateRequestTemplate[];
extern DERTemplate CERTCertificateTemplate[];
extern DERTemplate SECSignedCertificateTemplate[];
extern DERTemplate CERTCertExtensionTemplate[];
extern DERTemplate CERTNameTemplate[];
extern DERTemplate SECKEYPublicKeyTemplate[];
extern DERTemplate CERTSignedDataTemplate[];
extern DERTemplate CERTSubjectPublicKeyInfoTemplate[];
extern DERTemplate CERTValidityTemplate[];
extern DERTemplate CERTPublicKeyAndChallengeTemplate[];
extern DERTemplate SECCertSequenceTemplate[];

#include "secasn1t.h"	/* way down here because I expect template stuff to
			 * move out of here anyway */

extern const SEC_ASN1Template CERT_IssuerAndSNTemplate[];
extern const SEC_ASN1Template CERT_NameTemplate[];
extern const SEC_ASN1Template CERT_SetOfSignedCrlTemplate[];
extern const SEC_ASN1Template CERT_RDNTemplate[];
extern const SEC_ASN1Template CERT_SignedDataTemplate[];
extern const SEC_ASN1Template CERT_CrlTemplate[];

#endif /* _CERTT_H_ */
