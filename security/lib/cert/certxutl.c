/*
 * Certificate Extensions handling code
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 */

#include "cert.h"
#include "secitem.h"
#include "secoid.h"
#include "secder.h"
#include "secasn1.h"
#include "certxutl.h"

extern int SEC_ERROR_NO_MEMORY;
extern int SEC_ERROR_EXTENSION_NOT_FOUND;

static CERTCertExtension *
GetExtension (CERTCertExtension **extensions, SECItem *oid)
{
    CERTCertExtension **exts;
    CERTCertExtension *ext = NULL;
    SECComparison comp;

    exts = extensions;
    
    if (exts) {
	while ( *exts ) {
	    ext = *exts;
	    comp = SECITEM_CompareItem(oid, &ext->id);
	    if ( comp == SECEqual ) 
		break;

	    exts++;
	}
	return (*exts ? ext : NULL);
    }
    return (NULL);
}

SECStatus
cert_FindExtensionByOID (CERTCertExtension **extensions, SECItem *oid, SECItem *value)
{
    CERTCertExtension *ext;
    SECStatus rv = SECSuccess;
    
    ext = GetExtension (extensions, oid);
    if (ext == NULL) {
	PORT_SetError (SEC_ERROR_EXTENSION_NOT_FOUND);
	return (SECFailure);
    }
    if (value)
	rv = SECITEM_CopyItem(NULL, value, &ext->value);
    return (rv);
}
    

SECStatus
CERT_GetExtenCriticality (CERTCertExtension **extensions, int tag, PRBool *isCritical)
{
    CERTCertExtension *ext;
    SECOidData *oid;

    if (!isCritical)
	return (SECSuccess);
    
    /* find the extension in the extensions list */
    oid = SECOID_FindOIDByTag(tag);
    if ( !oid ) {
	return(SECFailure);
    }
    ext = GetExtension (extensions, &oid->oid);
    if (ext == NULL) {
	PORT_SetError (SEC_ERROR_EXTENSION_NOT_FOUND);
	return (SECFailure);
    }

    /* If the criticality is omitted, then it is false by default.
       ex->critical.data is NULL */
    if (ext->critical.data == NULL)
	*isCritical = PR_FALSE;
    else
	*isCritical = (ext->critical.data[0] == 0xff) ? PR_TRUE : PR_FALSE;
    return (SECSuccess);    
}

SECStatus
cert_FindExtension(CERTCertExtension **extensions, int tag, SECItem *value)
{
    SECOidData *oid;
    
    oid = SECOID_FindOIDByTag(tag);
    if ( !oid ) {
	return(SECFailure);
    }

    return(cert_FindExtensionByOID(extensions, &oid->oid, value));
}


typedef struct _extNode {
    struct _extNode *next;
    CERTCertExtension *ext;
} extNode;

typedef struct {
    ExtensionsType type;
    union {
	CERTCertificate *cert;
	CERTCrl *crl;
    } owner;
    PRArenaPool *arena;
    extNode *head;
    int count;
}extRec;

void *
cert_StartExtensions(void *owner, ExtensionsType type)
{
    PRArenaPool *arena;
    extRec *handle;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena ) {
	return(0);
    }

    handle = (extRec *)PORT_ArenaAlloc(arena, sizeof(extRec));
    if ( !handle ) {
	PORT_FreeArena(arena, PR_FALSE);
	return(0);
    }

    if (type == CertificateExtensions)
	handle->owner.cert = (CERTCertificate *)owner;
    else
	handle->owner.crl = (CERTCrl *)owner;
    handle->type = type;
    handle->arena = arena;
    handle->head = 0;
    handle->count = 0;
    
    return(handle);
}

static unsigned char hextrue = 0xff;

/*
 * Note - assumes that data pointed to by oid->data will not move
 */
