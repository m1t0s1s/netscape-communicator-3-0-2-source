#include "secnew.h"

static SECStatus
Decode(PRArenaPool *arena, SECArb *arb, BERTemplate *tmpl, void *addr)
{
    SECItem *it;
    SECStatus status;
    long kind;
    
    kind = tmpl->kind;
    
    /* Identify next thing to be captured */
    status = CaptureThing(buf, buflen, &len, &item, &itemLen, kind);
    if (status || ( item == 0 ) ) { /* failure or missing optional */
	return status;
    }

    if ( kind & DER_CONTEXT_SPECIFIC ) {
	if ( kind & DER_EXPLICIT ) {
	    unsigned dummylen;
	    if ( ! ( kind & DER_ANY ) ) {
		
		kind = dt->arg;
		status = CaptureThing(item, itemLen, &dummylen, &item,
				      &itemLen, kind);
		if (status || ( item == 0 ) ) { /* failure or missing optional */
		    return status;
		}
	    }
	} else {
	    /* rewrite the tag for IMPLICIT CONTEXT-SPECIFIC */
	    kind = dt->arg;
	}
    };
	
    kind = kind & ( ~(DER_FORCE | DER_OPTIONAL) );
    
    if ( kind & DER_DERPTR ) {
	/* save the current DER item */
	it = (SECItem *)addr;

	if ( kind & DER_OUTER ) {
	    it->data = buf;
	    it->len = len;
	} else {
	    it->data = item;
	    it->len = itemLen;
	}
	
	return( SECSuccess );
    }
    
    *bufp = buf + len;
    *buflenp = buflen - len;

    /* DER_ANY may have other bits set for non-universal types */
    if ( kind & DER_ANY ) {
	it = (SECItem*) addr;
	if ( kind & DER_EXPLICIT ) {
	    /* point to item in place, includeing its DER header */
	    it->data = item;
	    it->len = itemLen;
	} else {
	    it->data = (unsigned char*) buf;
	    it->len = len;
	}
	
	return(SECSuccess);
    }

    /* Figure out how to decode it */
    switch (kind) {
      case DER_SEQUENCE:
      case DER_SET:
	/* Capture items and fill in thing */
	for (t = dt + 1; t->kind; t++) {
	    void** taddr = (void**) ((char*)addr + t->offset);
	    status = Decode(arena, &item, &itemLen, t, taddr);
	    if (status) {
		return status;
	    }
	}
	break;

      case DER_POINTER:
	/* Allocate memory to hold struct */
	a = PORT_ArenaZAlloc(arena, dt->arg);
	if (!a) {
	    /* Out of memory */
	    goto loser;
	}
	*(void**)addr = a;

	/* Decode into allocated struct */
	a = (void**) ((char*)a + dt->sub->offset);
	status = Decode(arena, &buf, &buflen, dt->sub, a);
	break;

      case DER_INLINE:
	a = (void**) ((char*)addr + dt->sub->offset);
	status = Decode(arena, &buf, &buflen, dt->sub, a);
	break;

      case DER_INDEFINITE | DER_SEQUENCE:
      case DER_INDEFINITE | DER_SET:
      case DER_INDEFINITE | DER_OCTET_STRING:
	/* Count items to be captured */
	count = 1;
	saveItem = item;
	saveItemLen = itemLen;
	while (itemLen) {
	    unsigned char *c;
	    unsigned clen, thingLen;
	    status = CaptureThing(item, itemLen, &thingLen, &c, &clen,
				  dt->sub->kind);
	    if (status) {
		return status;
	    }

	    /* don't count 00 00 end of indefinite length */
	    if ( clen | item[0] | item[1] ) {
		count++;
	    }
	    
	    item += thingLen;
	    itemLen -= thingLen;
	}

	/* Allocate array of pointers to hold items */
	indp = (void**) PORT_ArenaZAlloc(arena, count * sizeof(void*));
	if (!indp) {
	    goto loser;
	}
	*(void **)addr = (void*) indp;

	/* Decode each item in the sequence/set */
	item = saveItem;
	itemLen = saveItemLen;

	/* adjust for sub-template offset */
	indp = (void**) ((char*)indp + dt->sub->offset);

	while (itemLen) {
	    unsigned char *c;
	    unsigned clen, thingLen;
	    status = CaptureThing(item, itemLen, &thingLen, &c, &clen,
				  dt->sub->kind);
	    if (status) {
		return status;
	    }

	    /* skip 00 00 end of indefinite length */
	    if ( clen | item[0] | item[1] ) {
		status = Decode(arena, &item, &itemLen, dt->sub, indp);
		if (status) {
		    return status;
		}
		indp++;
	    } else {
		item += thingLen;
		itemLen -= thingLen;
	    }
	    
	}
	break;

      case DER_BIT_STRING:
	it = (SECItem*) addr;
	it->data = item+1;
	it->len = ((itemLen-1) << 3) - item[0];
	break;

      case DER_SKIP:
	break;

      default:
	it = (SECItem*) addr;
	it->data = item;
	it->len = itemLen;
	break;
    }
    return SECSuccess;

  loser:
    return SECFailure;
}

SECStatus
BER_Decode(PRArenaPool *arena, void *dest, BERTemplate *t, SECArb *arb)
{
    SECStatus rv;

    dest = (void**) ((char*)dest + t->offset);
    
    rv = Decode(arena, arb, t, dest);
    return rv;
}
