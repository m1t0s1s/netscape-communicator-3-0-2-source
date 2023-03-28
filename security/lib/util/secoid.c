#include "secoid.h"
#include "mcom_db.h"
#include "pkcs11t.h"
#include "secmodt.h"
#include "secitem.h"

#if defined(SERVER_BUILD)
#define dbopen NS_dbopen
#endif

extern int SEC_ERROR_LIBRARY_FAILURE;

/* MISSI Mosaic Object ID space */
#define MISSI	0x60, 0x86, 0x48, 0x01, 0x65, 0x02, 0x01, 0x01
#define MISSI_OLD_KEA_DSS	MISSI, 0x0c
#define MISSI_OLD_DSS		MISSI, 0x02
#define MISSI_KEA_DSS		MISSI, 0x14
#define MISSI_DSS		MISSI, 0x13

/**
 ** The Netscape OID space is allocated by Jeff Weinstein.  If you need
 ** a piece of the space, contact him at jsw@netscape.com.
 **/

/* Netscape Communications Corporation Object ID space */
/* { 2 16 840 1 113730 } */
#define NETSCAPE_OID	0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x42
/* netscape certificate extensions */
#define NETSCAPE_CERT_EXT NETSCAPE_OID, 0x01

/* netscape data types */
#define NETSCAPE_DATA_TYPE NETSCAPE_OID, 0x02

/* netscape directory oid - owned by Tim Howes(howes@netscape.com) */
#define NETSCAPE_DIRECTORY NETSCAPE_OID, 0x03

/* various policy type OIDs */
#define NETSCAPE_POLICY NETSCAPE_OID, 0x04

/* these are old and should go away soon */
#define OLD_NETSCAPE	0x60, 0x86, 0x48, 0xd8, 0x6a
#define NS_CERT_EXT	OLD_NETSCAPE, 0x01
#define NS_FILE_TYPE	OLD_NETSCAPE, 0x02
#define NS_IMAGE_TYPE	OLD_NETSCAPE, 0x03

/* RSA OID name space */
#define RSADSI	0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d
#define PKCS	RSADSI, 0x01
#define DIGEST	RSADSI, 0x02
#define CIPHER	RSADSI, 0x03
#define PKCS1	PKCS, 0x01
#define PKCS5	PKCS, 0x05
#define PKCS7	PKCS, 0x07
#define PKCS9	PKCS, 0x09
#define PKCS12	PKCS, 0x0c

/* Other OID name spaces */
#define ALGORITHM		0x2b, 0x0e, 0x03, 0x02
#define X500			0x55
#define X520_ATTRIBUTE_TYPE	X500, 0x04
#define X500_ALG		X500, 0x08
#define X500_ALG_ENCRYPTION	X500_ALG, 0x01

/** X.509 v3 Extension OID 
 ** {joint-iso-ccitt (2) ds(5) 29}
 **/
#define	ID_CE_OID X500, 0x1d

#define RFC1274_ATTR_TYPE  0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x1

/* PKCS #12 name spaces */
#define PKCS12_MODE_IDS		PKCS12, 0x01
#define PKCS12_ESPVK_IDS	PKCS12, 0x02
#define PKCS12_BAG_IDS		PKCS12, 0x03
#define PKCS12_CERT_BAG_IDS	PKCS12, 0x04
#define PKCS12_OIDS		PKCS12, 0x05
#define PKCS12_PBE_IDS		PKCS12_OIDS, 0x01
#define PKCS12_ENVELOPING_IDS	PKCS12_OIDS, 0x02
#define PKCS12_SIGNATURE_IDS	PKCS12_OIDS, 0x03

/* for DSA algorithm */
/* { iso(1) member-body(2) us(840) x9-57(10040) x9algorithm(4) } */
#define ANSI_X9_ALGORITHM  0x2a, 0x86, 0x48, 0xce, 0x38, 0x4

#define VERISIGN 0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x45
#define PKIX 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07
#define PKIX_CERT_EXTENSIONS PKIX, 1
#define PKIX_POLICY_QUALIFIERS PKIX, 2
#define PKIX_KEY_USAGE PKIX, 3

static unsigned char md2[] = { DIGEST, 0x02 };
static unsigned char md4[] = { DIGEST, 0x04 };
static unsigned char md5[] = { DIGEST, 0x05 };
static unsigned char sha1[] = { ALGORITHM, 0x1a };
static unsigned char rc2cbc[] = { CIPHER, 0x02 };
static unsigned char rc4[] = { CIPHER, 0x04 };
static unsigned char desede3cbc[] = { CIPHER, 0x07 };
static unsigned char rc5cbcpad[] = { CIPHER, 0x09 };
static unsigned char desecb[] = { ALGORITHM, 0x06 };
static unsigned char descbc[] = { ALGORITHM, 0x07 };
static unsigned char desofb[] = { ALGORITHM, 0x08 };
static unsigned char descfb[] = { ALGORITHM, 0x09 };
static unsigned char desmac[] = { ALGORITHM, 0x0a };
static unsigned char desede[] = { ALGORITHM, 0x11 };
static unsigned char isoSHAWithRSASignature[] = { ALGORITHM, 0xf };
static unsigned char pkcs1RSAEncryption[] = { PKCS1, 0x01 };
static unsigned char pkcs1MD2WithRSAEncryption[] = { PKCS1, 0x02 };
static unsigned char pkcs1MD4WithRSAEncryption[] = { PKCS1, 0x03 };
static unsigned char pkcs1MD5WithRSAEncryption[] = { PKCS1, 0x04 };
static unsigned char pkcs1SHA1WithRSAEncryption[] = { PKCS1, 0x05 };
static unsigned char pkcs5PbeWithMD2AndDEScbc[] = { PKCS5, 0x01 };
static unsigned char pkcs5PbeWithMD5AndDEScbc[] = { PKCS5, 0x03 };
static unsigned char pkcs5PbeWithSha1AndDEScbc[] = { PKCS5, 0x0a };
static unsigned char pkcs7[] = { PKCS7 };
static unsigned char pkcs7Data[] = { PKCS7, 0x01 };
static unsigned char pkcs7SignedData[] = { PKCS7, 0x02 };
static unsigned char pkcs7EnvelopedData[] = { PKCS7, 0x03 };
static unsigned char pkcs7SignedEnvelopedData[] = { PKCS7, 0x04 };
static unsigned char pkcs7DigestedData[] = { PKCS7, 0x05 };
static unsigned char pkcs7EncryptedData[] = { PKCS7, 0x06 };
static unsigned char pkcs9EmailAddress[] = { PKCS9, 0x01 };
static unsigned char pkcs9UnstructuredName[] = { PKCS9, 0x02 };
static unsigned char pkcs9ContentType[] = { PKCS9, 0x03 };
static unsigned char pkcs9MessageDigest[] = { PKCS9, 0x04 };
static unsigned char pkcs9SigningTime[] = { PKCS9, 0x05 };
static unsigned char pkcs9CounterSignature[] = { PKCS9, 0x06 };
static unsigned char pkcs9ChallengePassword[] = { PKCS9, 0x07 };
static unsigned char pkcs9UnstructuredAddress[] = { PKCS9, 0x08 };
static unsigned char pkcs9ExtendedCertificateAttributes[] = { PKCS9, 0x09 };
static unsigned char pkcs9SMIMECapabilities[] = { PKCS9, 15 };
static unsigned char x520CommonName[] = { X520_ATTRIBUTE_TYPE, 3 };
static unsigned char x520CountryName[] = { X520_ATTRIBUTE_TYPE, 6 };
static unsigned char x520LocalityName[] = { X520_ATTRIBUTE_TYPE, 7 };
static unsigned char x520StateOrProvinceName[] = { X520_ATTRIBUTE_TYPE, 8 };
static unsigned char x520OrgName[] = { X520_ATTRIBUTE_TYPE, 10 };
static unsigned char x520OrgUnitName[] = { X520_ATTRIBUTE_TYPE, 11 };

