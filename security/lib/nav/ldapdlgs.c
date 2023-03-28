/*
 * Dialogs for fetching certs from ldap
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: ldapdlgs.c,v 1.2.4.1 1997/05/24 00:23:39 jwz Exp $
 */
#include "xp.h"
#include "cert.h"
#include "htmldlgs.h"
#include "statdlg.h"
#include "secnav.h"
#include "dirprefs.h"
#include "xp_list.h"
#include "xpgetstr.h"
#include "ldap.h"
#include "certldap.h"
#include "net.h"
#include "secpkcs7.h"
#include "sechash.h"
#include "secmime.h"

extern int XP_DIALOG_FETCH_TITLE;
extern int XP_DIALOG_FETCH_STRINGS;
extern int XP_DIALOG_FETCH2_STRINGS;
extern int XP_DIALOG_ALL_DIRECTORIES;
extern int XP_DIALOG_FETCH_RESULTS_TITLE;
extern int XP_DIALOG_FETCH_RESULTS_STRINGS;
extern int XP_DIALOG_FETCH_NOT_FOUND_STRINGS;
extern int XP_DIALOG_PUBLISH_CERT_TITLE;
extern int XP_DIALOG_PUBLISH_CERT_STRINGS;
extern int XP_SENDING_TO_DIRECTORY;
extern int XP_SEARCHING_DIRECTORY;
extern int XP_DIRECTORY_ERROR;

static void
certLdapCleanup(CertLdapOpDesc *op)
{
    if ( op->connData != NULL ) {
	/* the connection has been started */
	if ( op->connData->state == certLdapError ) {
	    /* some ldap error - put up a dialog */
	    /* XXX - FE_Alert() */
	} else if ( op->connData->state != certLdapDone ) {
	    /* cancel button was pressed - clean up ldap connection */
	    NET_InterruptSocket(op->connData->fd);
	}
    }

    if ( ( op->connData == NULL ) || ( op->connData->state == certLdapError ) ){
	FE_Alert(op->window, XP_GetString(XP_DIRECTORY_ERROR));
    }
    
    PORT_FreeArena(op->arena, PR_FALSE);
    return;
}

static void
cleanupCallback(void *arg)
{
    certLdapCleanup((CertLdapOpDesc *)arg);
    return;
}

static PRBool
searchResultsHandler(XPDialogState *state, char **argv, int argc,
		     unsigned int button)
{
    int i;
    CertLdapOpDesc *op;
    SECStatus rv;
    CERTCertificate *cert;
    CERTPackageType type;
    PRBool ret;
    SEC_PKCS7ContentInfo *ci;
    SECItem digest;
    unsigned char nullsha1[SHA1_LENGTH];
    
    op = (CertLdapOpDesc *)state->arg;
    
    if ( button == XP_DIALOG_OK_BUTTON ) {
	for ( i = 0; i < op->numnames; i++ ) {
	    if ( op->rawcerts[i].data != NULL ) {
		/* XXX - need to deal with both raw certs and chains!! */
		type = CERT_CertPackageType(&op->rawcerts[i], NULL);
		switch ( type ) {
		  case certPackageCert:
		    
		    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
						   &op->rawcerts[i],
						   NULL,
						   PR_FALSE,
						   PR_TRUE);
		    if ( cert != NULL ) {
			rv = CERT_SaveImportedCert(cert,
						   certUsageEmailRecipient,
						   PR_FALSE, NULL);
			if ( rv == SECSuccess ) {
			    rv = CERT_SaveSMimeProfile(cert, NULL, NULL);
			}
		    }
		    
		    break;
		  case certPackagePKCS7:
		    ci = SEC_PKCS7DecodeItem(&op->rawcerts[i], NULL, NULL,
					     NULL, NULL);
		    if ( ci != NULL ) {
			if ( SEC_PKCS7ContentIsSigned(ci) ) {
			    rv = SHA1_HashBuf(nullsha1, nullsha1, 0);
			    if ( rv != SECSuccess ) {
				break;
			    }
	
			    digest.len = SHA1_LENGTH;
			    digest.data = nullsha1;

			    ret = SEC_PKCS7VerifyDetachedSignature(ci,
								   certUsageEmailRecipient,
								   &digest,
								   HASH_AlgSHA1,
								   PR_TRUE);
			}
		    }
		    
		    break;
		}
	    }
	}
    }

    (* op->redrawcb->callback)(op->redrawcb->cbarg);

    certLdapCleanup(op);
    return(PR_FALSE);
}

