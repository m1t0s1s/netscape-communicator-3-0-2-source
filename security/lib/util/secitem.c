/*
 * Support routines for SECItem data structure.
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: secitem.c,v 1.9 1996/10/14 10:04:21 jsw Exp $
 */

#include "seccomon.h"
#include "base64.h"

extern int SEC_ERROR_NO_MEMORY;

SECComparison
SECITEM_CompareItem(SECItem *a, SECItem *b)
{
    unsigned m;
    SECComparison rv;

    m = ( ( a->len < b->len ) ? a->len : b->len );
    
    rv = (SECComparison) PORT_Memcmp(a->data, b->data, m);
    if (rv) {
	return rv;
    }
    if (a->len < b->len) {
	return SECLessThan;
    }
    if (a->len == b->len) {
	return SECEqual;
    }
    return SECGreaterThan;
}

SECItem *
SECITEM_DupItem(SECItem *from)
{
    SECItem *to;
    
    if ( from == NULL ) {
	return(NULL);
    }
    
    to = (SECItem *)PORT_Alloc(sizeof(SECItem));
    if ( to == NULL ) {
	return(NULL);
    }

    to->data = (unsigned char *)PORT_Alloc(from->len);
    if ( to->data == NULL ) {
	PORT_Free(to);
	return(NULL);
    }

    to->len = from->len;
    PORT_Memcpy(to->data, from->data, to->len);
    
    return(to);
}

SECStatus
SECITEM_CopyItem(PRArenaPool *arena, SECItem *to, SECItem *from)
{
    if (from->data && from->len) {
	if ( arena ) {
	    to->data = (unsigned char*) PORT_ArenaAlloc(arena, from->len);
	} else {
	    to->data = (unsigned char*) PORT_Alloc(from->len);
	}
	
	if (!to->data) {
	    return SECFailure;
	}
	PORT_Memcpy(to->data, from->data, from->len);
	to->len = from->len;
    } else {
	to->data = 0;
	to->len = 0;
    }
    return SECSuccess;
}

void
SECITEM_FreeItem(SECItem *zap, PRBool freeit)
{
    if (zap) {
	PORT_Free(zap->data);
	zap->data = 0;
	zap->len = 0;
	if (freeit) {
	    PORT_Free(zap);
	}
    }
}

void
SECITEM_ZfreeItem(SECItem *zap, PRBool freeit)
{
    if (zap) {
	PORT_ZFree(zap->data, zap->len);
	zap->data = 0;
	zap->len = 0;
	if (freeit) {
	    PORT_ZFree(zap, sizeof(SECItem));
	}
    }
}

SECStatus
ATOB_ConvertAsciiToItem (SECItem *result, char *istr)
{
    result->data = ATOB_AsciiToData (istr, &(result->len));
    if (result->data == NULL)
	return SECFailure;
    else
	return SECSuccess;
}

char *
BTOA_ConvertItemToAscii (SECItem *isrc)
{
    return BTOA_DataToAscii (isrc->data, isrc->len);
}