static unsigned char nsTypeGIF[] = { NETSCAPE_DATA_TYPE, 0x01 };
static unsigned char nsTypeJPEG[] = { NETSCAPE_DATA_TYPE, 0x02 };
static unsigned char nsTypeURL[] = { NETSCAPE_DATA_TYPE, 0x03 };
static unsigned char nsTypeHTML[] = { NETSCAPE_DATA_TYPE, 0x04 };
static unsigned char nsTypeCertSeq[] = { NETSCAPE_DATA_TYPE, 0x05 };
static unsigned char missiCertKEADSSOld[] = { MISSI_OLD_KEA_DSS };
static unsigned char missiCertDSSOld[] = { MISSI_OLD_DSS };
static unsigned char missiCertKEADSS[] = { MISSI_KEA_DSS };
static unsigned char missiCertDSS[] = { MISSI_DSS };
static unsigned char x500RSAEncryption[] = { X500_ALG_ENCRYPTION, 0x01 };

/* added for alg 1485 */
static unsigned char rfc1274Uid[] = { RFC1274_ATTR_TYPE, 1 };
static unsigned char rfc1274Mail[] = { RFC1274_ATTR_TYPE, 3 };

/* Netscape private certificate extensions */
static unsigned char nsCertExtNetscapeOK[] = { NS_CERT_EXT, 1 };
static unsigned char nsCertExtIssuerLogo[] = { NS_CERT_EXT, 2 };
static unsigned char nsCertExtSubjectLogo[] = { NS_CERT_EXT, 3 };
static unsigned char nsExtCertType[] = { NETSCAPE_CERT_EXT, 0x01 };
static unsigned char nsExtBaseURL[] = { NETSCAPE_CERT_EXT, 0x02 };
static unsigned char nsExtRevocationURL[] = { NETSCAPE_CERT_EXT, 0x03 };
static unsigned char nsExtCARevocationURL[] = { NETSCAPE_CERT_EXT, 0x04 };
static unsigned char nsExtCACRLURL[] = { NETSCAPE_CERT_EXT, 0x05 };
static unsigned char nsExtCACertURL[] = { NETSCAPE_CERT_EXT, 0x06 };
static unsigned char nsExtCertRenewalURL[] = { NETSCAPE_CERT_EXT, 0x07 };
static unsigned char nsExtCAPolicyURL[] = { NETSCAPE_CERT_EXT, 0x08 };
static unsigned char nsExtHomepageURL[] = { NETSCAPE_CERT_EXT, 0x09 };
static unsigned char nsExtEntityLogo[] = { NETSCAPE_CERT_EXT, 0x0a };
static unsigned char nsExtUserPicture[] = { NETSCAPE_CERT_EXT, 0x0b };
static unsigned char nsExtSSLServerName[] = { NETSCAPE_CERT_EXT, 0x0c };
static unsigned char nsExtComment[] = { NETSCAPE_CERT_EXT, 0x0d };
static unsigned char nsKeyUsageGovtApproved[] = { NETSCAPE_POLICY, 0x01 };

/* Standard x.509 v3 Certificate Extensions */
static unsigned char x509SubjectDirectoryAttr[] = { ID_CE_OID, 9 };
static unsigned char x509SubjectKeyID[] = { ID_CE_OID, 14 };
static unsigned char x509KeyUsage[] = { ID_CE_OID, 15 };
static unsigned char x509PrivateKeyUsagePeriod[] = { ID_CE_OID, 16 };
static unsigned char x509SubjectAltName[] = { ID_CE_OID, 17 };
static unsigned char x509IssuerAltName[] = { ID_CE_OID, 18 };
static unsigned char x509BasicConstraints[] = { ID_CE_OID, 19 };
static unsigned char x509NameConstraints[] = { ID_CE_OID, 30 };
static unsigned char x509CRLDistPoints[] = { ID_CE_OID, 31 };
static unsigned char x509CertificatePolicies[] = { ID_CE_OID, 32 };
static unsigned char x509PolicyMappings[] = { ID_CE_OID, 33 };
static unsigned char x509PolicyConstraints[] = { ID_CE_OID, 34 };
static unsigned char x509AuthKeyID[] = { ID_CE_OID, 35};
static unsigned char x509ExtKeyUsage[] = { ID_CE_OID, 37};

