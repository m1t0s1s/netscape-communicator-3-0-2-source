#include "cert.h"
#include "secoid.h"
#include "secder.h"	/* XXX remove this when remove the DERTemplates */
#include "secasn1.h"
#include "secitem.h"
#include <stdarg.h>

extern int SEC_ERROR_INVALID_ARGS;


/* XXX This template needs to go away in favor of the new SEC_ASN1 version. */
static DERTemplate certAVATemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(CERTAVA) },
    { DER_OBJECT_ID,
	  offsetof(CERTAVA,type), },
    { DER_ANY,
	  offsetof(CERTAVA,value), },
    { 0, }
};

static const SEC_ASN1Template cert_AVATemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTAVA) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(CERTAVA,type), },
    { SEC_ASN1_ANY,
	  offsetof(CERTAVA,value), },
    { 0, }
};

const SEC_ASN1Template CERT_RDNTemplate[] = {
    { SEC_ASN1_SET_OF,
	  offsetof(CERTRDN,avas), cert_AVATemplate, sizeof(CERTRDN) }
};


static int
CountArray(void **array)
{
    int count = 0;
    if (array) {
	while (*array++) {
	    count++;
	}
    }
    return count;
}

static void
**AddToArray(PRArenaPool *arena, void **array, void *element)
{
    unsigned count;
    void **ap;

    /* Count up number of slots already in use in the array */
    count = 0;
    ap = array;
    if (ap) {
	while (*ap++) {
	    count++;
	}
    }

    if (array) {
	array = (void**) PORT_ArenaGrow(arena, array,
					(count + 1) * sizeof(void *),
					(count + 2) * sizeof(void *));
    } else {
	array = (void**) PORT_ArenaAlloc(arena, (count + 2) * sizeof(void *));
    }
    if (array) {
	array[count] = element;
	array[count+1] = 0;
    }
    return array;
}

#if 0
static void
**RemoveFromArray(void **array, void *element)
{
    unsigned count;
    void **ap;
    int slot;

    /* Look for element */
    ap = array;
    if (ap) {
	count = 1;			/* count the null at the end */
	slot = -1;
	for (; *ap; ap++, count++) {
	    if (*ap == element) {
		/* Found it */
		slot = ap - array;
	    }
	}
	if (slot >= 0) {
	    /* Found it. Squish array down */
	    PORT_Memmove((void*) (array + slot), (void*) (array + slot + 1),
		       (count - slot - 1) * sizeof(void*));
	    /* Don't bother reallocing the memory */
	}
    }
    return array;
}
#endif /* 0 */

SECOidTag
CERT_GetAVATag(CERTAVA *ava)
{
    SECOidData *oid;
    if (!ava->type.data) return -1;

    oid = SECOID_FindOID(&ava->type);
    
    if ( oid ) {
	return(oid->offset);
    }
    return -1;
}