SECStatus
CERT_AddExtensionByOID (void *exthandle, SECItem *oid, SECItem *value,
			PRBool critical, PRBool copyData)
{
    CERTCertExtension *ext;
    PRArenaPool *ownerArena;
    SECStatus rv;
    extNode *node;
    extRec *handle;
    
    handle = (extRec *)exthandle;

    ownerArena = (handle->type == CertificateExtensions) ?
		 handle->owner.cert->arena : handle->owner.crl->arena;
    /* allocate space for extension and list node */
    ext = (CERTCertExtension*)PORT_ArenaZAlloc(ownerArena, sizeof(CERTCertExtension));
    if ( !ext ) {
	return(SECFailure);
    }

    node = (extNode*)PORT_ArenaAlloc(handle->arena, sizeof(extNode));
    if ( !node ) {
	return(SECFailure);
    }

    /* add to list */
    node->next = handle->head;
    handle->head = node;
   
    /* point to ext struct */
    node->ext = ext;
    
    /* the object ID of the extension */
    ext->id = *oid;
    
    /* set critical field */
    if ( critical ) {
	ext->critical.data = (unsigned char*)&hextrue;
	ext->critical.len = 1;
    }

    /* set the value */
    if ( copyData ) {
	rv = SECITEM_CopyItem(handle->owner.cert->arena, &ext->value, value);
	if ( rv ) {
	    return(SECFailure);
	}
    } else {
	ext->value = *value;
    }
    
    handle->count++;
    
    return(SECSuccess);

}

SECStatus
CERT_AddExtension(void *exthandle, int idtag, SECItem *value,
		     PRBool critical, PRBool copyData)
{
    SECOidData *oid;
    
    oid = SECOID_FindOIDByTag(idtag);
    if ( !oid ) {
	return(SECFailure);
    }

    return(CERT_AddExtensionByOID(exthandle, &oid->oid, value, critical, copyData));
}

SECStatus
CERT_EncodeAndAddExtension(void *exthandle, int idtag, SECItem *value,
			      PRBool critical, int dertype)	       
{
    DERTemplate template;
    extRec *handle;
    SECItem *encitem;
    PRArenaPool *arena;
    SECStatus rv;
    
    handle = (extRec *)exthandle;

    template.kind = dertype;
    template.offset = 0;
    template.sub = NULL;
    template.arg = sizeof(SECItem);
    
    arena = (handle->type == CertificateExtensions) ?
	    handle->owner.cert->arena : handle->owner.crl->arena;
    
    encitem = PORT_ArenaAlloc(arena, sizeof(SECItem));
    if ( encitem == NULL ) {
	return(SECFailure);
    }
    
    rv = DER_Encode(arena, encitem, &template, value);
    if ( rv == SECFailure ) {
	return(rv);
    }
    
    rv = CERT_AddExtension(exthandle, idtag, encitem, critical, PR_FALSE);

    return(rv);
}

void
PrepareBitStringForEncoding (SECItem *bitsmap, SECItem *value)
{
  unsigned char onebyte;
  unsigned int i, len = 0;

  /* to prevent warning on some platform at compile time */ 
  onebyte = '\0';   
  /* Get the position of the right-most turn-on bit */ 
  for (i = 0; i < (value->len ) * 8; ++i) {
      if (i % 8 == 0)
	  onebyte = value->data[i/8];
      if (onebyte & 0x80)
	  len = i;            
      onebyte <<= 1;
      
  }
  bitsmap->data = value->data;
  /* Add one here since we work with base 1 */ 
  bitsmap->len = len + 1;
}

SECStatus
CERT_EncodeAndAddBitStrExtension (void *exthandle, int idtag,
				  SECItem *value, PRBool critical)
{
  SECItem bitsmap;
  
  PrepareBitStringForEncoding (&bitsmap, value);
  return (CERT_EncodeAndAddExtension
	  (exthandle, idtag, &bitsmap, critical, DER_BIT_STRING));
}