/* Standard x.509 v3 CRL Extensions */
static unsigned char x509CrlNumber[] = { ID_CE_OID, 20};
static unsigned char x509ReasonCode[] = { ID_CE_OID, 21};
static unsigned char x509InvalidDate[] = { ID_CE_OID, 24};

/* pkcs 12 additions */
static unsigned char pkcs12[] = { PKCS12 };
static unsigned char pkcs12ModeIDs[] = { PKCS12_MODE_IDS };
static unsigned char pkcs12ESPVKIDs[] = { PKCS12_ESPVK_IDS };
static unsigned char pkcs12BagIDs[] = { PKCS12_BAG_IDS };
static unsigned char pkcs12CertBagIDs[] = { PKCS12_CERT_BAG_IDS };
static unsigned char pkcs12OIDs[] = { PKCS12_OIDS };
static unsigned char pkcs12PBEIDs[] = { PKCS12_PBE_IDS };
static unsigned char pkcs12EnvelopingIDs[] = { PKCS12_ENVELOPING_IDS };
static unsigned char pkcs12SignatureIDs[] = { PKCS12_SIGNATURE_IDS };
static unsigned char pkcs12OfflineTransportMode[] = { PKCS12_MODE_IDS, 0x01 };
static unsigned char pkcs12OnlineTransportMode[] = { PKCS12_MODE_IDS, 0x02 }; 
static unsigned char pkcs12PKCS8KeyShrouding[] = { PKCS12_ESPVK_IDS, 0x01 };
static unsigned char pkcs12KeyBagID[] = { PKCS12_BAG_IDS, 0x01 };
static unsigned char pkcs12CertAndCRLBagID[] = { PKCS12_BAG_IDS, 0x02 };
static unsigned char pkcs12SecretBagID[] = { PKCS12_BAG_IDS, 0x03 };
static unsigned char pkcs12X509CertCRLBag[] = { PKCS12_CERT_BAG_IDS, 0x01 };
static unsigned char pkcs12SDSICertBag[] = { PKCS12_CERT_BAG_IDS, 0x02 };
static unsigned char pkcs12PBEWithSha1And128BitRC4[] = { PKCS12_PBE_IDS, 0x01 };
static unsigned char pkcs12PBEWithSha1And40BitRC4[] = { PKCS12_PBE_IDS, 0x02 };
static unsigned char pkcs12PBEWithSha1AndTripleDESCBC[] = { PKCS12_PBE_IDS, 0x03 };
static unsigned char pkcs12PBEWithSha1And128BitRC2CBC[] = { PKCS12_PBE_IDS, 0x04 };
static unsigned char pkcs12PBEWithSha1And40BitRC2CBC[] = { PKCS12_PBE_IDS, 0x05 };
static unsigned char pkcs12PBEWithSha1AndRC4[] = { PKCS12_PBE_IDS, 0x6 };
static unsigned char pkcs12PBEWithSha1AndRC2CBC[] = { PKCS12_PBE_IDS, 0x7 };
static unsigned char pkcs12RSAEncryptionWith128BitRC4[] =
	{ PKCS12_ENVELOPING_IDS, 0x01 };
static unsigned char pkcs12RSAEncryptionWith40BitRC4[] = 
	{ PKCS12_ENVELOPING_IDS, 0x02 };
static unsigned char pkcs12RSAEncryptionWithTripleDES[] = 
	{ PKCS12_ENVELOPING_IDS, 0x03 }; 
static unsigned char pkcs12RSASignatureWithSHA1Digest[] =
	{ PKCS12_SIGNATURE_IDS, 0x01 };
static unsigned char ansix9DSASignature[] = { ANSI_X9_ALGORITHM, 0x01 };
static unsigned char ansix9DSASignaturewithSHA1Digest[] =
	{ ANSI_X9_ALGORITHM, 0x03 };

/* verisign OIDs */
static unsigned char verisignUserNotices[] = { VERISIGN, 1, 7, 1, 1 };

/* pkix OIDs */
static unsigned char pkixCPSPointerQualifier[] = { PKIX_POLICY_QUALIFIERS, 1 };
static unsigned char pkixUserNoticeQualifier[] = { PKIX_POLICY_QUALIFIERS, 2 };

/*
 * NOTE: the order of these entries must mach the SECOidTag enum in secoidt.h!
 */