static XPDialogInfo resultsDialog = {
    XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_OK_BUTTON,
    searchResultsHandler,
    500,
    300
};

static void
searchResults(void *arg)
{
    CertLdapOpDesc *op;
    char *found = NULL;
    char *notFound = NULL;
    int i;
    XPDialogStrings *strings;
    int ret;
    
    op = (CertLdapOpDesc *)arg;

    if ( ( op->connData == NULL ) ||
	( op->connData->state != certLdapDone ) ) {
	/* ldap transaction is not done yet and someone pressed the stop
	 * button, or there was an error in the protocol handler
	 * we bail out here.
	 */
	certLdapCleanup(op);
	return;
    }
    
    for ( i = 0; i < op->numnames; i++ ) {
	if ( op->rawcerts[i].data == NULL ) {
	    notFound = PR_sprintf_append(notFound, "%s<br>", op->names[i]);
	} else {
	    found = PR_sprintf_append(found, "%s<br>", op->names[i]);
	}
    }

    strings = XP_GetDialogStrings(XP_DIALOG_FETCH_RESULTS_STRINGS);
    if ( strings ) {
	XP_SetDialogString(strings, 0, found);
	if ( notFound != NULL ) {
	    XP_CopyDialogString(strings, 1,
			       XP_GetString(XP_DIALOG_FETCH_NOT_FOUND_STRINGS));
	    XP_SetDialogString(strings, 2, notFound);
	}
	
	XP_MakeHTMLDialog(op->window, &resultsDialog,
			  XP_DIALOG_FETCH_RESULTS_TITLE, strings,
			  (void *)op);
	XP_FreeDialogStrings(strings);
    }
    
    if ( found != NULL ) {
	PORT_Free(found);
    }
    if ( notFound != NULL ) {
	PORT_Free(notFound);
    }
    
    return;
}

static void
closeStatusHack(void *arg)
{
    SECNAV_CloseStatusDialog((SECStatusDialog *)arg);
    return;
}

/* XXX
 * call close from a timeout to give layout time to create the buttons
 * in the status window.  This does not remove the race, it just makes
 * it happen less frequently.  As soon as I can find out from Brendan what
 * I need to do to avoid the race I will fix it.
 */
static void
searchCallback(CertLdapOpDesc *op)
{
/*    SECNAV_CloseStatusDialog(op->statdlg); */
    if ( op->statdlg != NULL ) {
	(void)FE_SetTimeout(closeStatusHack, (void *)op->statdlg, 1000);
    }
    
    return;
}

