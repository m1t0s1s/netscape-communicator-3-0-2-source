/*
 * support for fetching certs from ldap
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: certldap.c,v 1.3.2.1 1997/05/24 00:23:39 jwz Exp $
 */

#include "xp.h"
#include "cert.h"
#include "net.h"
#include "dirprefs.h"
#include "ldap.h"
#include "secnav.h"
#include "certldap.h"
#include "secitem.h"
#include "xpgetstr.h"

extern int MK_OUT_OF_MEMORY;
extern int XP_DIRECTORY_PASSWORD;

CertLdapConnData *
SECNAV_CertLdapLoad(URL_Struct *urlStr)
{
    CertLdapConnData *connData = NULL;
    char *url;
    LDAP *ld;
    LBER_SOCKET socket;
    int ret;
    CertLdapOpDesc *op;

    op = (CertLdapOpDesc *)urlStr->sec_private;
    
    /* it is a bogus caller if sec_private is NULL */
    if ( op == NULL ) {
	return(NULL);
    }
    
    /* allocate connection data */
    connData = PORT_ArenaZAlloc(op->arena, sizeof(CertLdapConnData));
    
    if ( connData == NULL ) {
	PORT_SetError(MK_OUT_OF_MEMORY);
	goto loser;
    }

    connData->urlStr = urlStr;
    connData->op = op;

    op->connData = connData;
    
    connData->ld = ldap_init(op->servername, op->serverport);
    if ( connData->ld == NULL ) {
	goto loser;
    }

    ret = DIR_SetupSecureConnection(connData->ld);
    if ( ! op->isSecure ) {
	ldap_set_option (connData->ld, LDAP_OPT_SSL, LDAP_OPT_OFF);
    }

    /* lets try anonymous first */
    connData->msgid = ldap_simple_bind(connData->ld, NULL, NULL);

    if ( connData->msgid == -1 ) {
	goto loser;
    }
    
    ret = ldap_get_option(connData->ld, LDAP_OPT_DESC, (void *)&socket);
    if ( ret != LDAP_SUCCESS ) {
	goto loser;
    }
    
    connData->fd = (unsigned long)socket;
    
    connData->state = certLdapBindWait;
done:
    return(connData);

loser:
    if ( op->cb != NULL ) {
	if ( connData != NULL ) {
	    connData->state = certLdapError;
	}
	
	op->rv = SECFailure;
	(* op->cb)(op);
    }
    return(NULL);
}

static char *put_cert_attr[] = {
    NULL, NULL
};

static char *get_cert_attr[] = {
    NULL, ATTR_USER_CERT, ATTR_SMIME_CERT, NULL
};

static char *
buildFilter(CertLdapOpDesc *op)
{
    int i;
    char *filter;
    
    PORT_Assert(op->searchType == certLdapSearchMail);
    PORT_Assert(op->numnames > 0);

    if ( op->numnames == 1 ) {
	filter = PR_smprintf("(%s=%s)", op->mailAttrName, op->names[0]);
    } else {
	filter = PR_smprintf("(|(%s=%s)", op->mailAttrName, op->names[0]);
	if ( filter == NULL ) {
	    return(NULL);
	}
	
	for ( i = 1; i < op->numnames; i++ ) {
	    filter = PR_sprintf_append(filter, "(%s=%s)", op->mailAttrName,
				       op->names[i]);
	    if ( filter == NULL ) {
		return(NULL);
	    }
	}
	filter = PR_sprintf_append(filter, ")");
    }
    
    if ( filter == NULL ) {
	return(NULL);
    }

    return(filter);
}

static SECStatus
doPost(CertLdapConnData *connData)
{
    CertLdapOpDesc *op;
    SECStatus rv;
    LDAPMod certMod;
    struct berval certval;
    struct berval *certvals[2];
    LDAPMod *mods[2];
    
    op = connData->op;

    certMod.mod_type = ATTR_SMIME_CERT;
    certMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    certval.bv_val = (char *)op->postData.data;
    certval.bv_len = op->postData.len;
    certvals[0] = &certval;
    certvals[1] = NULL;
    certMod.mod_bvalues = certvals;

    mods[0] = &certMod;
    mods[1] = NULL;
    
    connData->msgid = ldap_modify(connData->ld, connData->postdn, mods);
    if ( connData->msgid == -1 ) {
	return(SECFailure);
    }

    return(SECSuccess);
}