static SECStatus
SetupAVAType(PRArenaPool *arena, SECOidTag type, SECItem *it, unsigned *maxLenp)
{
    unsigned char *oid;
    unsigned oidLen;
    unsigned char *cp;
    unsigned maxLen;
    SECOidData *oidrec;

    oidrec = SECOID_FindOIDByTag(type);
    if (oidrec == NULL)
	return SECFailure;

    oid = oidrec->oid.data;
    oidLen = oidrec->oid.len;

    switch (type) {
      case SEC_OID_AVA_COUNTRY_NAME:
	maxLen = 2;
	break;
      case SEC_OID_AVA_ORGANIZATION_NAME:
	maxLen = 64;
	break;
      case SEC_OID_AVA_COMMON_NAME:
	maxLen = 64;
	break;
      case SEC_OID_AVA_LOCALITY:
	maxLen = 128;
	break;
      case SEC_OID_AVA_STATE_OR_PROVINCE:
	maxLen = 128;
	break;
      case SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME:
	maxLen = 64;
	break;
      case SEC_OID_PKCS9_EMAIL_ADDRESS:
	maxLen = 128;
	break;
      case SEC_OID_RFC1274_UID:
	maxLen = 256;  /* RFC 1274 specifies 256 */
	break;
      case SEC_OID_RFC1274_MAIL:
	maxLen = 256;  /* RFC 1274 specifies 256 */
	break; 
      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    it->data = cp = (unsigned char*) PORT_ArenaAlloc(arena, oidLen);
    if (cp == NULL) {
	return SECFailure;
    }
    it->len = oidLen;
    PORT_Memcpy(cp, oid, oidLen);
    *maxLenp = maxLen;
    return SECSuccess;
}

static SECStatus
SetupAVAValue(PRArenaPool *arena, int valueType, char *value, SECItem *it,
	      unsigned maxLen)
{
    unsigned valueLen, valueLenLen, total;
    unsigned char *cp;

    switch (valueType) {
      case DER_PRINTABLE_STRING:
      case DER_IA5_STRING:
      case DER_T61_STRING:
	valueLen = PORT_Strlen(value);
	break;
      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    if (valueLen > maxLen) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    valueLenLen = DER_LengthLength(valueLen);
    total = 1 + valueLenLen + valueLen;
    it->data = cp = (unsigned char*) PORT_ArenaAlloc(arena, total);
    if (!cp) {
	return SECFailure;
    }
    it->len = total;
    cp = (unsigned char*) DER_StoreHeader(cp, valueType, valueLen);
    PORT_Memcpy(cp, value, valueLen);
    return SECSuccess;
}

CERTAVA *
CERT_CreateAVA(PRArenaPool *arena, SECOidTag kind, int valueType, char *value)
{
    CERTAVA *ava;
    int rv;
    unsigned maxLen;

    ava = (CERTAVA*) PORT_ArenaZAlloc(arena, sizeof(CERTAVA));
    if (ava) {
	rv = SetupAVAType(arena, kind, &ava->type, &maxLen);
	if (rv) {
	    /* Illegal AVA type */
	    return 0;
	}
	rv = SetupAVAValue(arena, valueType, value, &ava->value, maxLen);
	if (rv) {
	    /* Illegal value type */
	    return 0;
	}
    }
    return ava;
}

CERTAVA *
CERT_CopyAVA(PRArenaPool *arena, CERTAVA *from)
{
    CERTAVA *ava;
    int rv;

    ava = (CERTAVA*) PORT_ArenaZAlloc(arena, sizeof(CERTAVA));
    if (ava) {
	rv = SECITEM_CopyItem(arena, &ava->type, &from->type);
	if (rv) goto loser;
	rv = SECITEM_CopyItem(arena, &ava->value, &from->value);
	if (rv) goto loser;
    }
    return ava;

  loser:
    return 0;
}

/************************************************************************/
/* XXX This template needs to go away in favor of the new SEC_ASN1 version. */
static DERTemplate certRDNTemplate[] = {
    { DER_INDEFINITE | DER_SET,
	  offsetof(CERTRDN,avas), certAVATemplate, sizeof(CERTRDN) }
};


CERTRDN *
CERT_CreateRDN(PRArenaPool *arena, CERTAVA *ava0, ...)
{
    CERTAVA *ava;
    CERTRDN *rdn;
    va_list ap;
    unsigned count;
    CERTAVA **avap;

    rdn = (CERTRDN*) PORT_ArenaAlloc(arena, sizeof(CERTRDN));
    if (rdn) {
	/* Count number of avas going into the rdn */
	count = 1;
	va_start(ap, ava0);
	while ((ava = va_arg(ap, CERTAVA*)) != 0) {
	    count++;
	}
	va_end(ap);

	/* Now fill in the pointers */
	rdn->avas = avap =
	    (CERTAVA**) PORT_ArenaAlloc( arena, (count + 1)*sizeof(CERTAVA*));
	if (!avap) {
	    return 0;
	}
	*avap++ = ava0;
	va_start(ap, ava0);
	while ((ava = va_arg(ap, CERTAVA*)) != 0) {
	    *avap++ = ava;
	}
	va_end(ap);
	*avap++ = 0;
    }
    return rdn;
}

SECStatus
CERT_AddAVA(PRArenaPool *arena, CERTRDN *rdn, CERTAVA *ava)
{
    rdn->avas = (CERTAVA**) AddToArray(arena, (void**) rdn->avas, ava);
    return rdn->avas ? SECSuccess : SECFailure;
}

SECStatus
CERT_CopyRDN(PRArenaPool *arena, CERTRDN *to, CERTRDN *from)
{
    CERTAVA **avas, *fava, *tava;
    SECStatus rv;

    /* Copy each ava from from */
    avas = from->avas;
    while ((fava = *avas++) != 0) {
	tava = CERT_CopyAVA(arena, fava);
	if (!tava) return SECFailure;
	rv = CERT_AddAVA(arena, to, tava);
	if (rv) return rv;
    }
    return SECSuccess;
}

/************************************************************************/

/* XXX This template needs to go away in favor of the new SEC_ASN1 version. */
DERTemplate CERTNameTemplate[] = {
    { DER_INDEFINITE | DER_SEQUENCE,
	  offsetof(CERTName,rdns), certRDNTemplate, sizeof(CERTName) }
};

const SEC_ASN1Template CERT_NameTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF,
	  offsetof(CERTName,rdns), CERT_RDNTemplate, sizeof(CERTName) }
};