static SECOidData oids[] = {
    { { siDEROID, NULL, 0 },
	  SEC_OID_UNKNOWN,
	  "Unknown OID", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, md2, sizeof(md2) },
	  SEC_OID_MD2,
	  "MD2", CKM_MD2, INVALID_CERT_EXTENSION },
    { { siDEROID, md4, sizeof(md4) },
	  SEC_OID_MD4,
	  "MD4", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, md5, sizeof(md5) },
	  SEC_OID_MD5,
	  "MD5", CKM_MD5, INVALID_CERT_EXTENSION },
    { { siDEROID, sha1, sizeof(sha1) },
	  SEC_OID_SHA1,
	  "SHA-1", CKM_SHA_1, INVALID_CERT_EXTENSION },
    { { siDEROID, rc2cbc, sizeof(rc2cbc) },
	  SEC_OID_RC2_CBC,
	  "RC2-CBC", CKM_RC2_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, rc4, sizeof(rc4) },
	  SEC_OID_RC4,
	  "RC4", CKM_RC4, INVALID_CERT_EXTENSION },
    { { siDEROID, desede3cbc, sizeof(desede3cbc) },
	  SEC_OID_DES_EDE3_CBC,
	  "DES-EDE3-CBC", CKM_DES3_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, rc5cbcpad, sizeof(rc5cbcpad) },
	  SEC_OID_RC5_CBC_PAD,
	  "RC5-CBCPad", CKM_RC5_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, desecb, sizeof(desecb) },
	  SEC_OID_DES_ECB,
	  "DES-ECB", CKM_DES_ECB, INVALID_CERT_EXTENSION },
    { { siDEROID, descbc, sizeof(descbc) },
	  SEC_OID_DES_CBC,
	  "DES-CBC", CKM_DES_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, desofb, sizeof(desofb) },
	  SEC_OID_DES_OFB,
	  "DES-OFB", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, descfb, sizeof(descfb) },
	  SEC_OID_DES_CFB,
	  "DES-CFB", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, desmac, sizeof(desmac) },
	  SEC_OID_DES_MAC,
	  "DES-MAC", CKM_DES_MAC, INVALID_CERT_EXTENSION },
    { { siDEROID, desede, sizeof(desede) },
	  SEC_OID_DES_EDE,
	  "DES-EDE", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, isoSHAWithRSASignature, sizeof(isoSHAWithRSASignature) },
	  SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE,
	  "ISO SHA with RSA Signature", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs1RSAEncryption, sizeof(pkcs1RSAEncryption) },
	  SEC_OID_PKCS1_RSA_ENCRYPTION,
	  "PKCS #1 RSA Encryption", CKM_RSA_PKCS, INVALID_CERT_EXTENSION },

    /* the following Signing mechanisms should get new CKM_ values when
     * values for CKM_RSA_WITH_MDX and CKM_RSA_WITH_SHA_1 get defined in
     * PKCS #11.
     */
    { { siDEROID, pkcs1MD2WithRSAEncryption, sizeof(pkcs1MD2WithRSAEncryption) },
	  SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION,
	  "PKCS #1 MD2 With RSA Encryption", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs1MD4WithRSAEncryption, sizeof(pkcs1MD4WithRSAEncryption) },
	  SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION,
	  "PKCS #1 MD4 With RSA Encryption", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs1MD5WithRSAEncryption, sizeof(pkcs1MD5WithRSAEncryption) },
	  SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION,
	  "PKCS #1 MD5 With RSA Encryption", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs1SHA1WithRSAEncryption, sizeof(pkcs1SHA1WithRSAEncryption) },
	  SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION,
	  "PKCS #1 SHA-1 With RSA Encryption", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },

    { { siDEROID, pkcs5PbeWithMD2AndDEScbc, sizeof(pkcs5PbeWithMD2AndDEScbc) },
	  SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC,
	  "PKCS #5 Password Based Encryption with MD2 and DES CBC",
	  CKM_PBE_MD2_DES_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs5PbeWithMD5AndDEScbc, sizeof(pkcs5PbeWithMD5AndDEScbc) },
	  SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC,
	  "PKCS #5 Password Based Encryption with MD5 and DES CBC",
	  CKM_PBE_MD5_DES_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs5PbeWithSha1AndDEScbc,
	  sizeof(pkcs5PbeWithSha1AndDEScbc) },
          SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC,
	  "PKCS #5 Password Based Encryption with SHA1 and DES CBC", 
	  CKM_NETSCAPE_PBE_SHA1_DES_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7, sizeof(pkcs7) },
	  SEC_OID_PKCS7,
	  "PKCS #7", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7Data, sizeof(pkcs7Data) },
	  SEC_OID_PKCS7_DATA,
	  "PKCS #7 Data", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7SignedData, sizeof(pkcs7SignedData) },
	  SEC_OID_PKCS7_SIGNED_DATA,
	  "PKCS #7 Signed Data", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7EnvelopedData, sizeof(pkcs7EnvelopedData) },
	  SEC_OID_PKCS7_ENVELOPED_DATA,
	  "PKCS #7 Enveloped Data", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7SignedEnvelopedData, sizeof(pkcs7SignedEnvelopedData) },
	  SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA,
	  "PKCS #7 Signed And Enveloped Data", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7DigestedData, sizeof(pkcs7DigestedData) },
	  SEC_OID_PKCS7_DIGESTED_DATA,
	  "PKCS #7 Digested Data", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs7EncryptedData, sizeof(pkcs7EncryptedData) },
	  SEC_OID_PKCS7_ENCRYPTED_DATA,
	  "PKCS #7 Encrypted Data", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9EmailAddress, sizeof(pkcs9EmailAddress) },
	  SEC_OID_PKCS9_EMAIL_ADDRESS,
	  "PKCS #9 Email Address", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9UnstructuredName, sizeof(pkcs9UnstructuredName) },
	  SEC_OID_PKCS9_UNSTRUCTURED_NAME,
	  "PKCS #9 Unstructured Name", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9ContentType, sizeof(pkcs9ContentType) },
	  SEC_OID_PKCS9_CONTENT_TYPE,
	  "PKCS #9 Content Type", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9MessageDigest, sizeof(pkcs9MessageDigest) },
	  SEC_OID_PKCS9_MESSAGE_DIGEST,
	  "PKCS #9 Message Digest", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9SigningTime, sizeof(pkcs9SigningTime) },
	  SEC_OID_PKCS9_SIGNING_TIME,
	  "PKCS #9 Signing Time", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9CounterSignature, sizeof(pkcs9CounterSignature) },
	  SEC_OID_PKCS9_COUNTER_SIGNATURE,
	  "PKCS #9 Counter Signature", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9ChallengePassword, sizeof(pkcs9ChallengePassword) },
	  SEC_OID_PKCS9_CHALLENGE_PASSWORD,
	  "PKCS #9 Challenge Password", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9UnstructuredAddress, sizeof(pkcs9UnstructuredAddress) },
	  SEC_OID_PKCS9_UNSTRUCTURED_ADDRESS,
	  "PKCS #9 Unstructured Address", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9ExtendedCertificateAttributes,
          sizeof(pkcs9ExtendedCertificateAttributes) },
	  SEC_OID_PKCS9_EXTENDED_CERTIFICATE_ATTRIBUTES,
	  "PKCS #9 Extended Certificate Attributes", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs9SMIMECapabilities,
          sizeof(pkcs9SMIMECapabilities) },
	  SEC_OID_PKCS9_SMIME_CAPABILITIES,
	  "PKCS #9 S/MIME Capabilities", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, x520CommonName,
	  sizeof(x520CommonName) },
	  SEC_OID_AVA_COMMON_NAME,
	  "X520 Common Name", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, x520CountryName,
	  sizeof(x520CountryName) },
	  SEC_OID_AVA_COUNTRY_NAME,
	  "X520 Country Name", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, x520LocalityName,
	  sizeof(x520LocalityName) },
	  SEC_OID_AVA_LOCALITY,
	  "X520 Locality Name", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, x520StateOrProvinceName,
	  sizeof(x520StateOrProvinceName) },
	  SEC_OID_AVA_STATE_OR_PROVINCE,
	  "X520 State Or Province Name", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, x520OrgName,
	  sizeof(x520OrgName) },
	  SEC_OID_AVA_ORGANIZATION_NAME,
	  "X520 Organization Name", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, x520OrgUnitName,
	  sizeof(x520OrgUnitName) },
	  SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME,
	  "X520 Organizational Unit Name", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },

    { { siDEROID, nsTypeGIF,
	  sizeof(nsTypeGIF) },
	  SEC_OID_NS_TYPE_GIF,
	  "GIF", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, nsTypeJPEG,
	  sizeof(nsTypeJPEG) },
	  SEC_OID_NS_TYPE_JPEG,
	  "JPEG", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, nsTypeURL,
	  sizeof(nsTypeURL) },
	  SEC_OID_NS_TYPE_URL,
	  "URL", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, nsTypeHTML,
	  sizeof(nsTypeHTML) },
	  SEC_OID_NS_TYPE_HTML,
	  "HTML", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, nsTypeCertSeq,
	  sizeof(nsTypeCertSeq) },
	  SEC_OID_NS_TYPE_CERT_SEQUENCE,
	  "Certificate Sequence", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, missiCertKEADSSOld, sizeof(missiCertKEADSSOld) },
          SEC_OID_MISSI_KEA_DSS_OLD, "MISSI KEA and DSS Algorithm (Old)",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION},
    { { siDEROID, missiCertDSSOld, sizeof(missiCertDSSOld) },
          SEC_OID_MISSI_DSS_OLD, "MISSI DSS Algorithm (Old)",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION  },
    { { siDEROID, missiCertKEADSS, sizeof(missiCertKEADSS) },
          SEC_OID_MISSI_KEA_DSS, "MISSI KEA and DSS Algorithm",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION  },
    { { siDEROID, missiCertDSS, sizeof(missiCertDSS) },
          SEC_OID_MISSI_DSS, "MISSI DSS Algorithm",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION  },

    /* Netscape private extensions */
    { { siDEROID, nsCertExtNetscapeOK,
	  sizeof(nsCertExtNetscapeOK) },
	  SEC_OID_NS_CERT_EXT_NETSCAPE_OK,
	  "Netscape says this cert is OK",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsCertExtIssuerLogo,
	  sizeof(nsCertExtIssuerLogo) },
	  SEC_OID_NS_CERT_EXT_ISSUER_LOGO,
	  "Certificate Issuer Logo",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsCertExtSubjectLogo,
	  sizeof(nsCertExtSubjectLogo) },
	  SEC_OID_NS_CERT_EXT_SUBJECT_LOGO,
	  "Certificate Subject Logo",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtCertType,
	  sizeof(nsExtCertType) },
	  SEC_OID_NS_CERT_EXT_CERT_TYPE,
	  "Certificate Type",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtBaseURL,
	  sizeof(nsExtBaseURL) },
	  SEC_OID_NS_CERT_EXT_BASE_URL,
	  "Certificate Extension Base URL",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtRevocationURL,
	  sizeof(nsExtRevocationURL) },
	  SEC_OID_NS_CERT_EXT_REVOCATION_URL,
	  "Certificate Revocation URL",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtCARevocationURL,
	  sizeof(nsExtCARevocationURL) },
	  SEC_OID_NS_CERT_EXT_CA_REVOCATION_URL,
	  "Certificate Authority Revocation URL",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtCACRLURL,
	  sizeof(nsExtCACRLURL) },
	  SEC_OID_NS_CERT_EXT_CA_CRL_URL,
	  "Certificate Authority CRL Download URL",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtCACertURL,
	  sizeof(nsExtCACertURL) },
	  SEC_OID_NS_CERT_EXT_CA_CERT_URL,
	  "Certificate Authority Certificate Download URL",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtCertRenewalURL,
	  sizeof(nsExtCertRenewalURL) },
	  SEC_OID_NS_CERT_EXT_CERT_RENEWAL_URL,
	  "Certificate Renewal URL", CKM_INVALID_MECHANISM,
	  SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtCAPolicyURL,
	  sizeof(nsExtCAPolicyURL) },
	  SEC_OID_NS_CERT_EXT_CA_POLICY_URL,
	  "Certificate Authority Policy URL",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtHomepageURL,
	  sizeof(nsExtHomepageURL) },
	  SEC_OID_NS_CERT_EXT_HOMEPAGE_URL,
	  "Certificate Homepage URL", CKM_INVALID_MECHANISM,
	  UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtEntityLogo,
	  sizeof(nsExtEntityLogo) },
	  SEC_OID_NS_CERT_EXT_ENTITY_LOGO,
	  "Certificate Entity Logo", CKM_INVALID_MECHANISM,
	  UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtUserPicture,
	  sizeof(nsExtUserPicture) },
	  SEC_OID_NS_CERT_EXT_USER_PICTURE,
	  "Certificate User Picture", CKM_INVALID_MECHANISM,
	  UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtSSLServerName,
	  sizeof(nsExtSSLServerName) },
	  SEC_OID_NS_CERT_EXT_SSL_SERVER_NAME,
	  "Certificate SSL Server Name", CKM_INVALID_MECHANISM,
	  SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsExtComment,
	  sizeof(nsExtComment) },
	  SEC_OID_NS_CERT_EXT_COMMENT,
	  "Certificate Comment", CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, nsKeyUsageGovtApproved,
	  sizeof(nsKeyUsageGovtApproved) },
	  SEC_OID_NS_KEY_USAGE_GOVT_APPROVED,
	  "Strong Crypto Export Approved",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },

    /* x.509 v3 certificate extensions */
    { { siDEROID, x509SubjectDirectoryAttr, sizeof(x509SubjectDirectoryAttr) },
	  SEC_OID_X509_SUBJECT_DIRECTORY_ATTR,
	  "Certificate Subject Directory Attributes",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION},
    { { siDEROID, x509SubjectKeyID, sizeof(x509SubjectKeyID) },
	  SEC_OID_X509_SUBJECT_KEY_ID, "Certificate Subject Key ID",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509KeyUsage, sizeof(x509KeyUsage) },
	  SEC_OID_X509_KEY_USAGE, "Certificate Key Usage",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509PrivateKeyUsagePeriod,
	  sizeof(x509PrivateKeyUsagePeriod) },
	  SEC_OID_X509_PRIVATE_KEY_USAGE_PERIOD,
	  "Certificate Private Key Usage Period",
          CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509SubjectAltName, sizeof(x509SubjectAltName) },
	  SEC_OID_X509_SUBJECT_ALT_NAME, "Certificate Subject Alt Name",
          CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509IssuerAltName, sizeof(x509IssuerAltName) },
	  SEC_OID_X509_ISSUER_ALT_NAME, "Certificate Issuer Alt Name",
          CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509BasicConstraints, sizeof(x509BasicConstraints) },
	  SEC_OID_X509_BASIC_CONSTRAINTS, "Certificate Basic Constraints",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509NameConstraints, sizeof(x509NameConstraints) },
	  SEC_OID_X509_NAME_CONSTRAINTS, "Certificate Name Constraints",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509CRLDistPoints, sizeof(x509CRLDistPoints) },
	  SEC_OID_X509_CRL_DIST_POINTS, "CRL Distribution Points",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509CertificatePolicies, sizeof(x509CertificatePolicies) },
	  SEC_OID_X509_CERTIFICATE_POLICIES,
	  "Certificate Policies",
          CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509PolicyMappings, sizeof(x509PolicyMappings) },
	  SEC_OID_X509_POLICY_MAPPINGS, "Certificate Policy Mappings",
          CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509PolicyConstraints, sizeof(x509PolicyConstraints) },
	  SEC_OID_X509_POLICY_CONSTRAINTS, "Certificate Policy Constraints",
          CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509AuthKeyID, sizeof(x509AuthKeyID) },
	  SEC_OID_X509_AUTH_KEY_ID, "Certificate Authority Key Identifier",
	  CKM_INVALID_MECHANISM, SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509ExtKeyUsage, sizeof(x509ExtKeyUsage) },
	  SEC_OID_X509_EXT_KEY_USAGE, "Extended Key Usage",
	  CKM_INVALID_MECHANISM, UNSUPPORTED_CERT_EXTENSION },

    /* x.509 v3 CRL extensions */
    { { siDEROID, x509CrlNumber, sizeof(x509CrlNumber) },
	  SEC_OID_X509_CRL_NUMBER, "CRL	Number", CKM_INVALID_MECHANISM,
	  SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509ReasonCode, sizeof(x509ReasonCode) },
	  SEC_OID_X509_REASON_CODE, "CRL reason code", CKM_INVALID_MECHANISM,
	  SUPPORTED_CERT_EXTENSION },
    { { siDEROID, x509InvalidDate, sizeof(x509InvalidDate) },
	  SEC_OID_X509_INVALID_DATE, "Invalid Date", CKM_INVALID_MECHANISM,
	  SUPPORTED_CERT_EXTENSION },
	  
    { { siDEROID, x500RSAEncryption, sizeof(x500RSAEncryption) },
	  SEC_OID_X500_RSA_ENCRYPTION,
	  "X500 RSA Encryption", CKM_RSA_X_509, INVALID_CERT_EXTENSION },

    /* added for alg 1485 */
    { { siDEROID, rfc1274Uid,
	  sizeof(rfc1274Uid) },
	  SEC_OID_RFC1274_UID,
	  "RFC1274 User Id", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, rfc1274Mail,
	  sizeof(rfc1274Uid) },
	  SEC_OID_RFC1274_MAIL,
	  "RFC1274 E-mail Address", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },

    /* pkcs 12 additions */
    { { siDEROID, pkcs12, sizeof(pkcs12) },
	  SEC_OID_PKCS12,
	  "PKCS #12", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12ModeIDs, sizeof(pkcs12ModeIDs) },
	  SEC_OID_PKCS12_MODE_IDS,
	  "PKCS #12 Mode IDs", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12ESPVKIDs, sizeof(pkcs12ESPVKIDs) },
	  SEC_OID_PKCS12_ESPVK_IDS,
	  "PKCS #12 ESPVK IDs", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12BagIDs, sizeof(pkcs12BagIDs) },
	  SEC_OID_PKCS12_BAG_IDS,
	  "PKCS #12 Bag IDs", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12CertBagIDs, sizeof(pkcs12CertBagIDs) },
	  SEC_OID_PKCS12_CERT_BAG_IDS,
	  "PKCS #12 Cert Bag IDs", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12OIDs, sizeof(pkcs12OIDs) },
	  SEC_OID_PKCS12_OIDS,
	  "PKCS #12 OIDs", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEIDs, sizeof(pkcs12PBEIDs) },
	  SEC_OID_PKCS12_PBE_IDS,
	  "PKCS #12 PBE IDs", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12SignatureIDs, sizeof(pkcs12SignatureIDs) },
	  SEC_OID_PKCS12_SIGNATURE_IDS,
	  "PKCS #12 Signature IDs", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12EnvelopingIDs, sizeof(pkcs12EnvelopingIDs) },
	  SEC_OID_PKCS12_ENVELOPING_IDS,
	  "PKCS #12 Enveloping IDs", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12OfflineTransportMode, 
    	sizeof(pkcs12OfflineTransportMode) },
	  SEC_OID_PKCS12_OFFLINE_TRANSPORT_MODE,
	  "PKCS #12 Offline Transport Mode", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12OnlineTransportMode, 
    	sizeof(pkcs12OnlineTransportMode) },
	  SEC_OID_PKCS12_ONLINE_TRANSPORT_MODE,
	  "PKCS #12 Online Transport Mode", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PKCS8KeyShrouding, 
    	sizeof(pkcs12PKCS8KeyShrouding) },
	  SEC_OID_PKCS12_PKCS8_KEY_SHROUDING,
	  "PKCS #12 Key Shrouding", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12KeyBagID, 
    	sizeof(pkcs12KeyBagID) },
	  SEC_OID_PKCS12_KEY_BAG_ID,
	  "PKCS #12 Key Bag ID", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12CertAndCRLBagID, 
    	sizeof(pkcs12CertAndCRLBagID) },
	  SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID,
	  "PKCS #12 Cert And CRL Bag ID", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12SecretBagID, 
    	sizeof(pkcs12SecretBagID) },
	  SEC_OID_PKCS12_SECRET_BAG_ID,
	  "PKCS #12 Secret Bag ID", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12X509CertCRLBag, 
    	sizeof(pkcs12X509CertCRLBag) },
	  SEC_OID_PKCS12_X509_CERT_CRL_BAG,
	  "PKCS #12 X509 Cert CRL Bag", CKM_INVALID_MECHANISM,
	  INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12SDSICertBag, 
    	sizeof(pkcs12SDSICertBag) },
	  SEC_OID_PKCS12_SDSI_CERT_BAG,
	  "PKCS #12 SDSI Cert Bag", CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1And128BitRC4, 
    	sizeof(pkcs12PBEWithSha1And128BitRC4) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4,
	  "PKCS #12 PBE With Sha1 and 128 Bit RC4", 
	  CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1And40BitRC4, 
    	sizeof(pkcs12PBEWithSha1And40BitRC4) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4,
	  "PKCS #12 PBE With Sha1 and 40 Bit RC4", 
	  CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1AndTripleDESCBC, 
    	sizeof(pkcs12PBEWithSha1AndTripleDESCBC) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC,
	  "PKCS #12 PBE With Sha1 and Triple DES CBC", 
	  CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1And128BitRC2CBC, 
    	sizeof(pkcs12PBEWithSha1And128BitRC2CBC) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC,
	  "PKCS #12 PBE With Sha1 and 128 Bit RC2 CBC", 
	  CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1And40BitRC2CBC, 
    	sizeof(pkcs12PBEWithSha1And40BitRC2CBC) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC,
	  "PKCS #12 PBE With Sha1 and 40 Bit RC2 CBC", 
	  CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1AndRC2CBC, 
    	sizeof(pkcs12PBEWithSha1AndRC2CBC) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC2_CBC,
	  "PKCS #12 PBE With Sha1 and RC2 CBC", 
	  CKM_NETSCAPE_PBE_SHA1_RC2_CBC, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12PBEWithSha1AndRC4, 
    	sizeof(pkcs12PBEWithSha1AndRC4) },
	  SEC_OID_PKCS12_PBE_WITH_SHA1_AND_RC4,
	  "PKCS #12 PBE With Sha1 and RC4", 
	  CKM_NETSCAPE_PBE_SHA1_RC4, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12RSAEncryptionWith128BitRC4, 
    	sizeof(pkcs12RSAEncryptionWith128BitRC4) },
	  SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_128_BIT_RC4,
	  "PKCS #12 RSA Encryption with 128 Bit RC4",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12RSAEncryptionWith40BitRC4, 
    	sizeof(pkcs12RSAEncryptionWith40BitRC4) },
	  SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_40_BIT_RC4,
	  "PKCS #12 RSA Encryption with 40 Bit RC4",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12RSAEncryptionWithTripleDES, 
    	sizeof(pkcs12RSAEncryptionWithTripleDES) },
	  SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_TRIPLE_DES,
	  "PKCS #12 RSA Encryption with Triple DES",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },
    { { siDEROID, pkcs12RSASignatureWithSHA1Digest, 
    	sizeof(pkcs12RSASignatureWithSHA1Digest) },
	  SEC_OID_PKCS12_RSA_SIGNATURE_WITH_SHA1_DIGEST,
	  "PKCS #12 RSA Encryption with Triple DES",
	  CKM_INVALID_MECHANISM, INVALID_CERT_EXTENSION },

    /* DSA signatures */
    { { siDEROID, ansix9DSASignature, sizeof(ansix9DSASignature) },
        SEC_OID_ANSIX9_DSA_SIGNATURE,
	"ANSI X9.57 DSA Signature", CKM_INVALID_MECHANISM,
	INVALID_CERT_EXTENSION },
    { { siDEROID, ansix9DSASignaturewithSHA1Digest,
	sizeof(ansix9DSASignaturewithSHA1Digest) },
        SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST,
	"ANSI X9.57 DSA Signature with SHA1 Digest", CKM_INVALID_MECHANISM,
	INVALID_CERT_EXTENSION },

    /* verisign oids */
    { { siDEROID, verisignUserNotices,
	sizeof(verisignUserNotices) },
        SEC_OID_VERISIGN_USER_NOTICES,
	"Verisign User Notices", CKM_INVALID_MECHANISM,
	INVALID_CERT_EXTENSION },

    /* pkix oids */
    { { siDEROID, pkixCPSPointerQualifier,
	sizeof(pkixCPSPointerQualifier) },
        SEC_OID_PKIX_CPS_POINTER_QUALIFIER,
	"PKIX CPS Pointer Qualifier", CKM_INVALID_MECHANISM,
	INVALID_CERT_EXTENSION },
    { { siDEROID, pkixUserNoticeQualifier,
	sizeof(pkixUserNoticeQualifier) },
        SEC_OID_PKIX_USER_NOTICE_QUALIFIER,
	"PKIX User Notice Qualifier", CKM_INVALID_MECHANISM,
	INVALID_CERT_EXTENSION },
};

