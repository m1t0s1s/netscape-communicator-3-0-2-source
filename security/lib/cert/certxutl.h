/*
 * x.509 v3 certificate extension helper routines
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 */


#ifndef _CERTXUTL_H_
#define _CERTXUTL_H_

typedef enum {
    CertificateExtensions,
    CrlExtensions
} ExtensionsType;

extern PRBool
cert_HasCriticalExtension (CERTCertExtension **extensions);

extern SECStatus
CERT_FindBitStringExtension (CERTCertExtension **extensions,
			     int tag, SECItem *retItem);
extern void *
cert_StartExtensions (void *owner, ExtensionsType type);

extern SECStatus
cert_FindExtension (CERTCertExtension **extensions, int tag, SECItem *value);

extern SECStatus
cert_FindExtensionByOID (CERTCertExtension **extensions,
			 SECItem *oid, SECItem *value);

extern SECStatus
cert_GetExtenCriticality (CERTCertExtension **extensions,
			  int tag, PRBool *isCritical);

extern PRBool
cert_HasUnknownCriticalExten (CERTCertExtension **extensions);

#endif
