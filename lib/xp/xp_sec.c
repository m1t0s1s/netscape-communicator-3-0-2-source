#include "xp.h"
#include "cert.h"
#include "secder.h"
#include "xp_sec.h"
#include "ssl.h"


/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_SEC_GOT_RSA;
extern int XP_SEC_HIGH_MESSAGE;
extern int XP_SEC_INTERNATIONAL;
extern int XP_SEC_LOW_MESSAGE;
extern int XP_SEC_NO_MESSAGE;

extern int XP_PRETTY_CERT_INF0; 

/*
** Take basic security key information and return an allocated string
** that contains a "pretty printed" version.
*/
char *XP_PrettySecurityStatus(int level, char *cipher, int keySize,
			      int secretKeySize)
{
    int baseLen, len;
    char *rv, *base=NULL;
    char temp[100];

    switch (level) {
      case SSL_SECURITY_STATUS_ON_HIGH:
#ifdef FORTEZZA
      case SSL_SECURITY_STATUS_FORTEZZA:
#endif
	StrAllocCopy(base, XP_GetString(XP_SEC_HIGH_MESSAGE)); /* Move message to xp_msg.i  */
	if (!base) {
	    return NULL;
	}
	baseLen = XP_STRLEN(base);
	break;

      case SSL_SECURITY_STATUS_ON_LOW:
	StrAllocCopy(base, XP_GetString(XP_SEC_LOW_MESSAGE)); /* Move message to xp_msg.i  */
	if (!base) {
	    return NULL;
	}
	baseLen = XP_STRLEN(base);
	break;

      default:
	StrAllocCopy(base, XP_GetString(XP_SEC_NO_MESSAGE));  /* Move message to xp_msg.i  */
	return base;
    }
    temp[0] = '\0';

    if (keySize != secretKeySize) {
	XP_SPRINTF(temp, " (%s, %d bit with %d secret).", cipher, keySize,
		   secretKeySize);
    } else {
	XP_SPRINTF(temp, " (%s, %d bit).", cipher, keySize);
    }
    len = baseLen + XP_STRLEN(temp);
    rv = (char*) XP_ALLOC(len+1);
    if (rv) {
	XP_STRCPY(rv, base);
	XP_STRCAT(rv, temp);
    }
	if (base)
		XP_FREE(base);
    return rv;
}

static char *hex = "0123456789ABCDEF";

/*
** Convert a der-encoded integer to a hex printable string form
*/
static char *Hexify(SECItem *i)
{
    unsigned char *cp, *end;
    char *rv, *o;

    if (!i->len) {
	return XP_STRDUP("00");
    }

    rv = o = (char*) XP_ALLOC(i->len * 3);
    if (!rv) return rv;

    cp = i->data;
    end = cp + i->len;
    while (cp < end) {
	unsigned char ch = *cp++;
	*o++ = hex[(ch >> 4) & 0xf];
	*o++ = hex[ch & 0xf];
	if (cp != end) {
	    *o++ = ':';
	} else {
	    *o++ = 0;
	}
    }
    return rv;
}

/*
** Pretty print a certificate, in a form that is suitable for user
** viewing. Only handles X509 certificates for now, and assumes that
** the certificate's signature was already checked.
*/
char *XP_PrettyCertInfo(CERTCertificate *cert)
{
#ifdef HAVE_SECURITY /* added by jwz */
    char *rv;
    char *issuer, *subject, *serialNumber, *version;
    char *notBefore, *notAfter;
    
    if (!cert) return XP_STRDUP("");

    rv = 0;
    issuer = CERT_NameToAscii(&cert->issuer);
    subject = CERT_NameToAscii(&cert->subject);
    version = Hexify(&cert->version);
    serialNumber = Hexify(&cert->serialNumber);
    notBefore = DER_UTCTimeToAscii(&cert->validity.notBefore);
    notAfter = DER_UTCTimeToAscii(&cert->validity.notAfter);

#ifdef NOT
     rv = XP_Cat("Version: ", version, LINEBREAK,
		 "Serial Number: ", serialNumber, LINEBREAK,
		 "Issuer: ", issuer, LINEBREAK,
		 "Subject: ", subject, LINEBREAK,
		 0);
#else
	{

		int len = 0; 

		/* assume we got non-NULL strings back from all calls above */		
		len += XP_STRLEN(issuer);
		len += XP_STRLEN(subject);
		len += XP_STRLEN(version);
		len += XP_STRLEN(serialNumber);
		len += XP_STRLEN(notBefore);
		len += XP_STRLEN(notAfter);
		
		/* add padding for english and linebreaks */
		len += 128;
			
		rv = (char *) XP_ALLOC(len * sizeof(char));
		if(NULL == rv)
			return(NULL);
			
    	sprintf(rv, XP_GetString(XP_PRETTY_CERT_INF0),
		version, LINEBREAK, serialNumber, LINEBREAK, issuer,LINEBREAK, 
		subject, LINEBREAK, notBefore, LINEBREAK, notAfter, LINEBREAK);
    
    }
#endif
    XP_FREE(issuer);
    XP_FREE(subject);
    XP_FREE(version);
    XP_FREE(serialNumber);
    XP_FREE(notBefore);
    XP_FREE(notAfter);
    
    return rv;
#else
    return XP_STRDUP("");
#endif /* !HAVE_SECURITY -- added by jwz */
}