/*
 * now the dynamic table. The dynamic table gets build at init time.
 *  and gets modified if the user loads new crypto modules.
 */

static DB *oid_d_hash = 0;
static SECOidData **secoidDynamicTable = NULL;
static int secoidDynamicTableSize = 0;
static int secoidLastDynamicEntry = 0;
static int secoidLastHashEntry = 0;

static SECStatus
secoid_DynamicRehash(void)
{
    DBT key;
    DBT data;
    int rv;
    SECOidData *oid;
    int i;
    int last = secoidLastDynamicEntry;

    if (!oid_d_hash) {
        oid_d_hash = dbopen( 0, O_RDWR | O_CREAT, 0600, DB_HASH, 0 );
    }


    if ( !oid_d_hash ) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return(SECFailure);
    }

    for ( i = secoidLastHashEntry; i < last; i++ ) {
	oid = secoidDynamicTable[i];

	PORT_Assert ( oid->offset == i );

	key.data = oid->oid.data;
	key.size = oid->oid.len;
	
	data.data = &oid;
	data.size = sizeof(oid);

	rv = (* oid_d_hash->put)( oid_d_hash, &key, &data, R_NOOVERWRITE );
	if ( rv ) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return(SECFailure);
	}
    }
    secoidLastHashEntry = last;
    return(SECSuccess);
}