static SECStatus
parseNameString(CertLdapOpDesc *op, char *nameString)
{
    int numnames = 0;
    PRBool spacesep = PR_FALSE;
    int c;
    char *tmpstring, *tmpstring2;
    int i;
    
    if ( op->searchType == certLdapSearchMail ) {
	spacesep = PR_TRUE;
    }

    tmpstring = nameString;
    
    while ( *tmpstring ) {
	if ( spacesep ) {
	    /* skip any seperators */
	    while ( ( c = *tmpstring ) &&
		   ( ( c == ' ' ) || ( c == ',' ) ) ) {
		*tmpstring = '\0';
		tmpstring++;
	    }
	    if ( c == '\0' ) {
		break;
	    }

	    /* count the name */
	    numnames++;

	    /* now skip the name */
	    while ( ( c = *tmpstring ) &&
		   ( ( c != ' ' ) && ( c != ',' ) ) ) {
		tmpstring++;
	    }
	} else {
	    /* skip any seperators */
	    while ( ( c = *tmpstring ) &&
		   ( ( c == ' ' ) || ( c == ',' ) ) ) {
		*tmpstring = '\0';
		tmpstring++;
	    }

	    if ( c == '\0' ) {
		break;
	    }

	    /* count the name */
	    numnames++;

	    /* now skip the name */
	    while ( ( c = *tmpstring ) && ( c != ',' ) ) {
		tmpstring++;
	    }
	    if ( c == ',' ) {
		/* go back and nuke any trailing spaces */
		tmpstring2 = tmpstring;
		tmpstring2--;
		while ( *tmpstring2 == ' ' ) {
		    *tmpstring2 = '\0';
		    tmpstring2--;
		}
	    }
	}
    }

    if ( numnames == 0 ) {
	goto loser;
    }
    
    op->numnames = numnames;
    op->names = PORT_ArenaAlloc(op->arena, numnames * sizeof(char *));
    if ( op->names == NULL ) {
	goto loser;
    }

    tmpstring = nameString;
    
    for ( i = 0; i < numnames; i++ ) {
	/* skip seperator */
	while ( *tmpstring == '\0' ) {
	    tmpstring++;
	}
	
	/* copy the string */
	op->names[i] = PORT_ArenaStrdup(op->arena, tmpstring);

	/* skip past it */
	while ( *tmpstring != '\0' ) {
	    tmpstring++;
	}
    }

    return(SECSuccess);
loser:
    return(SECFailure);
}

typedef struct {
    XP_List *ldapServers;
    SECNAVDeleteCertCallback *cb;
    PRBool inputAddrs;
} searchArgs;

static void
doLdapSearchCallback(void *arg)
{
    CertLdapOpDesc *op;
    URL_Struct *url;
    int err;

    op = (CertLdapOpDesc *)arg;
    
    url = (URL_Struct *)PORT_ZAlloc(sizeof(URL_Struct));
    if ( url == NULL ) {
	goto loser;
    }
	
    url->address = PORT_Strdup("internal-certldap");
    if ( url->address == NULL ) {
	goto loser;
    }

    url->sec_private = (void *)op;
	
    err = NET_GetURL(url, FO_PRESENT, (MWContext *)op->window, NULL);
    if ( err != 0 ) {
	goto loser;
    }
    
    if ( op->type == certLdapPostCert ) {
	op->statdlg = SECNAV_MakeStatusDialog(XP_GetString(XP_SENDING_TO_DIRECTORY),
					      op->window,
					      cleanupCallback, op);
    } else {
	op->statdlg = SECNAV_MakeStatusDialog(XP_GetString(XP_SEARCHING_DIRECTORY),
					      op->window,
					      searchResults, op);
    }
    
    
    if ( op->statdlg == NULL ) {
	goto loser;
    }

    return;
    
loser:
    certLdapCleanup(op);
    return;
}