/*
** Return a static string (NOT MALLOC'D) which describes what security
** version the application supports: domestic or international.
**
** Bobj will hate me for this.
*/
char *XP_SecurityVersion(int longForm)
{
#ifndef NO_SSL /* added by jwz */
    int domestic = SSL_IsDomestic();
#else
    int domestic = FALSE;
#endif /* NO_SSL -- added by jwz */
    if (domestic) {
	return longForm ? "U.S." : "U";
    }
    return longForm ? XP_GetString(XP_SEC_INTERNATIONAL) : "I";

}

/*
** Return a dynamically allocated string which describes what security
** version the security library supports. The returned string describes
** in totality the crypto capabilities of the library.
*/

#define GOT_RSA			"RSA Public Key Cryptography"
#define GOT_MD2			"MD2"
#define GOT_MD5			"MD5"
#define GOT_RC2_CBC		"RC2-CBC"
#define GOT_RC4			"RC4"
#define GOT_DES_CBC		"DES-CBC"
#define GOT_DES_EDE3_CBC	"DES-EDE3-CBC"
#define GOT_IDEA_CBC		"IDEA-CBC"
static char SEP[3] = { ',', ' ', '\0' };

char *XP_SecurityCapabilities(void)
{
#ifdef HAVE_SECURITY /* added by jwz */
    unsigned long c = SSL_SecurityCapabilities();
    int len = 1;
    char *rv;

    /* Figure out how long the returned string will be */
    if (c & SSL_SC_RSA)
	len += strlen(XP_GetString(XP_SEC_GOT_RSA)) + sizeof(SEP) - 1;
    if (c & SSL_SC_MD2)
	len += sizeof(GOT_MD2) + sizeof(SEP) - 1;
    if (c & SSL_SC_MD5)
	len += sizeof(GOT_MD5) + sizeof(SEP) - 1;
    if (c & SSL_SC_RC2_CBC)
	len += sizeof(GOT_RC2_CBC) + sizeof(SEP) - 1;
    if (c & SSL_SC_RC4)
	len += sizeof(GOT_RC4) + sizeof(SEP) - 1;
    if (c & SSL_SC_DES_CBC)
	len += sizeof(GOT_DES_CBC) + sizeof(SEP) - 1;
    if (c & SSL_SC_DES_EDE3_CBC)
	len += sizeof(GOT_DES_EDE3_CBC) + sizeof(SEP) - 1;
    if (c & SSL_SC_IDEA_CBC)
	len += sizeof(GOT_IDEA_CBC) + sizeof(SEP) - 1;

    /* Now construct the string */
    rv = (char*) XP_ALLOC(len);
    if (rv) {
	rv[0] = 0;
	if (c & SSL_SC_RSA) {
	    XP_STRCAT(rv, XP_GetString(XP_SEC_GOT_RSA));
	}
	if (c & SSL_SC_MD2) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_MD2);
	}
	if (c & SSL_SC_MD5) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_MD5);
	}
	if (c & SSL_SC_RC2_CBC) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_RC2_CBC);
	}
	if (c & SSL_SC_RC4) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_RC4);
	}
	if (c & SSL_SC_DES_CBC) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_DES_CBC);
	}
	if (c & SSL_SC_DES_EDE3_CBC) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_DES_EDE3_CBC);
	}
	if (c & SSL_SC_IDEA_CBC) {
	    if (rv[0]) XP_STRCAT(rv, SEP);
	    XP_STRCAT(rv, GOT_IDEA_CBC);
	}
    }
    return rv;
#else
    return XP_STRDUP("");
#endif /* !HAVE_SECURITY -- added by jwz */
}