CERTName *
CERT_CreateName(CERTRDN *rdn0, ...)
{
    CERTRDN *rdn;
    CERTName *name;
    va_list ap;
    unsigned count;
    CERTRDN **rdnp;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena ) {
	return(0);
    }
    
    name = (CERTName*) PORT_ArenaAlloc(arena, sizeof(CERTName));
    if (name) {
	name->arena = arena;
	
	/* Count number of RDNs going into the Name */
	count = 1;
	va_start(ap, rdn0);
	while ((rdn = va_arg(ap, CERTRDN*)) != 0) {
	    count++;
	}
	va_end(ap);

	/* Now fill in the pointers */
	name->rdns = rdnp =
	    (CERTRDN**) PORT_ArenaAlloc(arena, (count + 1) * sizeof(CERTRDN*));
	if (!name->rdns) {
	    goto loser;
	}
	*rdnp++ = rdn0;
	va_start(ap, rdn0);
	while ((rdn = va_arg(ap, CERTRDN*)) != 0) {
	    *rdnp++ = rdn;
	}
	va_end(ap);
	*rdnp++ = 0;
    }
    return name;

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(0);
}

void
CERT_DestroyName(CERTName *name)
{
    if (name && name->arena) {
	PORT_FreeArena(name->arena, PR_FALSE);
    }
}

SECStatus
CERT_AddRDN(CERTName *name, CERTRDN *rdn)
{
    name->rdns = (CERTRDN**) AddToArray(name->arena, (void**) name->rdns, rdn);
    return name->rdns ? SECSuccess : SECFailure;
}

SECStatus
CERT_CopyName(PRArenaPool *arena, CERTName *to, CERTName *from)
{
    CERTRDN **rdns, *frdn, *trdn;
    SECStatus rv;

    CERT_DestroyName(to);
    to->arena = arena;

    /* Copy each rdn from from */
    rdns = from->rdns;
    while ((frdn = *rdns++) != 0) {
	trdn = CERT_CreateRDN(arena, 0);
	if ( trdn == NULL ) {
	    return(SECFailure);
	}
	rv = CERT_CopyRDN(arena, trdn, frdn);
	if (rv) return rv;
	rv = CERT_AddRDN(to, trdn);
	if (rv) return rv;
    }
    return SECSuccess;
}

/************************************************************************/

SECComparison
CERT_CompareAVA(CERTAVA *a, CERTAVA *b)
{
    SECComparison rv;

    rv = SECITEM_CompareItem(&a->type, &b->type);
    if (rv) {
	/*
	** XXX for now we are going to just assume that a bitwise
	** comparison of the value codes will do the trick.
	*/
    }
    rv = SECITEM_CompareItem(&a->value, &b->value);
    return rv;
}

SECComparison
CERT_CompareRDN(CERTRDN *a, CERTRDN *b)
{
    CERTAVA **aavas, *aava;
    CERTAVA **bavas, *bava;
    int ac, bc;
    SECComparison rv = SECEqual;

    aavas = a->avas;
    bavas = b->avas;

    /*
    ** Make sure array of ava's are the same length. If not, then we are
    ** not equal
    */
    ac = CountArray((void**) aavas);
    bc = CountArray((void**) bavas);
    if (ac < bc) return SECLessThan;
    if (ac > bc) return SECGreaterThan;

    for (;;) {
	aava = *aavas++;
	bava = *bavas++;
	if (!aava) {
	    break;
	}
	rv = CERT_CompareAVA(aava, bava);
	if (rv) return rv;
    }
    return rv;
}

SECComparison
CERT_CompareName(CERTName *a, CERTName *b)
{
    CERTRDN **ardns, *ardn;
    CERTRDN **brdns, *brdn;
    int ac, bc;
    SECComparison rv = SECEqual;

    ardns = a->rdns;
    brdns = b->rdns;

    /*
    ** Make sure array of rdn's are the same length. If not, then we are
    ** not equal
    */
    ac = CountArray((void**) ardns);
    bc = CountArray((void**) brdns);
    if (ac < bc) return SECLessThan;
    if (ac > bc) return SECGreaterThan;

    for (;;) {
	ardn = *ardns++;
	brdn = *brdns++;
	if (!ardn) {
	    break;
	}
	rv = CERT_CompareRDN(ardn, brdn);
	if (rv) return rv;
    }
    return rv;
}