/*
 * Lookup a Dynamic OID. Dynamic OID's still change slowly, so it's
 * cheaper to rehash the table when it changes than it is to do the loop
 * each time. Worry: what about thread safety here? Global Static data with
 * no locks.... (sigh).
 */
static SECStatus
secoid_FindDynamic(DBT *key, DBT *data) {
    if (secoidDynamicTable == NULL) {
	return DSFailure;
    }
    if (secoidLastHashEntry != secoidLastDynamicEntry) {
	SECStatus rv = secoid_DynamicRehash();
	if ( rv != SECSuccess ) {
	    return rv;
	}
    }
    return (* oid_d_hash->get)( oid_d_hash, key, data, 0 );
	
}

static SECOidData *
secoid_FindDynamicByTag(SECOidTag tagnum)
{
    if (secoidDynamicTable == NULL) {
	return NULL;
    }

    if (tagnum < SEC_OID_TOTAL) {
	return NULL;
    }

    tagnum -= SEC_OID_TOTAL;
    if (tagnum >= secoidLastDynamicEntry) {
	return NULL;
    }

    return(secoidDynamicTable[tagnum]);
}

/*
 * this routine is definately not thread safe. It is only called out
 * of the UI, or at init time. If we want to call it any other time,
 * we need to make it thread safe.
 */