static SECStatus
doSearch(CertLdapConnData *connData)
{
    char *filter;
    CertLdapOpDesc *op;
    SECStatus rv;
    char **attr;
    
    op = connData->op;
    
    /* search for the entry */
    filter = buildFilter(op);
    if ( filter == NULL ) {
	goto loser;
    }
    if ( op->type == certLdapPostCert ) {
	PORT_Assert(op->numnames == 1);
	attr = put_cert_attr;
    } else {
	attr = get_cert_attr;
    }
    attr[0] = op->mailAttrName;
    
    connData->msgid = ldap_search(connData->ld, op->base,
				  LDAP_SCOPE_SUBTREE, filter, attr, 0);
    PORT_Free(filter);
    if ( connData->msgid == -1 ) {
	goto loser;
    }

    return(SECSuccess);
loser:
    return(SECFailure);
}

static SECStatus
doPasswordBind(CertLdapConnData *connData)
{
    CertLdapOpDesc *op;
    SECStatus rv;
    char *pw;
    
    op = connData->op;
    
    pw = FE_PromptPassword(((MWContext *)op->window), XP_GetString(XP_DIRECTORY_PASSWORD));
    
    if ( pw == NULL ) {
	return(SECFailure);
    }

    /* lets try anonymous first */
    connData->msgid = ldap_simple_bind(connData->ld, connData->postdn, pw);

    SECNAV_ZeroPassword(pw);
    PORT_Free(pw);

    if ( connData->msgid == -1 ) {
	return(SECFailure);
    }

    return(SECSuccess);
}