static PRBool
searchLdapDialogHandler(XPDialogState *state, char **argv, int argc,
			unsigned int button)
{
    char *dirName;
    char *searchByString;
    char *searchForString;
    PRBool searchByEmail = PR_TRUE;
    XP_List *ldapServers;
    PRBool ret;
    DIR_Server *s;
    PRBool allServers = PR_TRUE;
    int i;
    char *filter = NULL;
    PRArenaPool *arena;
    CertLdapOpDesc *op;
    int numnames;
    SECStatus rv;
    searchArgs *searchargs;
    
    searchargs = (searchArgs *)state->arg;
    ldapServers = searchargs->ldapServers;
    
    if ( button == XP_DIALOG_FETCH_BUTTON ) {
	dirName = XP_FindValueInArgs("dirmenu", argv, argc);
	searchByString = XP_FindValueInArgs("searchby", argv, argc);
	searchForString = XP_FindValueInArgs("searchfor", argv, argc);

	for ( i = 1; i <= XP_ListCount(ldapServers); i++ ) {
	    s = (DIR_Server *)XP_ListGetObjectNum (ldapServers, i);
	    if ( s->description != NULL ) {
		if ( PORT_Strcmp(s->description, dirName) == 0 ) {
		    allServers = PR_FALSE;
		    break;
		}
	    } else {
		if ( PORT_Strcmp(s->serverName, dirName) == 0 ) {
		    allServers = PR_FALSE;
		    break;
		}
	    }
	}
	if ( allServers ) {
	    /* don't support all servers right now */
	    goto loser;
	    s = (DIR_Server *)XP_ListGetObjectNum (ldapServers, 1);
	}
	
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if ( arena == NULL ) {
	    goto loser;
	}
	
	op = PORT_ArenaZAlloc(arena, sizeof(CertLdapOpDesc));
	if ( op == NULL ) {
	    goto loser;
	}

	op->redrawcb = searchargs->cb;
	op->window = state->proto_win;
	op->arena = arena;
	op->servername = PORT_ArenaStrdup(arena, s->serverName);
	if ( op->servername == NULL ) {
	    goto loser;
	}
	op->serverport = s->port;
	op->base = PORT_ArenaStrdup(arena, s->searchBase);
	op->mailAttrName = PORT_ArenaStrdup(arena,
					    (char *)DIR_GetFirstAttributeString(s, mail));
	if ( s->isSecure ) {
	    op->isSecure = PR_TRUE;
	} else {
	    op->isSecure = PR_FALSE;
	}
	
	if ( op->base == NULL ) {
	    goto loser;
	}

#ifdef NOTDEF
	if ( PORT_Strcmp(searchByString, "name") == 0 ) {
	    op->searchType = certLdapSearchCN;
	} else {
	    op->searchType = certLdapSearchMail;
	}
#endif
	/* always search by email address for now */
	op->searchType = certLdapSearchMail;
	
	op->type = certLdapGetCert;

	op->cb = searchCallback;
	op->cbarg = NULL;

	/* decodes names and fills in op->names and op->numnames */
	rv = parseNameString(op, searchForString);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
	
	op->rawcerts =
	    (SECItem *)PORT_ArenaZAlloc(arena, op->numnames * sizeof(SECItem));

	if ( op->rawcerts == NULL ) {
	    goto loser;
	}

	state->deleteCallback = doLdapSearchCallback;
	state->cbarg = (void *)op;
	
	goto done;
    }

    XP_ListDestroy(ldapServers);
done:
    ret = PR_FALSE;
    return(ret);
loser:
    XP_ListDestroy(ldapServers);
    return(PR_FALSE);
}

static XPDialogInfo fetchDialog = {
    XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_FETCH_BUTTON,
    searchLdapDialogHandler,
    600,
    300
};

static char *
breakEmailAddrs(char *emailAddrs)
{
    char *addrstart, *addrend;
    char *retstr = NULL;
    
    addrstart = addrend = emailAddrs;
    
    while ( *addrend != '\0' ) {
	if ( *addrend == ',' ) {
	    *addrend = '\0';
	    retstr = PR_sprintf_append(retstr, "%s<br>", addrstart);
	    *addrend = ',';
	    addrstart = &addrend[1];
	}
	addrend++;
    }

    return(retstr);
}

