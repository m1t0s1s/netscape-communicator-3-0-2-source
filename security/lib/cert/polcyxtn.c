/*
 * Support for various policy related extensions
 *
 * Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: polcyxtn.c,v 1.3 1997/05/22 00:48:29 jsw Exp $
 */

#include "seccomon.h"
#include "secport.h"
#include "secder.h"
#include "cert.h"
#include "secoid.h"

extern int SEC_ERROR_NO_MEMORY;

DERTemplate CERTNoticeReferenceTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTNoticeReference) },
    { DER_IA5_STRING,
	  offsetof(CERTNoticeReference, organization), },
    { DER_INDEFINITE | DER_SEQUENCE,
	  offsetof(CERTNoticeReference, noticeNumbers),
	  SECIntegerTemplate, },
    { 0, }
};

/* this template can not be encoded because of the option inline */
DERTemplate CERTUserNoticeTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTUserNotice) },
    { DER_OPTIONAL | DER_INLINE,
	  offsetof(CERTUserNotice, noticeReference),
          CERTNoticeReferenceTemplate },
    { DER_OPTIONAL | DER_ANY,
	  offsetof(CERTUserNotice, displayText) },
    { 0, }
};

DERTemplate CERTPolicyQualifierTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTPolicyQualifier) },
    { DER_OBJECT_ID,
	  offsetof(CERTPolicyQualifier, qualifierID) },
    { DER_ANY,
	  offsetof(CERTPolicyQualifier, qualifierValue) },
    { 0, }
};

DERTemplate CERTPolicyInfoTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTPolicyInfo) },
    { DER_OBJECT_ID,
	  offsetof(CERTPolicyInfo, policyID) },
    { DER_INDEFINITE | DER_SEQUENCE,
	  offsetof(CERTPolicyInfo, policyQualifiers),
	  CERTPolicyQualifierTemplate, },
    { 0, }
};

DERTemplate CERTCertificatePoliciesTemplate[] = {
    { DER_INDEFINITE | DER_SEQUENCE,
	  offsetof(CERTCertificatePolicies, policyInfos),
	  CERTPolicyInfoTemplate, sizeof(CERTCertificatePolicies) }
};

static void
breakLines(char *string)
{
    char *tmpstr;
    char *lastspace = NULL;
    int curlen = 0;
    int c;
    
    tmpstr = string;

    while ( ( c = *tmpstr ) != '\0' ) {
	switch ( c ) {
	  case ' ':
	    lastspace = tmpstr;
	    break;
	  case '\n':
	    lastspace = NULL;
	    curlen = 0;
	    break;
	}
	
	if ( ( curlen >= 55 ) && ( lastspace != NULL ) ) {
	    *lastspace = '\n';
	    curlen = ( tmpstr - lastspace );
	    lastspace = NULL;
	}
	
	curlen++;
	tmpstr++;
    }
    
    return;
}

CERTCertificatePolicies *
CERT_DecodeCertificatePoliciesExtension(SECItem *extnValue)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    CERTCertificatePolicies *policies;
    CERTPolicyInfo **policyInfos, *policyInfo;
    CERTPolicyQualifier **policyQualifiers, *policyQualifier;
    
    /* make a new arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	goto loser;
    }

    /* allocate the certifiate policies structure */
    policies = (CERTCertificatePolicies *)
	PORT_ArenaZAlloc(arena, sizeof(CERTCertificatePolicies));
    
    if ( policies == NULL ) {
	goto loser;
    }
    
    policies->arena = arena;

    /* decode the policy info */
    rv = DER_Decode(arena, policies, CERTCertificatePoliciesTemplate,
		    extnValue);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* initialize the oid tags */
    policyInfos = policies->policyInfos;
    while (*policyInfos != NULL ) {
	policyInfo = *policyInfos;
	policyInfo->oid = SECOID_FindOIDTag(&policyInfo->policyID);
	policyQualifiers = policyInfo->policyQualifiers;
	while ( *policyQualifiers != NULL ) {
	    policyQualifier = *policyQualifiers;
	    policyQualifier->oid =
		SECOID_FindOIDTag(&policyQualifier->qualifierID);
	    policyQualifiers++;
	}
	policyInfos++;
    }

    return(policies);
    