int
SECNAV_CertLdapProcess(CertLdapConnData *connData)
{
    int ret;
    LDAPMessage *result;
    LDAPMessage *entry;
    struct timeval zerotime;
    struct berval **listOfCerts;
    SECItem derCert;
    char *dn;
    char **vals;
    CertLdapOpDesc *op;
    int i;
    SECStatus rv;
    int err;
    
    op = connData->op;
    
    zerotime.tv_sec = zerotime.tv_usec = 0L;
    
    while ( ( ret = ldap_result(connData->ld, connData->msgid,
				0, &zerotime, &result) ) > 0 ) {

	if ( ret != LDAP_RES_SEARCH_ENTRY ) {
	    /* ldap_result2error() doesn't work right if the result
	     * is not the final one from a search
	     */
	    if ( ( err = ldap_result2error(connData->ld, result, 0) ) 
		!= LDAP_SUCCESS ) {
		if ( ( ( connData->state == certLdapPostWait ) ||
		      ( connData->state == certLdapPWBindWait ) ) &&
		    ( ( err == LDAP_INSUFFICIENT_ACCESS ) ||
		     ( err == LDAP_INVALID_CREDENTIALS ) ) ) {
		    /* need to re-bind with password to write to
		     * the directory
		     */
		    rv = doPasswordBind(connData);
		    if ( rv != SECSuccess ) {
			goto loser;
		    }
		    connData->state = certLdapPWBindWait;
		    continue;
		} else {
		    goto loser;
		}
	    }
	}

	switch (connData->state) {
	  case certLdapPWBindWait:
	    PORT_Assert(ret == LDAP_RES_BIND);
	    PORT_Assert(op->type == certLdapPostCert);

	    rv = doPost(connData);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	    connData->state = certLdapPostWait;
	    break;
	  case certLdapBindWait:
	    PORT_Assert(ret == LDAP_RES_BIND);
	    
	    rv = doSearch(connData);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	    
	    connData->state = certLdapSearchWait;
	    break;
	  case certLdapSearchWait:
	    PORT_Assert( ( ret == LDAP_RES_SEARCH_RESULT ) ||
			( ret == LDAP_RES_SEARCH_ENTRY ) );
	    if ( op->type == certLdapPostCert ) {
		if ( connData->postdn == NULL ) {
		    /* get DN from result, and then post the new cert */
		    entry = ldap_first_entry(connData->ld, result);
		    if ( entry == NULL ) {
			goto loser;
		    }
		    dn = ldap_get_dn(connData->ld, entry);
		    if ( dn == NULL ) {
			goto loser;
		    }
		    connData->postdn = PORT_ArenaStrdup(op->arena, dn);
		    if ( connData->postdn == NULL ) {
			goto loser;
		    }
		}
		
		if ( ret == LDAP_RES_SEARCH_RESULT ) {
		    /* wait til we get the last search result before posting*/
		    rv = doPost(connData);
		    if ( rv != SECSuccess ) {
			goto loser;
		    }
		    connData->state = certLdapPostWait;
		}
	    } else {
		/* collect the certs from the results */
		entry = ldap_first_entry(connData->ld, result);
		while ( entry ) {
		    listOfCerts = ldap_get_values_len(connData->ld, entry,
						      ATTR_SMIME_CERT);
    
		    if ( listOfCerts == NULL ) {
			listOfCerts = ldap_get_values_len(connData->ld, entry,
							  ATTR_USER_CERT);
			if ( listOfCerts == NULL ) {
			    goto endloop;
			}
		    }
		    
		    vals = ldap_get_values(connData->ld, entry,
					   op->mailAttrName);
		    
		    if ( ( vals[0] == NULL ) || ( listOfCerts[0] == NULL ) ) {
			goto endloop;
		    }
		    
		    for ( i = 0; i < op->numnames; i++ ) {
			/* look for matching email address slot and drop the
			 * cert into it
			 */
			if ( PORT_Strcmp(vals[0], op->names[i])== 0){
			    derCert.len = listOfCerts[0]->bv_len;
			    derCert.data = (unsigned char *)listOfCerts[0]->bv_val;
			    rv = SECITEM_CopyItem(op->arena,
						  &op->rawcerts[i],
						  &derCert);
			    break;
			}
		    }
		    
endloop:
		    entry = ldap_next_entry(connData->ld, entry);
		}
		if ( ret == LDAP_RES_SEARCH_RESULT ) {
		    ret = ldap_unbind(connData->ld);
		    
		    connData->state = certLdapDone;
		    /* we are done, so set the return value to cause us to
		     * stop, and call the callback
		     */
		    ret = 1;
		
		    if ( op->cb != NULL ) {
			op->rv = SECSuccess;
			(* op->cb)(op);
		    }
		    goto done;
		}
	    }
	    break;
	  case certLdapPostWait:
	    /* post is done */
	    PORT_Assert(ret == LDAP_RES_MODIFY);
	    ret = ldap_unbind(connData->ld);
	    connData->state = certLdapDone;
	    ret = 1;

	    if ( op->cb != NULL ) {
		op->rv = SECSuccess;
		(* op->cb)(op);
	    }
	    goto done;
	}
    }
    
    if ( ret == -1 ) {
	goto loser;
    }
    /* ret == 0 */
    /* not ready yet */
    goto done;

loser:
    connData->state = certLdapError;
    if ( op->cb != NULL ) {
	op->rv = SECFailure;
	(* op->cb)(op);
    }
    /* try to close down the connection */
    SECNAV_CertLdapInterrupt(connData);
    return(-1);
done:
    return(ret);
}

int
SECNAV_CertLdapInterrupt(CertLdapConnData *connData)
{
    int ret;
    CertLdapOpDesc *op;
    int err = 0;
    
    op = connData->op;
    ret = ldap_abandon(connData->ld, connData->msgid);
    if ( ret != LDAP_SUCCESS ) {
	err = -1;
    }
    
    ret = ldap_unbind(connData->ld);
    if ( ret != LDAP_SUCCESS ) {
	err = -1;
    }

    return(err);
}