void
SECNAV_SearchLDAPDialog(void *window, SECNAVDeleteCertCallback *cb,
			char *emailAddrs)
{
    char *dirs;
    int ret;
    XP_List *ldapServers;
    DIR_Server *s;
    int i;
    XPDialogStrings *strings;
    searchArgs *searchargs;
    char *brokenEmailAddrs;
    
    /* don't support "All Directories" right now */
#ifdef NOTDEF    
    dirs = PR_sprintf_append(NULL, "<OPTION>%s",
			     XP_GetString(XP_DIALOG_ALL_DIRECTORIES));
    if ( dirs == NULL ) {
	goto loser;
    }
    
#endif
    dirs = NULL;
    
    ldapServers = XP_ListNew();
    if ( ldapServers == NULL ) {
	goto loser;
    }
    
    ret = DIR_GetLdapServers(FE_GetDirServers(), ldapServers);
    if ( ret != 0 ) {
	goto loser;
    }

    for ( i = 1; i <= XP_ListCount(ldapServers); i++ ) {
	s = (DIR_Server *)XP_ListGetObjectNum (ldapServers, i);
	if ( s->description != NULL ) {
	    dirs = PR_sprintf_append(dirs, "<OPTION>%s",
				     s->description);
	} else {
	    dirs = PR_sprintf_append(dirs, "<OPTION>%s",
				     s->serverName);
	}
    }

    if ( dirs == NULL ) {
	goto loser;
    }

    searchargs = PORT_Alloc(sizeof(searchArgs));
    if ( searchargs == NULL ) {
	goto loser;
    }
    searchargs->ldapServers = ldapServers;
    searchargs->cb = cb;

    if ( emailAddrs != NULL ) {
	searchargs->inputAddrs = PR_FALSE;
	brokenEmailAddrs = breakEmailAddrs(emailAddrs);
	if ( brokenEmailAddrs == NULL ) {
	    goto loser;
	}
	
	strings = XP_GetDialogStrings(XP_DIALOG_FETCH2_STRINGS);
	if ( strings ) {
	    XP_CopyDialogString(strings, 0, emailAddrs);
	    XP_SetDialogString(strings, 1, dirs);
	    XP_CopyDialogString(strings, 2, brokenEmailAddrs);
	    XP_MakeHTMLDialog(window, &fetchDialog,
			      XP_DIALOG_FETCH_TITLE, strings,
			      (void *)searchargs);
	    XP_FreeDialogStrings(strings);
	}
    } else {
	searchargs->inputAddrs = PR_TRUE;
	strings = XP_GetDialogStrings(XP_DIALOG_FETCH_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, dirs);
	    XP_MakeHTMLDialog(window, &fetchDialog,
			      XP_DIALOG_FETCH_TITLE, strings,
			      (void *)searchargs);
	    XP_FreeDialogStrings(strings);
	}
    }
    
    PORT_Free(dirs);
    
loser:
    return;
}