loser:
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

void
CERT_DestroyCertificatePoliciesExtension(CERTCertificatePolicies *policies)
{
    if ( policies != NULL ) {
	PORT_FreeArena(policies->arena, PR_FALSE);
    }
    return;
}


CERTUserNotice *
CERT_DecodeUserNotice(SECItem *noticeItem)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    CERTUserNotice *userNotice;
    
    /* make a new arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	goto loser;
    }

    /* allocate the userNotice structure */
    userNotice = (CERTUserNotice *)PORT_ArenaZAlloc(arena,
						    sizeof(CERTUserNotice));
    
    if ( userNotice == NULL ) {
	goto loser;
    }
    
    userNotice->arena = arena;

    /* decode the user notice */
    rv = DER_Decode(arena, userNotice, CERTUserNoticeTemplate, noticeItem);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    return(userNotice);
    
loser:
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

void
CERT_DestroyUserNotice(CERTUserNotice *userNotice)
{
    if ( userNotice != NULL ) {
	PORT_FreeArena(userNotice->arena, PR_FALSE);
    }
    return;
}

static CERTPolicyStringCallback policyStringCB = NULL;
static void *policyStringCBArg = NULL;

void
CERT_SetCAPolicyStringCallback(CERTPolicyStringCallback cb, void *cbarg)
{
    policyStringCB = cb;
    policyStringCBArg = cbarg;
    return;
}

static char *
stringFromUserNotice(SECItem *noticeItem)
{
    SECItem *org;
    unsigned int len, headerlen;
    char *stringbuf;
    CERTUserNotice *userNotice;
    char *policystr;
    char *retstr = NULL;
    SECItem *displayText;
    SECItem **noticeNumbers;
    unsigned int strnum;
    
    /* decode the user notice */
    userNotice = CERT_DecodeUserNotice(noticeItem);
    if ( userNotice == NULL ) {
	return(NULL);
    }
    
    org = &userNotice->noticeReference.organization;
    if ( ( org->len != 0 ) && ( policyStringCB != NULL ) ) {
	/* has a noticeReference */

	/* extract the org string */
	len = org->len;
	stringbuf = PORT_Alloc(len + 1);
	if ( stringbuf != NULL ) {
	    PORT_Memcpy(stringbuf, org->data, len);
	    stringbuf[len] = '\0';

	    noticeNumbers = userNotice->noticeReference.noticeNumbers;
	    while ( *noticeNumbers != NULL ) {
		/* XXX - only one byte integers right now*/
		strnum = (*noticeNumbers)->data[0];
		policystr = (* policyStringCB)(stringbuf,
					       strnum,
					       policyStringCBArg);
		if ( policystr != NULL ) {
		    if ( retstr != NULL ) {
			retstr = PR_sprintf_append(retstr, "\n%s", policystr);
		    } else {
			retstr = PR_sprintf_append(retstr, "%s", policystr);
		    }

		    PORT_Free(policystr);
		}
		
		noticeNumbers++;
	    }

	    PORT_Free(stringbuf);
	}
    }

    if ( retstr == NULL ) {
	if ( userNotice->displayText.len != 0 ) {
	    displayText = &userNotice->displayText;

	    if ( displayText->len > 2 ) {
		if ( displayText->data[0] == DER_VISIBLE_STRING ) {
		    headerlen = 2;
		    if ( displayText->data[1] & 0x80 ) {
			/* multibyte length */
			headerlen += ( displayText->data[1] & 0x7f );
		    }

		    len = displayText->len - headerlen;
		    retstr = PORT_Alloc(len + 1);
		    if ( retstr != NULL ) {
			PORT_Memcpy(retstr, &displayText->data[headerlen],len);
			retstr[len] = '\0';
		    }
		}
	    }
	}
    }
    
    CERT_DestroyUserNotice(userNotice);
    
    return(retstr);
}