SECStatus
SECOID_AddEntry(SECItem *oid, char *description, unsigned long mech) {
    SECOidData *oiddp = (SECOidData *)PORT_Alloc(sizeof(SECOidData));
    int last = secoidLastDynamicEntry;
    int tableSize = secoidDynamicTableSize;
    int next = last++;
    SECOidData **newTable = secoidDynamicTable;
    SECOidData **oldTable = NULL;

    if (oid == NULL) {
	return SECFailure;
    }

    /* fill in oid structure */
    if (SECITEM_CopyItem(NULL,&oiddp->oid,oid) != SECSuccess) {
	PORT_Free(oiddp);
	return SECFailure;
    }
    oiddp->offset = next + SEC_OID_TOTAL;
    /* may we should just reference the copy passed to us? */
    oiddp->desc = PORT_Strdup(description);
    oiddp->mechanism = mech;


    if (last > tableSize) {
	int oldTableSize = tableSize;
	tableSize += 10;
	oldTable = newTable;
	newTable = (SECOidData **)PORT_ZAlloc(sizeof(SECOidData *)*tableSize);
	if (newTable == NULL) {
	   PORT_Free(oiddp->oid.data);
	   PORT_Free(oiddp);
	   return SECFailure;
	}
	PORT_Memcpy(newTable,oldTable,sizeof(SECOidData *)*oldTableSize);
	PORT_Free(oldTable);
    }

    newTable[next] = oiddp;
    secoidDynamicTable = newTable;
    secoidDynamicTableSize = tableSize;
    secoidLastDynamicEntry= last;
    return SECSuccess;
}
	