static PRBool
publishCertDialogHandler(XPDialogState *state, char **argv, int argc,
			 unsigned int button)
{
    char *dirName;
    XP_List *ldapServers;
    PRBool ret;
    DIR_Server *s;
    PRBool allServers = PR_TRUE;
    int i;
    PRArenaPool *arena;
    CertLdapOpDesc *op;
    SECStatus rv;
    CERTCertificate *cert;
    unsigned char nullsha1[SHA1_LENGTH];
    SEC_PKCS7ContentInfo *ci;
    SECItem *postdata;
    SECItem digest;
    int err;
    URL_Struct *url;
    
    ldapServers = (XP_List *)state->arg;
    
    if ( button == XP_DIALOG_OK_BUTTON ) {

	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if ( arena == NULL ) {
	    goto loser;
	}
	
	op = PORT_ArenaZAlloc(arena, sizeof(CertLdapOpDesc));
	if ( op == NULL ) {
	    goto loser;
	}

	cert = SECNAV_GetDefaultEMailCert(state->proto_win);
	if ( cert == NULL ) {
	    goto loser;
	}
	
	rv = SHA1_HashBuf(nullsha1, nullsha1, 0);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
	
	digest.len = SHA1_LENGTH;
	digest.data = nullsha1;
	
	ci = SECMIME_CreateSigned(cert, cert->dbhandle, SEC_OID_SHA1,
				  &digest, NULL, state->proto_win);
	
	if ( ci == NULL ) {
	    goto loser;
	}
	
	postdata = SEC_PKCS7EncodeItem(NULL, &op->postData, ci, NULL, NULL,
				       state->proto_win);
	
	PORT_Assert(postdata == &op->postData);

	dirName = XP_FindValueInArgs("dirmenu", argv, argc);

	for ( i = 1; i <= XP_ListCount(ldapServers); i++ ) {
	    s = (DIR_Server *)XP_ListGetObjectNum (ldapServers, i);
	    if ( s->description != NULL ) {
		if ( PORT_Strcmp(s->description, dirName) == 0 ) {
		    allServers = PR_FALSE;
		    break;
		}
	    } else {
		if ( PORT_Strcmp(s->serverName, dirName) == 0 ) {
		    allServers = PR_FALSE;
		    break;
		}
	    }
	}
	if ( allServers ) {
	    /* don't support all servers right now */
	    goto loser;
	    s = (DIR_Server *)XP_ListGetObjectNum (ldapServers, 1);
	}
	
	op->window = state->proto_win;
	op->arena = arena;
	op->mailAttrName = PORT_ArenaStrdup(arena,
					    (char *)DIR_GetFirstAttributeString(s, mail));
	if ( s->isSecure ) {
	    op->isSecure = PR_TRUE;
	} else {
	    op->isSecure = PR_FALSE;
	}
	op->servername = PORT_ArenaStrdup(arena, s->serverName);
	if ( op->servername == NULL ) {
	    goto loser;
	}
	op->serverport = s->port;
	op->base = PORT_ArenaStrdup(arena, s->searchBase);
	if ( op->base == NULL ) {
	    goto loser;
	}

#ifdef NOTDEF
	if ( PORT_Strcmp(searchByString, "name") == 0 ) {
	    op->searchType = certLdapSearchCN;
	} else {
	    op->searchType = certLdapSearchMail;
	}
#endif
	/* always search by email address for now */
	op->searchType = certLdapSearchMail;
	
	op->type = certLdapPostCert;

	op->names = (char **)PORT_ArenaAlloc(arena, sizeof(char *));
	if ( op->names == NULL ) {
	    goto loser;
	}
	
	op->names[0] = PORT_ArenaStrdup(arena, cert->emailAddr);
	op->numnames = 1;

	op->cb = searchCallback;
	op->cbarg = NULL;
	
	state->deleteCallback = doLdapSearchCallback;
	state->cbarg = (void *)op;
	
	ret = PR_FALSE;
	goto done;
    }

    ret = PR_FALSE;
done:
    XP_ListDestroy(ldapServers);
    return(ret);
loser:
    XP_ListDestroy(ldapServers);
    return(PR_FALSE);
}

static XPDialogInfo publishDialog = {
    XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_OK_BUTTON,
    publishCertDialogHandler,
    400,
    300
};

void
SECNAV_PublishSMimeCertDialog(void *window)
{
    char *dirs;
    int ret;
    XP_List *ldapServers;
    DIR_Server *s;
    int i;
    XPDialogStrings *strings;

    /* don't support "All Directories" right now */
#ifdef NOTDEF    
    dirs = PR_sprintf_append(NULL, "<OPTION>%s",
			     XP_GetString(XP_DIALOG_ALL_DIRECTORIES));
    if ( dirs == NULL ) {
	goto loser;
    }
    
#endif
    dirs = NULL;
    
    ldapServers = XP_ListNew();
    if ( ldapServers == NULL ) {
	goto loser;
    }
    
    ret = DIR_GetLdapServers(FE_GetDirServers(), ldapServers);
    if ( ret != 0 ) {
	goto loser;
    }

    for ( i = 1; i <= XP_ListCount(ldapServers); i++ ) {
	s = (DIR_Server *)XP_ListGetObjectNum (ldapServers, i);
	if ( s->description != NULL ) {
	    dirs = PR_sprintf_append(dirs, "<OPTION>%s",
				     s->description);
	} else {
	    dirs = PR_sprintf_append(dirs, "<OPTION>%s",
				     s->serverName);
	}
    }

    if ( dirs == NULL ) {
	goto loser;
    }
    
    strings = XP_GetDialogStrings(XP_DIALOG_PUBLISH_CERT_STRINGS);
    if ( strings ) {
	XP_SetDialogString(strings, 0, dirs);
	XP_MakeHTMLDialog(window, &publishDialog,
			  XP_DIALOG_PUBLISH_CERT_TITLE, strings,
			  (void *)ldapServers);
	XP_FreeDialogStrings(strings);
    }
    PORT_Free(dirs);
    
loser:
    return;
}