char *
CERT_GetCertCommentString(CERTCertificate *cert)
{
    char *retstring = NULL;
    SECStatus rv;
    SECItem policyItem;
    CERTCertificatePolicies *policies = NULL;
    CERTPolicyInfo **policyInfos;
    CERTPolicyQualifier **policyQualifiers, *qualifier;

    policyItem.data = NULL;
    
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_CERTIFICATE_POLICIES,
				&policyItem);
    if ( rv != SECSuccess ) {
	goto nopolicy;
    }

    policies = CERT_DecodeCertificatePoliciesExtension(&policyItem);
    if ( policies == NULL ) {
	goto nopolicy;
    }

    policyInfos = policies->policyInfos;
    /* search through policyInfos looking for the verisign policy */
    while (*policyInfos != NULL ) {
	if ( (*policyInfos)->oid == SEC_OID_VERISIGN_USER_NOTICES ) {
	    policyQualifiers = (*policyInfos)->policyQualifiers;
	    /* search through the policy qualifiers looking for user notice */
	    while ( *policyQualifiers != NULL ) {
		qualifier = *policyQualifiers;
		if ( qualifier->oid == SEC_OID_PKIX_USER_NOTICE_QUALIFIER ) {
		    retstring =
			stringFromUserNotice(&qualifier->qualifierValue);
		    break;
		}

		policyQualifiers++;
	    }
	    break;
	}
	policyInfos++;
    }

nopolicy:
    if ( policyItem.data != NULL ) {
	PORT_Free(policyItem.data);
    }

    if ( policies != NULL ) {
	CERT_DestroyCertificatePoliciesExtension(policies);
    }
    
    if ( retstring == NULL ) {
	retstring = CERT_FindNSStringExtension(cert,
					       SEC_OID_NS_CERT_EXT_COMMENT);
    }
    
    if ( retstring != NULL ) {
	breakLines(retstring);
    }
    
    return(retstring);
}

typedef struct {
    PRArenaPool *arena;
    SECItem **oids;
} CERTOidSequence;

DERTemplate CERTOidSeqTemplate[] = {
    { DER_INDEFINITE | DER_SEQUENCE,
	  offsetof(CERTOidSequence, oids),
	  SECObjectIDTemplate, sizeof(CERTOidSequence) }
};

CERTOidSequence *
CERT_DecodeOidSequence(SECItem *seqItem)
{
    PRArenaPool *arena = NULL;
    SECStatus rv;
    CERTOidSequence *oidSeq;
    
    /* make a new arena */
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	goto loser;
    }

    /* allocate the userNotice structure */
    oidSeq = (CERTOidSequence *)PORT_ArenaZAlloc(arena,
						 sizeof(CERTOidSequence));
    
    if ( oidSeq == NULL ) {
	goto loser;
    }
    
    oidSeq->arena = arena;

    /* decode the user notice */
    rv = DER_Decode(arena, oidSeq, CERTOidSeqTemplate, seqItem);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    return(oidSeq);
    
loser:
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

void
CERT_DestroyOidSequence(CERTOidSequence *oidSeq)
{
    if ( oidSeq != NULL ) {
	PORT_FreeArena(oidSeq->arena, PR_FALSE);
    }
    return;
}

PRBool
CERT_GovtApprovedBitSet(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem extItem;
    CERTOidSequence *oidSeq = NULL;
    PRBool ret;
    SECItem **oids;
    SECItem *oid;
    SECOidTag oidTag;
    
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_EXT_KEY_USAGE, &extItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    oidSeq = CERT_DecodeOidSequence(&extItem);
    if ( oidSeq == NULL ) {
	goto loser;
    }

    oids = oidSeq->oids;
    while ( *oids != NULL ) {
	oid = *oids;
	
	oidTag = SECOID_FindOIDTag(oid);
	
	if ( oidTag == SEC_OID_NS_KEY_USAGE_GOVT_APPROVED ) {
	    goto success;
	}
	
	oids++;
    }

loser:
    ret = PR_FALSE;
    goto done;
success:
    ret = PR_TRUE;
done:
    if ( oidSeq != NULL ) {
	CERT_DestroyOidSequence(oidSeq);
    }
    
    return(ret);
}