/* normal static table processing */
static DB *oidhash = 0;

static SECStatus
InitOIDHash(void)
{
    DBT key;
    DBT data;
    int rv;
    SECOidData *oid;
    int i;
    
    oidhash = dbopen( 0, O_RDWR | O_CREAT, 0600, DB_HASH, 0 );

    if ( !oidhash ) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return(SECFailure);
    }

    for ( i = 0; i < ( sizeof(oids) / sizeof(SECOidData) ); i++ ) {
	oid = &oids[i];

	PORT_Assert ( oid->offset == i );

	key.data = oid->oid.data;
	key.size = oid->oid.len;
	
	data.data = &oid;
	data.size = sizeof(oid);

	rv = (* oidhash->put)( oidhash, &key, &data, R_NOOVERWRITE );
	if ( rv ) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return(SECFailure);
	}
    }

    PORT_Assert (i == SEC_OID_TOTAL);

    return(SECSuccess);
}


SECOidData *
SECOID_FindOID(SECItem *oid)
{
    DBT key;
    DBT data;
    SECOidData *ret;
    int rv;
    
    if ( !oidhash ) {
	rv = InitOIDHash();
	if ( rv != SECSuccess ) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return(0);
	}
    }
    
    key.data = oid->data;
    key.size = oid->len;
    
    rv = (* oidhash->get)( oidhash, &key, &data, 0 );
    if ( rv || ( data.size != sizeof(SECOidData*) ) ) {
	rv = secoid_FindDynamic(&key, &data);
	if (rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return(0);
	}
    }

    PORT_Memcpy(&ret, data.data, data.size);

    return(ret);
}

SECOidTag
SECOID_FindOIDTag(SECItem *oid)
{
    SECOidData *oiddata;

    oiddata = SECOID_FindOID (oid);
    if (oiddata == NULL)
	return SEC_OID_UNKNOWN;

    return oiddata->offset;
}

SECOidData *
SECOID_FindOIDByTag(SECOidTag tagnum)
{

    if (tagnum >= SEC_OID_TOTAL) {
	return secoid_FindDynamicByTag(tagnum);
    }

    PORT_Assert((unsigned int)tagnum < (sizeof(oids) / sizeof(SECOidData)));
    return(&oids[tagnum]);
}

PRBool SECOID_KnownCertExtenOID (SECItem *extenOid)
{
    SECOidData * oidData;

    oidData = SECOID_FindOID (extenOid);
    if (oidData == (SECOidData *)NULL)
	return (PR_FALSE);
    return ((oidData->supportedExtension == SUPPORTED_CERT_EXTENSION) ?
            PR_TRUE : PR_FALSE);
}


const char *
SECOID_FindOIDTagDescription(SECOidTag tagnum)
{
  SECOidData *oidData = SECOID_FindOIDByTag(tagnum);
  if (!oidData)
    return 0;
  else
    return oidData->desc;
}