SECStatus
CERT_FinishExtensions(void *exthandle)
{
    extRec *handle;
    extNode *node;
    CERTCertExtension **exts;
    PRArenaPool *ownerArena;
    
    handle = (extRec *)exthandle;

    ownerArena = (handle->type == CertificateExtensions) ?
		 handle->owner.cert->arena : handle->owner.crl->arena;
    /* allocate space for extensions array */
    exts = (CERTCertExtension**)PORT_ArenaAlloc
	   (ownerArena, sizeof(CERTCertExtension *) * ( handle->count + 1 ) );
    if ( !exts ) {
	return(SECFailure);
    }

    /* put extensions in owner object and update its version number */
    if (handle->type == CertificateExtensions) {
	handle->owner.cert->extensions = exts;
	DER_SetUInteger (ownerArena, &(handle->owner.cert->version),
			 SEC_CERTIFICATE_VERSION_3);
    }
    else {
	handle->owner.crl->extensions = exts;
	DER_SetUInteger (ownerArena, &(handle->owner.crl->version),
			 SEC_CRL_VERSION_2);
    }
	
    /* update the version number */

    /* copy each extension pointer */
    node = handle->head;
    while ( node ) {
	*exts = node->ext;
	
	node = node->next;
	exts++;
    }

    /* terminate the array of extensions */
    *exts = 0;

    /* free working arena */
    PORT_FreeArena(handle->arena, PR_FALSE);
    
    return(SECSuccess);
}

/*
 * get the value of the Netscape Certificate Type Extension
 */
SECStatus
CERT_FindBitStringExtension (CERTCertExtension **extensions, int tag,
			     SECItem *retItem)
{
    SECItem wrapperItem, tmpItem;
    SECStatus rv;
    PRArenaPool *arena = NULL;
    
    wrapperItem.data = NULL;
    tmpItem.data = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	return(SECFailure);
    }
    
    rv = cert_FindExtension(extensions, tag, &wrapperItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = DER_Decode(arena, &tmpItem, SECBitStringTemplate, &wrapperItem);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    retItem->data = (unsigned char *)PORT_Alloc( ( tmpItem.len + 7 ) >> 3 );
    if ( retItem->data == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(retItem->data, tmpItem.data, ( tmpItem.len + 7 ) >> 3);
    retItem->len = tmpItem.len;
    
    rv = SECSuccess;
    goto done;
    
loser:
    rv = SECFailure;

done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    if ( wrapperItem.data ) {
	PORT_Free(wrapperItem.data);
    }

    return(rv);
}

PRBool
cert_HasCriticalExtension (CERTCertExtension **extensions)
{
    CERTCertExtension **exts;
    CERTCertExtension *ext = NULL;
    PRBool hasCriticalExten = PR_FALSE;
    
    exts = extensions;
    
    if (exts) {
	while ( *exts ) {
	    ext = *exts;
	    /* If the criticality is omitted, it's non-critical */
	    if (ext->critical.data && ext->critical.data[0] == 0xff) {
		hasCriticalExten = PR_TRUE;
		break;
	    }
	    exts++;
	}
    }
    return (hasCriticalExten);
}

PRBool
cert_HasUnknownCriticalExten (CERTCertExtension **extensions)
{
    CERTCertExtension **exts;
    CERTCertExtension *ext = NULL;
    PRBool hasUnknownCriticalExten = PR_FALSE;
    
    exts = extensions;
    
    if (exts) {
	while ( *exts ) {
	    ext = *exts;
	    /* If the criticality is omitted, it's non-critical.
	       If an extension is critical, make sure that we know
	       how to process the extension.
             */
	    if (ext->critical.data && ext->critical.data[0] == 0xff) {
		if (SECOID_KnownCertExtenOID (&ext->id) == PR_FALSE) {
		    hasUnknownCriticalExten = PR_TRUE;
		    break;
		}
	    }
	    exts++;
	}
    }
    return (hasUnknownCriticalExten);
}
