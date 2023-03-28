/*
 * Dialogs for certificate management
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: certdlgs.c,v 1.73.2.17 1997/05/24 00:23:38 jwz Exp $
 */
#include "prtime.h"
#include "xp.h"
#include "cert.h"
#include "net.h"
#include "secder.h"
#include "secrng.h"
#include "key.h"
#include "secitem.h"
#include "crypto.h"
#include "base64.h"
#include "net.h"
#include "ssl.h"
#include "htmldlgs.h"
#ifdef XP_MAC
#include "sslimpl.h"
#include "certdb.h"
#else
#include "../ssl/sslimpl.h"
#include "../cert/certdb.h"
#endif
#include "sslproto.h"
#include "secnav.h"
#include "sslerr.h"
#include "xpgetstr.h"
#include "pk11func.h"
#include "prefapi.h"

extern int XP_ERRNO_EBADF;
extern int XP_ERRNO_ECONNREFUSED;
extern int XP_ERRNO_EINVAL;
extern int XP_ERRNO_EIO;
extern int XP_ERRNO_EWOULDBLOCK;
extern int SEC_ERROR_UNKNOWN_ISSUER;
extern int SEC_ERROR_UNTRUSTED_ISSUER;
extern int SSL_ERROR_BAD_CERT_DOMAIN;
extern int SSL_ERROR_BAD_CERTIFICATE;
extern int SEC_ERROR_ADDING_CERT;
extern int SEC_ERROR_FILING_KEY;
extern int SEC_ERROR_EXPIRED_CERTIFICATE;
extern int SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE;
extern int SEC_ERROR_NO_MODULE;
extern int SEC_ERROR_KEYGEN_FAIL;
extern int SEC_ERROR_BAD_NICKNAME;
extern int XP_SEC_SLOT_READONLY;

extern int XP_BROWSER_SEC_INFO_CLEAR_STRINGS;
extern int XP_BROWSER_SEC_INFO_ENCRYPTED_STRINGS;
extern int XP_CA_CERT_DOWNLOAD1_STRINGS;
extern int XP_CA_CERT_DOWNLOAD2_STRINGS;
extern int XP_CA_CERT_DOWNLOAD3_STRINGS;
extern int XP_CA_CERT_DOWNLOAD4_STRINGS;
extern int XP_CA_CERT_DOWNLOAD5_STRINGS;
extern int XP_CA_CERT_DOWNLOAD6_STRINGS;
extern int XP_CA_CERT_DOWNLOAD7_STRINGS;
extern int XP_CA_EXPIRED_DIALOG_STRINGS;
extern int XP_CA_NOT_YET_GOOD_DIALOG_STRINGS;
extern int XP_CERT_EXPIRED_DIALOG_STRINGS;
extern int XP_CERT_NOT_YET_GOOD_DIALOG_STRINGS;
extern int XP_CERT_VIEW_STRINGS;
extern int XP_DELETE_CA_CERT_STRINGS;
extern int XP_DELETE_SITE_CERT_STRINGS;
extern int XP_DELETE_USER_CERT_STRINGS;
extern int XP_EDIT_CA_CERT_STRINGS;
extern int XP_EDIT_SITE_CERT_STRINGS;
extern int XP_KEY_GEN_DIALOG_STRINGS;
extern int XP_KEY_GEN_TOKEN_SELECT;
extern int XP_KEY_GEN_MOREINFO_STRINGS;
extern int XP_KEY_GEN_MOREINFO_TITLE_STRING;
extern int XP_NO_CERT_CLIENT_AUTH_DIALOG_STRINGS;
extern int XP_PLAIN_CERT_DIALOG_STRINGS;
extern int XP_SSL_CERT_DOMAIN_DIALOG_STRINGS;
extern int XP_SSL_CERT_POST_WARN_DIALOG_STRINGS;
extern int XP_SSL_SERVER_CERT1_STRINGS;
extern int XP_SSL_SERVER_CERT2_STRINGS;
extern int XP_SSL_SERVER_CERT3_STRINGS;
extern int XP_SSL_SERVER_CERT4_STRINGS;
extern int XP_SSL_SERVER_CERT5A_STRINGS;
extern int XP_SSL_SERVER_CERT5B_STRINGS;
extern int XP_SSL_SERVER_CERT5C_STRINGS;
extern int XP_USER_CERT_DOWNLOAD_STRINGS;
extern int XP_USER_CERT_NICKNAME_STRINGS;
extern int XP_USER_CERT_DL_MOREINFO_STRINGS;
extern int XP_CERT_DL_MOREINFO_TITLE_STRING;
extern int XP_USER_CERT_SAVE_STRINGS;
extern int XP_USER_CERT_SAVE_TITLE;
extern int XP_WHICH_CERT_CLIENT_AUTH_DIALOG_STRINGS;
extern int XP_CLIENT_CERT_EXPIRED_DIALOG_STRINGS;
extern int XP_CLIENT_CERT_EXPIRED_RENEWAL_DIALOG_STRINGS;
extern int XP_CLIENT_CERT_NOT_YET_GOOD_DIALOG_STRINGS;
extern int XP_SEC_RENEW;
extern int XP_ALERT_TITLE_STRING;
extern int XP_VIEWCERT_TITLE_STRING;
extern int XP_CERTDOMAIN_TITLE_STRING;
extern int XP_CERTISEXP_TITLE_STRING;
extern int XP_CERTNOTYETGOOD_TITLE_STRING;
extern int XP_CAEXP_TITLE_STRING;
extern int XP_CANOTYETGOOD_TITLE_STRING;
extern int XP_ENCINFO_TITLE_STRING;
extern int XP_VIEWPCERT_TITLE_STRING;
extern int XP_DELPCERT_TITLE_STRING;
extern int XP_DELSCERT_TITLE_STRING;
extern int XP_DELCACERT_TITLE_STRING;
extern int XP_EDITSCERT_TITLE_STRING;
extern int XP_EDITCACERT_TITLE_STRING;
extern int XP_NOPCERT_TITLE_STRING;
extern int XP_SELPCERT_TITLE_STRING;
extern int XP_SECINFO_TITLE_STRING;
extern int XP_GENKEY_TITLE_STRING;
extern int XP_NEWSCERT_TITLE_STRING;
extern int XP_NEWCACERT_TITLE_STRING;
extern int XP_NEWPCERT_TITLE_STRING;
extern int XP_CLCERTEXP_TITLE_STRING;
extern int XP_DOWNLOAD_EMAIL_CERT_DIALOG_STRINGS;
extern int XP_EMAIL_CERT_DOWNLOAD_TITLE_STRING;
extern int XP_CA_CERT_SSL_OK_STRING;
extern int XP_CA_CERT_EMAIL_OK_STRING;
extern int XP_CA_CERT_OBJECT_SIGNING_OK_STRING;
extern int XP_CERT_MULTI_SUBJECT_SELECT_STRING;
extern int XP_CERT_MULTI_SUBJECT_SELECT_TITLE_STRING;
extern int XP_DELETE_EMAIL_CERT_STRINGS;
extern int XP_DEL_EMAIL_CERT_TITLE_STRING;
extern int XP_SET_EMAIL_CERT_STRING;
extern int XP_VERIFY_CERT_DIALOG_STRINGS;
extern int XP_VERIFYCERT_TITLE_STRING;
extern int SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID;
extern int SEC_ERROR_REVOKED_CERTIFICATE;
extern int SEC_ERROR_CA_CERT_INVALID;
extern int SEC_ERROR_CRL_BAD_SIGNATURE;
extern int SEC_ERROR_CRL_EXPIRED;
extern int SEC_ERROR_BAD_SIGNATURE;
extern int SEC_ERROR_INADEQUATE_KEY_USAGE;
extern int SEC_ERROR_INADEQUATE_CERT_TYPE;
extern int SEC_ERROR_UNTRUSTED_CERT;
extern int XP_VERIFY_CERT_OK_DIALOG_STRING;

extern int XP_VERIFY_ERROR_EXPIRED;
extern int XP_VERIFY_ERROR_NOT_CERTIFIED;
extern int XP_VERIFY_ERROR_NOT_TRUSTED;
extern int XP_VERIFY_ERROR_NO_CA;
extern int XP_VERIFY_ERROR_BAD_SIG;
extern int XP_VERIFY_ERROR_BAD_CRL;
extern int XP_VERIFY_ERROR_REVOKED;
extern int XP_VERIFY_ERROR_NOT_CA;
extern int XP_VERIFY_ERROR_INTERNAL_ERROR;
extern int XP_VERIFY_ERROR_SIGNING;
extern int XP_VERIFY_ERROR_ENCRYPTION;
extern int XP_VERIFY_ERROR_CERT_SIGNING;
extern int XP_VERIFY_ERROR_CERT_UNKNOWN_USAGE;
extern int XP_VERIFY_ERROR_EMAIL_CA;
extern int XP_VERIFY_ERROR_SSL_CA;
extern int XP_VERIFY_ERROR_OBJECT_SIGNING_CA;
extern int XP_VERIFY_ERROR_EMAIL;
extern int XP_VERIFY_ERROR_SSL;
extern int XP_VERIFY_ERROR_OBJECT_SIGNING;

/* forward reference */
static SECStatus
MakeBadCertDialog(int err, void *proto_win, CERTCertificate *cert);

static XPDialogInfo infoDialog = {
    XP_DIALOG_OK_BUTTON,
    0,
    550,
    350
};

static SECStatus SEC_MakeClientAuthDialog(void *proto_win,
					 CERTCertificate *serverCert,
					 SSLSocket *ss,
					 CERTDistNames *caNames);

/*
 * remove multiple whitespace characters from the string, replacing
 * any run of whitespace with a single space character
 */
static void
compress_spaces(char *psrc)
{
    char *pdst;
    char c;
    PRBool lastspace = PR_FALSE;
    
    if ( psrc == NULL ) {
	return;
    }
    
    pdst = psrc;

    while ( ( (c = *psrc) != 0 ) && isspace(c) ) {
	psrc++;
	/* swallow leading spaces */
    }
    
    
    while ( (c = *psrc++) != 0 ) {
	if ( isspace(c) ) {
	    if ( !lastspace ) {
		*pdst++ = ' ';
	    }
	    lastspace = PR_TRUE;
	} else {
	    *pdst++ = c;
	    lastspace = PR_FALSE;
	}
    }
    *pdst = '\0';

    /* if the last character is a space, then remove it too */
    pdst--;
    c = *pdst;
    if ( isspace(c) ) {
	*pdst = '\0';
    }
    
    return;
}

static PRBool
ErrorNeedsDialog(int err)
{
    return ( ( err == SEC_ERROR_UNKNOWN_ISSUER ) ||
	    ( err == SEC_ERROR_UNTRUSTED_ISSUER ) ||
	    ( err == SSL_ERROR_BAD_CERT_DOMAIN ) ||
	    ( err == SSL_ERROR_POST_WARNING ) ||
	    ( err == SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE ) ||
	    ( err == SEC_ERROR_CA_CERT_INVALID ) ||
	    ( err == SEC_ERROR_EXPIRED_CERTIFICATE ) );
}

/*
 * Add a socket to the linked list of sockets waiting for the cert to
 * be approved by the user.
 */
static SECStatus
AddSockToCert(SSLSocket *ss, CERTCertificate *cert)
{
    SECSocketNode *node;
    
    node = PORT_ZAlloc(sizeof(SECSocketNode));
    if ( node ) {
	node->ss = ss;
	node->next = cert->socketlist;
	cert->socketlist = node;
	cert->socketcount++;

	ss->dialogPending = PR_TRUE;
	
	return(SECSuccess);
    }

    return(SECFailure);
}

/*
 * Add a socket to the linked list of sockets waiting for client auth to
 * be approved by the user for the certificate.
 */
static SECStatus
AuthAddSockToCert(SSLSocket *ss, CERTCertificate *cert)
{
    SECSocketNode *node;
    
    node = PORT_ZAlloc(sizeof(SECSocketNode));
    if ( node ) {
	node->ss = ss;
	node->next = cert->authsocketlist;
	cert->authsocketlist = node;
	cert->authsocketcount++;

	ss->dialogPending = PR_TRUE;
	
	return(SECSuccess);
    }

    return(SECFailure);
}

/*
 * Handshake function that blocks.  Used to force a
 * retry on a connection on the next read/write.
 */
static int
AlwaysBlock(SSLSocket *ss)
{
    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
    return -2;
}

/*
 * set the handshake to block
 */
static void
SetAlwaysBlock(SSLSocket *ss)
{
    ss->handshake = AlwaysBlock;
    ss->nextHandshake = 0;
    return;
}

/*
 * Attempt to restart the handshakes of all sockets waiting for the
 * given server certificate.
 */
static void
RestartHandshakeAfterServerCert(CERTCertificate *cert, void *proto_win)
{
    SSLSocket *ss;
    SECStatus rv;
    SECSocketNode *node, *lastnode;
    int err, saveerr = 0;
    
    /* XXX - re-check cert here, and put up next dialog if necessary */

    node = cert->socketlist;
    cert->socketlist = NULL;
    cert->socketcount = 0;
    
    while ( node ) {
	ss = node->ss;

	if ( ss->beenFreed ) {
	    /* if the socket has already been freed by the owner, then
	     * lets really free it.
	     */
	    ssl_FreeSocket(ss);
	    lastnode = node;
	    node = node->next;
	    PORT_Free(lastnode);
	} else {
	
	    /* try checking the cert again */
	    rv = (int)(* ss->sec->authCertificate)(ss->sec->authCertificateArg,
						   ss->fd, PR_FALSE, PR_FALSE);

	    if ( rv ) {
		/* Not done with this socket, we may need another dialog */

		err = PORT_GetError();
		if ( !ErrorNeedsDialog(err) ) {
		    /* error is unrecoverable */
		    ss->dialogPending = PR_FALSE;
		    if (ss->version == SSL_LIBRARY_VERSION_2) {
			SSL_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
		    } else {
			SSL3_SendAlert(ss, alert_fatal, bad_certificate);
		    }
		    NET_InterruptSocket(ss->fd);

		    lastnode = node;
		    node = node->next;
		    PORT_Free(lastnode);
		} else {
		    /* error is recoverable */

		    /* if this is the first time, remember the error code */
		    if ( cert->socketlist == NULL ) {
			saveerr = err;
		    }
		    
		    /* link socket back into the cert list */
		    lastnode = node;
		    node = node->next;
		    lastnode->next = cert->socketlist;
		    cert->socketlist = lastnode;
		    cert->socketcount++;
		}
	    } else {
		/* certificate checks out OK now */
		ss->dialogPending = PR_FALSE;
		/* if the socket has already been freed by the owner, then
		 * lets really free it.
		 */
	
		rv = SSL_RestartHandshakeAfterServerCert(ss);
	
		if ( rv == SECFailure ) {
		    NET_InterruptSocket(ss->fd);
		}

		lastnode = node;
		node = node->next;
		PORT_Free(lastnode);
	    }
	}
    }

    if ( cert->socketlist != NULL ) {
	/* if there are still problems with the cert, make another dialog */
	rv = MakeBadCertDialog(saveerr, proto_win, cert);
    }
    
    return;
}

/*
 * Attempt to drop all connections waiting for the given certificate,
 * by causing errors on them all.
 */
static void
SendError(CERTCertificate *cert)
{
    SSLSocket *ss;
    SECSocketNode *node, *lastnode;

    cert = CERT_DupCertificate(cert);
    node = cert->socketlist;

    while ( node ) {
	ss = node->ss;

	ss->dialogPending = PR_FALSE;
	/* if the socket has already been freed by the owner, then lets really
	 * free it.
	 */
	if ( ss->beenFreed ) {
	    ssl_FreeSocket(ss);
	} else {
	    if (ss->version == SSL_LIBRARY_VERSION_2) {
		SSL_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    } else {
		SSL3_SendAlert(ss, alert_fatal, bad_certificate);
	    }
	    NET_InterruptSocket(ss->fd);
	}
	lastnode = node;
	node = node->next;
	PORT_Free(lastnode);
    }

    cert->socketlist = NULL;
    cert->socketcount = 0;

    node = cert->authsocketlist;

    while ( node ) {
	ss = node->ss;

	ss->dialogPending = PR_FALSE;
	/* if the socket has already been freed by the owner, then lets really
	 * free it.
	 */
	if ( ss->beenFreed ) {
	    ssl_FreeSocket(ss);
	} else {
	    if (ss->version == SSL_LIBRARY_VERSION_2) {
		SSL_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    } else {
		SSL3_SendAlert(ss, alert_fatal, bad_certificate);
	    }
	    NET_InterruptSocket(ss->fd);
	}
	lastnode = node;
	node = node->next;
	PORT_Free(lastnode);
    }

    cert->authsocketlist = NULL;
    cert->authsocketcount = 0;
    
    CERT_DestroyCertificate(cert);
    return;
}

/*
 * Attempt to restart the handshakes of all sockets waiting for client auth
 * to a given server.
 */
static void
RestartHandshakeAfterClientAuth(CERTCertificate *serverCert,
				CERTCertificate *clientCert,
				SECKEYPrivateKey *clientKey,
				CERTCertificateList *clientCertChain)
{
    SSLSocket *ss;
    SECStatus rv;
    SECSocketNode *node, *lastnode;

    /* XXX - re-check cert here, and put up next dialog if necessary */

    node = serverCert->authsocketlist;
    serverCert->authsocketlist = NULL;
    serverCert->authsocketcount = 0;
    
    while ( node ) {
	ss = node->ss;

	if ( ss->beenFreed ) {
	    /* if the socket has already been freed by the owner, then
	     * lets really free it.
	     */
	    ssl_FreeSocket(ss);
	} else {
	    rv = SSL_RestartHandshakeAfterCertReq(ss, clientCert, clientKey,
						  clientCertChain);
	    ss->dialogPending = PR_FALSE;
	    if ( rv ) {
		NET_InterruptSocket(ss->fd);
	    }

	}
	lastnode = node;
	node = node->next;
	PORT_Free(lastnode);
    }

    return;
}


typedef struct {
    CERTCertificate *cert;
    char *host;
    char *accept;
    PRBool postwarn;
    void *proto_win;
} SSLServerPanelState;

static int
sslServerHandler3(XPPanelState *state, char **argv, int argc,
		  unsigned int button)
{
    char *accept;
    SSLServerPanelState *mystate;
    
    mystate = (SSLServerPanelState *)(state->arg);
    
    accept = XP_FindValueInArgs("accept", argv, argc);
    
    mystate->accept = PORT_Strdup(accept);

    if (PORT_Strcmp (accept, "cancel") == 0)
	return (state->curPanel + 3);		/* skip next panel */
    else
	return 0;
}

static int
sslServerHandler4(XPPanelState *state, char **argv, int argc,
		  unsigned int button)
{
    char *postwarn;
    SSLServerPanelState *mystate;
    
    mystate = (SSLServerPanelState *)(state->arg);
    
    postwarn = XP_FindValueInArgs("postwarn", argv, argc);
    if ((postwarn != NULL) && (PORT_Strcmp(postwarn, "yes") == 0)) {
	mystate->postwarn = PR_TRUE;
    } else {
	mystate->postwarn = PR_FALSE;
    }
	
    return(0);
}

static int
sslServerHandler5(XPPanelState *state, char **argv, int argc,
		  unsigned int button)
{
    SSLServerPanelState *mystate;
    mystate = (SSLServerPanelState *)(state->arg);

    /* If the user accepted cancel, in panel 3, then back should go
     * directly back there.  If they accepted the cert, then we
     * just go back to panel 4.
     */
    if ( PORT_Strncmp(mystate->accept, "cancel", 6) == 0 ) {
	if (button == XP_DIALOG_BACK_BUTTON) {
	    return(3);
	}
    }
    
    return 0;
}

static XPDialogStrings *
sslServerPanel1(XPPanelState *state)
{
    SSLServerPanelState *mystate;
    XPDialogStrings *strings;
    
    mystate = (SSLServerPanelState *)(state->arg);

    strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT1_STRINGS);
    if ( strings != NULL ) {
	XP_CopyDialogString(strings, 0, mystate->host);
    }

    return(strings);
}

static XPDialogInfo certDialog = {
    XP_DIALOG_OK_BUTTON,
    0,
    600,
    400
};

static int
sslServerHandler2(XPPanelState *state, char **argv, int argc,
		  unsigned int button)
{
    SSLServerPanelState *mystate;
    char *certstr;
    XPDialogStrings *strings;
    
    mystate = (SSLServerPanelState *)(state->arg);
    
    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(mystate->cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(mystate->proto_win, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
    }
    
    return(0);
}

static XPDialogStrings *
sslServerPanel2(XPPanelState *state)
{
    SSLServerPanelState *mystate;
    int secret;
    char buf[20];
    char *str;
    XPDialogStrings *strings;
    
    mystate = (SSLServerPanelState *)(state->arg);

    strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT2_STRINGS);
    if ( strings != NULL ) {
	str = CERT_GetOrgName(&mystate->cert->subject);
	if ( str ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 0, " ");
	}
	
	str = CERT_GetOrgName(&mystate->cert->issuer);
	if ( str ) {
	    XP_CopyDialogString(strings, 1, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 1, " ");
	}

	secret = mystate->cert->socketlist->ss->sec->secretKeyBits;
    
	if ( secret <= 40 ) {
	    str = "Export";
	} else if ( secret <= 80 ) {
	    str = "Medium";
	} else {
	    str = "Highest";
	}
	XP_CopyDialogString(strings, 2, str);

	/* name of cipher */
	if (mystate->cert->socketlist->ss->version == SSL_LIBRARY_VERSION_2) {
	    str = ssl_cipherName[
			    mystate->cert->socketlist->ss->sec->cipherType];
	} else {
	    str = ssl3_cipherName[
			    mystate->cert->socketlist->ss->sec->cipherType];
	}
	XP_CopyDialogString(strings, 3, str);

	/* secret key size */
	XP_SPRINTF(buf, "%d", secret);
	XP_CopyDialogString(strings, 4, buf);
    }
    
    return(strings);
}

static XPDialogStrings *
sslServerPanel3(XPPanelState *state)
{
    SSLServerPanelState *mystate;
    XPDialogStrings *strings;
    char *str1, *str2, *str3;
    
    mystate = (SSLServerPanelState *)(state->arg);
    if ( mystate->accept ) {
	if ( PORT_Strncmp(mystate->accept, "session", 7) == 0 ) {
	    str1 = " checked ";
	    str2 = " ";
	    str3 = " ";
	} else if ( PORT_Strncmp(mystate->accept, "forever", 7) == 0 ) {
	    str1 = " ";
	    str2 = " ";
	    str3 = " checked ";
	} else {
	    str1 = " ";
	    str2 = " checked ";
	    str3 = " ";
	}
    } else {
	str1 = " checked ";
	str2 = " ";
	str3 = " ";
    }
    
    strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT3_STRINGS);
    if ( strings != NULL ) {
	XP_CopyDialogString(strings, 0, str1);
	XP_CopyDialogString(strings, 1, str2);
	XP_CopyDialogString(strings, 2, str3);
    }
    
    return(strings);
}

static XPDialogStrings *
sslServerPanel4(XPPanelState *state)
{
    SSLServerPanelState *mystate;
    char *str;
    XPDialogStrings *strings;
    
    mystate = (SSLServerPanelState *)(state->arg);
    if ( mystate->postwarn ) {
	str = "checked";
    } else {
	str = "";
    }
    
    strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT4_STRINGS);
    if ( strings != NULL ) {
	XP_CopyDialogString(strings, 0, str);
    }
    
    return(strings);
}

static XPDialogStrings *
sslServerPanel5(XPPanelState *state)
{
    SSLServerPanelState *mystate;
    char *str;
    XPDialogStrings *strings;
    
    mystate = (SSLServerPanelState *)(state->arg);

    str = mystate->host;

    PORT_Assert (mystate->accept != NULL);

    if (mystate->accept != NULL) {
	if (PORT_Strcmp(mystate->accept, "cancel") == 0) {
	    strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT5A_STRINGS);
	} else {
	    if (mystate->postwarn) {
		strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT5B_STRINGS);
	    } else {
		strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT5C_STRINGS);
	    }
	}
    } else {
	/*
	 * If we get here, something is wrong.  Attempt a repair by
	 * forcing a cancel.  Either of the following two assignments
	 * could fail if our problem is that we are out of memory.
	 * But that, which will end up setting either string to NULL,
	 * does not make us any worse off than we already are and is
	 * already handled as best as it can by the remaining code.
	 */
	strings = XP_GetDialogStrings(XP_SSL_SERVER_CERT5A_STRINGS);
	mystate->accept = PORT_Strdup ("cancel");
    }

    if (strings != NULL) {
	XP_CopyDialogString(strings, 0, str);
    }

    return(strings);
}

static char *
default_server_nickname(CERTCertificate *cert)
{
    char *nickname = NULL;
    int count;
    PRBool conflict;
    char *servername = NULL;
    
    servername = CERT_GetCommonName(&cert->subject);
    if ( servername == NULL ) {
	goto loser;
    }
    
    count = 1;
    while ( 1 ) {
	
	if ( count == 1 ) {
	    nickname = PR_smprintf("%s", servername);
	} else {
	    nickname = PR_smprintf("%s #%d", servername, count);
	}
	if ( nickname == NULL ) {
	    goto loser;
	}

	compress_spaces(nickname);
	
	conflict = SEC_CertNicknameConflict(nickname, &cert->derSubject,
					    cert->dbhandle);
	
	if ( !conflict ) {
	    goto done;
	}

	/* free the nickname */
	PORT_Free(nickname);

	count++;
    }
    
loser:
    if ( nickname ) {
	PORT_Free(nickname);
    }

    nickname = "";
    
done:
    if ( servername ) {
	PORT_Free(servername);
    }
    
    return(nickname);
}

static void
server_cert_restart_cb(void *closure)
{
    SSLServerPanelState *mystate = (SSLServerPanelState *)closure;

    RestartHandshakeAfterServerCert(mystate->cert, mystate->proto_win);

    if ( mystate->host ) {
	PORT_Free(mystate->host);
    }
    if ( mystate->accept ) {
	PORT_Free(mystate->accept);
    }
    PORT_Free(mystate);
}

static void
sec_sslServerFinish(XPPanelState *state, PRBool cancel)
{
    SSLServerPanelState *mystate;
    CERTCertificate *cert;
    char *nickname;
    CERTCertTrust trust;
    CERTCertTrust *trustptr;
    SECStatus rv;
    
    mystate = (SSLServerPanelState *)(state->arg);

    cert = mystate->cert;
    
    if ( cancel ) {
	goto sslerror;
    }

    if ( mystate->accept ) {
	if ( PORT_Strncmp(mystate->accept, "forever", 7) == 0 ) {

	    PORT_Memset((void *)&trust, 0, sizeof(trust));
	    trust.sslFlags = CERTDB_VALID_PEER | CERTDB_TRUSTED;
	    if ( mystate->postwarn ) {
		trust.sslFlags |= CERTDB_SEND_WARN;
	    }

	    nickname = default_server_nickname(cert);
	    
	    rv = CERT_AddTempCertToPerm(cert, nickname, &trust);
	    
	    if ( nickname ) {
		PORT_Free(nickname);
	    }

	    /* XXX - what do we do on error?? */
	    if ( rv ) {
	    }
	} else if ( PORT_Strncmp(mystate->accept, "cancel", 6) == 0 ) {
	    goto sslerror;
	} else if ( PORT_Strncmp(mystate->accept, "session", 7) == 0 ) {
	    /* bump the ref count on the cert so that it stays around for
	     * the entire session
	     */
	    cert->keepSession = PR_TRUE;
	    
	    if ( cert->trust ) {
		trustptr = cert->trust;
	    } else {
		trustptr = (CERTCertTrust *)PORT_ArenaZAlloc(cert->arena,
							 sizeof(CERTCertTrust));
		cert->trust = trustptr;
	    }
	    if ( trustptr == NULL ) {
		goto sslerror;
	    }

	    /* set the trust value */
	    trustptr->sslFlags |= (CERTDB_VALID_PEER | CERTDB_TRUSTED);
	    if ( mystate->postwarn ) {
		trustptr->sslFlags |= CERTDB_SEND_WARN;
	    }
	}
    } else {
	goto sslerror;
    }

    /* restart the connection */
    state->deleteCallback = server_cert_restart_cb;
    state->cbarg = (void *)mystate;
    
    goto done;

sslerror:
    SendError(mystate->cert);

    if ( mystate->host ) {
	PORT_Free(mystate->host);
    }
    if ( mystate->accept ) {
	PORT_Free(mystate->accept);
    }
    PORT_Free(mystate);
    
done:
    return;
}

static XPPanelDesc sslServerPanels[] = {
    { NULL, sslServerPanel1, 0 },
    { sslServerHandler2, sslServerPanel2, 0 },
    { sslServerHandler3, sslServerPanel3, 0 },
    { sslServerHandler4, sslServerPanel4, 0 },
    { sslServerHandler5, sslServerPanel5, 0 }
};

static XPPanelInfo sslServerPanel = {
    sslServerPanels,
    sizeof(sslServerPanels)/sizeof(XPPanelDesc),
    sec_sslServerFinish,
    550,
    350
};

static SECStatus
SEC_MakeSSLServerPanel(void *proto_win, CERTCertificate *cert)
{
    SSLServerPanelState *mystate;
    SSLSocket *ss;
    
    ss = cert->socketlist->ss;

    mystate = (SSLServerPanelState *)PORT_Alloc(sizeof(SSLServerPanelState));
    if ( mystate == NULL ) {
	return(SECFailure);
    }
    
    mystate->cert = cert;

    if ( ss->sec->url ) {
	mystate->host = SECNAV_GetHostFromURL(ss->sec->url);
    } else {
	mystate->host =
	    CERT_GetCommonName(&cert->subject);
    }
    
    mystate->postwarn = PR_FALSE;
    mystate->accept = NULL;
    mystate->proto_win = proto_win;
    
    XP_MakeHTMLPanel(proto_win, &sslServerPanel,
		     XP_NEWSCERT_TITLE_STRING, (void *)mystate);

    return(SECSuccess);
}

typedef struct {
    CERTCertificate *cert;
    CERTDERCerts *derCerts;
    char *postwarn;
    PRBool acceptssl;
    PRBool acceptsmime;
    PRBool acceptobjectsigning;
    char *nickname;
    void *proto_win;
} CADownloadPanelState;

char *
getDisplayName(CERTName *name)
{
    char *str;
    
    str = CERT_GetOrgName(name);
    if ( str == NULL ) {
	str = CERT_GetCommonName(name);
    
	if ( str == NULL ) {
	    str = CERT_GetOrgUnitName(name);
    
	    if ( str == NULL ) {
		str = CERT_GetCertEmailAddress(name);

		if ( str == NULL ) {
		    str = " ";
		}
	    }
	}
    }
    
    return(str);
}

static XPDialogStrings *
caDownloadPanel3(XPPanelState *state)
{
    CADownloadPanelState *mystate;
    XPDialogStrings *strings;
    char *str;
    
    mystate = (CADownloadPanelState *)(state->arg);

    strings = XP_GetDialogStrings(XP_CA_CERT_DOWNLOAD3_STRINGS);

    if ( strings ) {
	
	str = getDisplayName(&mystate->cert->subject);
	XP_CopyDialogString(strings, 0, str);
	PORT_Free(str);

	str = getDisplayName(&mystate->cert->issuer);
	XP_CopyDialogString(strings, 1, str);
	PORT_Free(str);
    }
    
    return(strings);
}

static XPDialogStrings *
caDownloadPanel4(XPPanelState *state)
{
    CADownloadPanelState *mystate;
    XPDialogStrings *strings;
    char *sslstr = NULL, *emailstr = NULL, *objectsigningstr = NULL;
    char *name;
    unsigned int certtype;
    
    mystate = (CADownloadPanelState *)(state->arg);

    certtype = mystate->cert->nsCertType;
    if ( ! ( certtype & NS_CERT_TYPE_CA ) ) {
	certtype = NS_CERT_TYPE_CA;
    }
    
    if ( certtype & NS_CERT_TYPE_SSL_CA ) {
	name = "acceptssl";
	sslstr = PR_smprintf("<input type=checkbox name=%s value=yes %s>%s<br>",
			     name, mystate->acceptssl ? "checked" : "",
			     XP_GetString(XP_CA_CERT_SSL_OK_STRING));
    }

    if ( sslstr == NULL ) {
	sslstr = "";
    }

    if ( certtype & NS_CERT_TYPE_EMAIL_CA ) {
	name = "acceptsmime";
	emailstr = PR_smprintf("<input type=checkbox name=%s value=yes %s>%s<br>",
			       name, mystate->acceptsmime ? "checked" : "",
			       XP_GetString(XP_CA_CERT_EMAIL_OK_STRING));
    }
    if ( emailstr == NULL ) {
	emailstr = "";
    }

    if ( certtype & NS_CERT_TYPE_OBJECT_SIGNING_CA ) {
	name = "acceptobjectsigning";
	objectsigningstr =
	    PR_smprintf("<input type=checkbox name=%s value=yes %s>%s",
			name, mystate->acceptobjectsigning ? "checked" : "",
			XP_GetString(XP_CA_CERT_OBJECT_SIGNING_OK_STRING));
    }
    if (objectsigningstr == NULL ) {
	objectsigningstr = "";
    }
    
    strings = XP_GetDialogStrings(XP_CA_CERT_DOWNLOAD4_STRINGS);
    if ( strings != NULL ) {
	XP_CopyDialogString(strings, 0, sslstr);
	XP_CopyDialogString(strings, 1, emailstr);
	XP_CopyDialogString(strings, 2, objectsigningstr);
    }

    if ( *sslstr != 0 ) {
	PORT_Free(sslstr);
    }
    if ( *emailstr != 0 ) {
	PORT_Free(emailstr);
    }
    if ( *objectsigningstr != 0 ) {
	PORT_Free(objectsigningstr);
    }
    
    return(strings);
}

static XPDialogStrings *
caDownloadPanel5(XPPanelState *state)
{
    CADownloadPanelState *mystate;
    XPDialogStrings *strings;
    char *str;
    
    mystate = (CADownloadPanelState *)(state->arg);
    if ( mystate->postwarn ) {
	if ( PORT_Strncmp(mystate->postwarn, "yes", 3) == 0 ) {
	    str = "checked";
	} else {
	    str = "";
	}
    } else {
	str = "";
    }
    
    strings = XP_GetDialogStrings(XP_CA_CERT_DOWNLOAD5_STRINGS);
    if ( strings != NULL ) {
	XP_CopyDialogString(strings, 0, str);
    }
    
    return(strings);
}

static XPDialogStrings *
caDownloadPanelX(XPPanelState *state)
{
    CADownloadPanelState *mystate;
    XPDialogStrings *strings;
    int stringtag;
    
    mystate = (CADownloadPanelState *)(state->arg);

    switch ( state->curPanel ) {
      case 0:
	stringtag = XP_CA_CERT_DOWNLOAD1_STRINGS;
	break;
      case 1:
	stringtag = XP_CA_CERT_DOWNLOAD2_STRINGS;
	break;
      case 2:
	stringtag = XP_CA_CERT_DOWNLOAD3_STRINGS;
	break;
      case 3:
	stringtag = XP_CA_CERT_DOWNLOAD4_STRINGS;
	break;
      case 4:
	stringtag = XP_CA_CERT_DOWNLOAD5_STRINGS;
	break;
      case 5:
	stringtag = XP_CA_CERT_DOWNLOAD6_STRINGS;
	break;
      case 6:
	stringtag = XP_CA_CERT_DOWNLOAD7_STRINGS;
	break;
      default:
	return(NULL);
    }
    
    strings = XP_GetDialogStrings(stringtag);

    return(strings);
}

static void
caDownloadFinish(XPPanelState *state, PRBool cancel)
{
    CADownloadPanelState *mystate;
    CERTCertificate *cert;
    PRBool warn;
    CERTCertTrust trust;
    char *nickname;
    SECStatus rv;
    int numcacerts;
    SECItem *cacerts;
    PRBool accept;
    
    mystate = (CADownloadPanelState *)(state->arg);
    cert = mystate->cert;
    nickname = NULL;
    
    if ( cancel ) {
	goto done;
    }

    warn = PR_FALSE;
    if ( mystate->postwarn ) {
	if ( PORT_Strncmp(mystate->postwarn, "yes", 3) == 0 ) {
	    warn = PR_TRUE;
	}
    }

    if ( mystate->nickname ) {
	nickname = mystate->nickname;
    } else {
	nickname = CERT_GetCommonName(&cert->subject);
    }
    
    accept = PR_FALSE;
    PORT_Memset((void *)&trust, 0, sizeof(trust));
    if ( mystate->acceptssl ) {
	if ( warn ) {
	    trust.sslFlags = CERTDB_VALID_CA | CERTDB_TRUSTED_CA |
		CERTDB_SEND_WARN;
	} else {
	    trust.sslFlags = CERTDB_VALID_CA | CERTDB_TRUSTED_CA;
	}
	accept = PR_TRUE;
    }
    if ( mystate->acceptsmime ) {
	trust.emailFlags = CERTDB_VALID_CA | CERTDB_TRUSTED_CA;
	accept = PR_TRUE;
    }
    if ( mystate->acceptobjectsigning ) {
	trust.objectSigningFlags = CERTDB_VALID_CA | CERTDB_TRUSTED_CA;
	accept = PR_TRUE;
    }

    if ( accept ) {
	/* add the certificate to the perm database */
	rv = CERT_AddTempCertToPerm(cert, nickname, &trust);
	    
	/* XXX - what do we do on error?? */
	if ( rv ) {
	}

	/* now add other ca certs */
	numcacerts = mystate->derCerts->numcerts - 1;
	    
	if ( numcacerts ) {
	    cacerts = mystate->derCerts->rawCerts+1;
	    rv = CERT_ImportCAChain(cacerts, numcacerts, certUsageSSLCA);
	    /* XXX - what do we do on error?? */
	    if ( rv != SECSuccess ) {
	    }
	}
    }

done:
    
    if ( mystate->derCerts ) {
	PORT_FreeArena(mystate->derCerts->arena, PR_FALSE);
    }
    
    CERT_DestroyCertificate(cert);
    
    if ( mystate->postwarn ) {
	PORT_Free(mystate->postwarn);
    }

    if ( nickname ) {
	PORT_Free(nickname);
    }

    return;
}

static int
caDownloadHandler3(XPPanelState *state, char **argv, int argc,
	       unsigned int button)
{
    CADownloadPanelState *mystate;
    char *certstr;
    XPDialogStrings *strings;
    
    mystate = (CADownloadPanelState *)(state->arg);

    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(mystate->cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(mystate->proto_win, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
    }
    return(0);
}

static int
caDownloadHandler4(XPPanelState *state, char **argv, int argc,
		   unsigned int button)
{
    char *accept;
    CADownloadPanelState *mystate;
    
    mystate = (CADownloadPanelState *)(state->arg);
    
    accept = XP_FindValueInArgs("acceptssl", argv, argc);
    
    if ( accept ) {
	mystate->acceptssl = ( PORT_Strcmp(accept, "yes") == 0 );
    }

    accept = XP_FindValueInArgs("acceptsmime", argv, argc);
    
    if ( accept ) {
	mystate->acceptsmime = ( PORT_Strcmp(accept, "yes") == 0 );
    }

    accept = XP_FindValueInArgs("acceptobjectsigning", argv, argc);
    
    if ( accept ) {
	mystate->acceptobjectsigning = ( PORT_Strcmp(accept, "yes") == 0 );
    }
    
    if ( ! ( mystate->acceptssl | mystate->acceptsmime |
	    mystate->acceptobjectsigning ) ) {
	return(7);
    }
    
    return(0);
}

static int
caDownloadHandler5(XPPanelState *state, char **argv, int argc,
		   unsigned int button)
{
    char *postwarn;
    CADownloadPanelState *mystate;
    
    mystate = (CADownloadPanelState *)(state->arg);
    
    postwarn = XP_FindValueInArgs("postwarn", argv, argc);
    
    if ( postwarn ) {
	mystate->postwarn = PORT_Strdup(postwarn);
    } else {
	if ( mystate->postwarn ) {
	    PORT_Free(mystate->postwarn);
	    mystate->postwarn = NULL;
	}
    }
	
    return(0);
}

static int
caDownloadHandler6(XPPanelState *state, char **argv, int argc,
		   unsigned int button)
{
    char *nickname;
    CADownloadPanelState *mystate;
    
    mystate = (CADownloadPanelState *)(state->arg);
    
    nickname = XP_FindValueInArgs("nickname", argv, argc);
    
    if ( nickname && ( *nickname ) ) {
	mystate->nickname = PORT_Strdup(nickname);
    } else {
	if ( mystate->nickname ) {
	    PORT_Free(mystate->nickname);
	    mystate->nickname = NULL;
	}
	if ( button == XP_DIALOG_FINISHED_BUTTON ) {
	    return(6);
	}
    }
	
    return(0);
}

static int
caDownloadHandler7(XPPanelState *state, char **argv, int argc,
		   unsigned int button)
{
    
    if ( button == XP_DIALOG_BACK_BUTTON ) {
	return(4);
    }
	
    return(0);
}

static XPPanelDesc caDownloadPanels[] = {
    { NULL, caDownloadPanelX, 0 },
    { NULL, caDownloadPanelX, 0 },
    { caDownloadHandler3, caDownloadPanel3, 0 },
    { caDownloadHandler4, caDownloadPanel4, 0 },
    { caDownloadHandler5, caDownloadPanel5, 0 },
    { caDownloadHandler6, caDownloadPanelX, XP_PANEL_FLAG_FINAL },
    { caDownloadHandler7, caDownloadPanelX , XP_PANEL_FLAG_FINAL }
};

static XPPanelInfo caDownloadPanel = {
    caDownloadPanels,
    sizeof(caDownloadPanels)/sizeof(XPPanelDesc),
    caDownloadFinish,
    550,
    350
};

void
SECNAV_MakeCADownloadPanel(void *proto_win, CERTCertificate *cert,
			   CERTDERCerts *derCerts)
{
    CADownloadPanelState *mystate;
    mystate = (CADownloadPanelState *)PORT_Alloc(sizeof(CADownloadPanelState));
    if ( mystate == NULL ) {
	return;
    }
    
    mystate->cert = cert;
    mystate->derCerts = derCerts;
    mystate->postwarn = NULL;
    mystate->acceptssl = PR_FALSE;
    mystate->acceptsmime = PR_FALSE;
    mystate->acceptobjectsigning = PR_FALSE;
    mystate->nickname = NULL;
    mystate->proto_win = proto_win;
    
    XP_MakeHTMLPanel(proto_win, &caDownloadPanel,
		     XP_NEWCACERT_TITLE_STRING, (void *)mystate);

    return;
}

static PRBool
certDomainDialogHandler(XPDialogState *state, char **argv, int argc,
			unsigned int button)
{
    char *certstr;
    XPDialogStrings *strings;
    CERTCertificate *cert;
    
    cert = (CERTCertificate *)state->arg;
    
    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(state->window, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	return(PR_TRUE);
    }

    if ( button == XP_DIALOG_CONTINUE_BUTTON ) {
	/* keep the cert around for the entire session */
	cert->keepSession = PR_TRUE;
	cert->domainOK = PR_TRUE;

	RestartHandshakeAfterServerCert(cert, state->proto_win);
    } else { /* CANCEL */
	SendError(cert);
    }
    
    return(PR_FALSE);
}

static XPDialogInfo certDomainDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    certDomainDialogHandler,
    550,
    350
};

static SECStatus
SEC_MakeCertDomainDialog(void *proto_win, CERTCertificate *cert)
{
    XPDialogStrings *strings;
    char buf[20];
    char *str;
    int secret;
    SSLSocket *ss;
    
    ss = cert->socketlist->ss;

    strings = XP_GetDialogStrings(XP_SSL_CERT_DOMAIN_DIALOG_STRINGS);
    if ( strings != NULL ) {
	str = SECNAV_GetHostFromURL(ss->sec->url);
	if ( str ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 0, " ");
	}
	
	str = CERT_GetOrgName(&cert->subject);
	if ( str ) {
	    XP_CopyDialogString(strings, 1, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 1, " ");
	}
	
	str = CERT_GetOrgName(&cert->issuer);
	if ( str ) {
	    XP_CopyDialogString(strings, 2, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 2, " ");
	}

	secret = ss->sec->secretKeyBits;
    
	if ( secret <= 40 ) {
	    str = "Export";
	} else if ( secret <= 80 ) {
	    str = "Medium";
	} else {
	    str = "Highest";
	}
	XP_CopyDialogString(strings, 3, str);

	/* name of cipher */
	str = ssl_cipherName[ss->sec->cipherType];
	XP_CopyDialogString(strings, 4, str);

	/* secret key size */
	XP_SPRINTF(buf, "%d", secret);
	XP_CopyDialogString(strings, 5, buf);

	/* mark socket as dialog pending */
	ss->dialogPending = PR_TRUE;
	
	XP_MakeHTMLDialog(proto_win, &certDomainDialog,
			  XP_CERTDOMAIN_TITLE_STRING, strings, (void *)cert);
	XP_FreeDialogStrings(strings);

	return(SECSuccess);
    }
    
    return(SECFailure);
}

static PRBool
certExpiredDialogHandler(XPDialogState *state, char **argv, int argc,
			unsigned int button)
{
    char *certstr;
    XPDialogStrings *strings;
    CERTCertificate *cert;
    
    cert = (CERTCertificate *)state->arg;
    
    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(state->window, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	return(PR_TRUE);
    }

    if ( button == XP_DIALOG_CONTINUE_BUTTON ) {
	/* keep the cert around for the entire session */
	cert->keepSession = PR_TRUE;
	cert->timeOK = PR_TRUE;

	RestartHandshakeAfterServerCert(cert, state->proto_win);
    } else { /* CANCEL */
	SendError(cert);
    }
    
    return(PR_FALSE);
}

static XPDialogInfo certExpiredDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    certExpiredDialogHandler,
    550,
    350
};

static XPDialogInfo certNotYetGoodDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    certExpiredDialogHandler,
    550,
    350
};

static SECStatus
SEC_MakeCertExpiredDialog(void *proto_win, CERTCertificate *cert)
{
    XPDialogStrings *strings;
    char *str;
    SSLSocket *ss;
    int64 now, notBefore, notAfter;
    PRBool expired;
    SECStatus rv;
    
    ss = cert->socketlist->ss;

    now = PR_Now();
    rv = CERT_GetCertTimes(cert, &notBefore, &notAfter);
    if ( rv == SECFailure) {
	return(SECFailure);
    }

    if ( LL_CMP(now, >, notAfter) ) {
	expired = PR_TRUE;
	strings = XP_GetDialogStrings(XP_CERT_EXPIRED_DIALOG_STRINGS);
    } else {
	expired = PR_FALSE;
	strings = XP_GetDialogStrings(XP_CERT_NOT_YET_GOOD_DIALOG_STRINGS);
    }
    
    if ( strings != NULL ) {
	/* hostname of site */
	str = SECNAV_GetHostFromURL(ss->sec->url);
	if ( str ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 0, " ");
	}
	
	/* expiration date of cert */
	if ( expired ) {
	    str = DER_UTCDayToAscii(&cert->validity.notAfter);
	} else {
	    str = DER_UTCDayToAscii(&cert->validity.notBefore);
	}
	
	if ( str ) {
	    XP_CopyDialogString(strings, 1, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 1, " ");
	}
	/* current date */
	str = CERT_UTCTime2FormattedAscii (now, "%a %b %d, %Y");
	if (str) {
	    XP_CopyDialogString(strings, 2, str);
	    PORT_Free (str);
	} else {
	    XP_CopyDialogString(strings, 2, " ");	    
	}

	/* mark socket as dialog pending */
	ss->dialogPending = PR_TRUE;

	if ( expired ) {
	    XP_MakeHTMLDialog(proto_win, &certExpiredDialog,
			      XP_CERTISEXP_TITLE_STRING, 
			      strings, (void *)cert);
	} else {
	    XP_MakeHTMLDialog(proto_win, &certNotYetGoodDialog,
			      XP_CERTNOTYETGOOD_TITLE_STRING, 
			      strings, (void *)cert);
	}
	
	XP_FreeDialogStrings(strings);

	return(SECSuccess);
    }
    
    return(SECFailure);
}

typedef struct {
    CERTCertificate *cert;
    CERTCertificate *expiredCert;
} CAExpiredDialogState;

static PRBool
caExpiredDialogHandler(XPDialogState *state, char **argv, int argc,
		       unsigned int button)
{
    char *certstr;
    XPDialogStrings *strings;
    CERTCertificate *cert;
    CERTCertificate *expiredCert;
    CAExpiredDialogState *mystate;
    
    mystate = (CAExpiredDialogState *)state->arg;

    cert = mystate->cert;
    expiredCert = mystate->expiredCert;
    
    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(expiredCert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(state->window, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	return(PR_TRUE);
    }

    if ( button == XP_DIALOG_CONTINUE_BUTTON ) {
	/* keep the cert around for the entire session */
	cert->keepSession = PR_TRUE;
	cert->timeOK = PR_TRUE;
	CERT_DestroyCertificate(expiredCert);
	PORT_Free(mystate);
	RestartHandshakeAfterServerCert(cert, state->proto_win);
    } else { /* CANCEL */
	CERT_DestroyCertificate(expiredCert);
	PORT_Free(mystate);
	SendError(cert);
    }
    
    return(PR_FALSE);
}

static XPDialogInfo caExpiredDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    caExpiredDialogHandler,
    550,
    350
};

static XPDialogInfo caNotYetGoodDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    caExpiredDialogHandler,
    550,
    350
};

static SECStatus
SEC_MakeCAExpiredDialog(void *proto_win, CERTCertificate *cert)
{
    XPDialogStrings *strings;
    char *str;
    SSLSocket *ss;
    int64 now, notBefore, notAfter;
    PRBool expired;
    SECStatus rv;
    CERTCertificate *expiredCert;
    CAExpiredDialogState *mystate;
    
    ss = cert->socketlist->ss;
    expiredCert = CERT_FindExpiredIssuer(CERT_GetDefaultCertDB(), cert);
    if ( expiredCert == NULL ) {
	return(SECFailure);
    }
    
    now = PR_Now();

    rv = CERT_GetCertTimes(expiredCert, &notBefore, &notAfter);
    if ( rv == SECFailure) {
	CERT_DestroyCertificate(expiredCert);
	return(SECFailure);
    }

    if ( LL_CMP(now, >, notAfter) ) {
	expired = PR_TRUE;
	strings = XP_GetDialogStrings(XP_CA_EXPIRED_DIALOG_STRINGS);
    } else {
	expired = PR_FALSE;
	strings = XP_GetDialogStrings(XP_CA_NOT_YET_GOOD_DIALOG_STRINGS);
    }
    
    if ( strings != NULL ) {
	/* hostname of site */
	str = SECNAV_GetHostFromURL(ss->sec->url);
	if ( str ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 0, " ");
	}
	
	str = CERT_GetOrgName(&expiredCert->subject);
	if ( str ) {
	    XP_CopyDialogString(strings, 1, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 1, " ");
	}
	
	/* expiration date of cert */
	if ( expired ) {
	    str = DER_UTCDayToAscii(&expiredCert->validity.notAfter);
	} else {
	    str = DER_UTCDayToAscii(&expiredCert->validity.notBefore);
	}
	
	if ( str ) {
	    XP_CopyDialogString(strings, 2, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 2, " ");
	}
	
	/* current date */
	str = CERT_UTCTime2FormattedAscii (now, "%a %b %d, %Y");
	if (str) {
	    XP_CopyDialogString(strings, 3, str);
	    PORT_Free (str);
	} else {
	    XP_CopyDialogString(strings, 3, " ");	    
	}
	

	/* mark socket as dialog pending */
	ss->dialogPending = PR_TRUE;

	mystate = (CAExpiredDialogState *)
	    PORT_Alloc(sizeof(CAExpiredDialogState));
	if ( mystate == NULL ) {
	    XP_FreeDialogStrings(strings);
	    CERT_DestroyCertificate(expiredCert);
	    return(SECFailure);
	}
	mystate->cert = cert;
	mystate->expiredCert = expiredCert;
	
	if ( expired ) {
	    XP_MakeHTMLDialog(proto_win, &caExpiredDialog,
			      XP_CAEXP_TITLE_STRING, 
			      strings, (void *)mystate);
	} else {
	    XP_MakeHTMLDialog(proto_win, &caNotYetGoodDialog,
			      XP_CANOTYETGOOD_TITLE_STRING, 
			      strings, (void *)mystate);
	}
	
	XP_FreeDialogStrings(strings);

	return(SECSuccess);
    }
    
    CERT_DestroyCertificate(expiredCert);
    return(SECFailure);
}

static void
SetPostWarnOK(CERTCertificate *cert)
{
    SECSocketNode *node;

    node = cert->socketlist;
    
    while ( node ) {
	node->ss->sec->post_ok = PR_TRUE;
	node = node->next;
    }

    return;
}

static PRBool
certPostWarnDialogHandler(XPDialogState *state, char **argv, int argc,
			  unsigned int button)
{
    SECStatus rv;
    char *actionString;
    char *certstr;
    XPDialogStrings *strings;
    CERTCertificate *cert;
    CERTCertTrust trust;
    
    cert = (CERTCertificate *)state->arg;

    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(state->window, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	return(PR_TRUE);
    }

    actionString = XP_FindValueInArgs("action", argv, argc);

    if ( PORT_Strcasecmp(actionString, "sendandwarn") == 0 ) {
	/* send the data and warn next time */
	SetPostWarnOK(cert);
	RestartHandshakeAfterServerCert(cert, state->proto_win);
    } else if ( PORT_Strcasecmp(actionString, "dontsend") == 0 ) {
	/* dont send the data and warn next time */
	SendError(cert);
    } else { /* actionString == send */
	/* send the data and don't warn any more */
	rv = CERT_GetCertTrust(cert, &trust);
	if ( rv == SECSuccess ) {
	    trust.sslFlags &= ( ~CERTDB_SEND_WARN );
	    rv = CERT_ChangeCertTrust(cert->dbhandle, cert, &trust);
	}

	RestartHandshakeAfterServerCert(cert, state->proto_win);
    }
    
    return(PR_FALSE);
}

static XPDialogInfo certPostWarnDialog = {
    XP_DIALOG_OK_BUTTON,
    certPostWarnDialogHandler,
    550,
    350
};

static SECStatus
SEC_MakeCertPostWarnDialog(void *proto_win, CERTCertificate *cert)
{
    XPDialogStrings *strings;
    char buf[20];
    char *str;
    int secret;
    SSLSocket *ss;
    
    ss = cert->socketlist->ss;
    
    strings = XP_GetDialogStrings(XP_SSL_CERT_POST_WARN_DIALOG_STRINGS);
    if ( strings != NULL ) {
	
	str = SECNAV_GetHostFromURL(ss->sec->url);
	if ( str != NULL ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	}
	
	str = CERT_GetOrgName(&cert->subject);
	XP_CopyDialogString(strings, 1, str);
	PORT_Free(str);

	str = CERT_GetOrgName(&cert->issuer);
	XP_CopyDialogString(strings, 2, str);
	PORT_Free(str);

	secret = ss->sec->secretKeyBits;
    
	if ( secret <= 40 ) {
	    str = "Export";
	} else if ( secret <= 80 ) {
	    str = "Medium";
	} else {
	    str = "Highest";
	}
	XP_CopyDialogString(strings, 3, str);

	/* name of cipher */
	str = ssl_cipherName[ss->sec->cipherType];
	XP_CopyDialogString(strings, 4, str);

	/* secret key size */
	XP_SPRINTF(buf, "%d", secret);
	XP_CopyDialogString(strings, 5, buf);

	/* mark socket as dialog pending */
	ss->dialogPending = PR_TRUE;
	
	XP_MakeHTMLDialog(proto_win, &certPostWarnDialog,
			  XP_ENCINFO_TITLE_STRING, strings, (void *)cert);
	XP_FreeDialogStrings(strings);

	return(SECSuccess);
    }
    
    return(SECFailure);
}

static SECStatus
MakeBadCertDialog(int err, void *proto_win, CERTCertificate *cert)
{
    SECStatus rv;
    
    if ( ( err == SEC_ERROR_UNKNOWN_ISSUER ) ||
	( err == SEC_ERROR_CA_CERT_INVALID ) ||
	( err == SEC_ERROR_UNTRUSTED_ISSUER ) ) {
	rv = SEC_MakeSSLServerPanel(proto_win, cert);
    } else if ( err == SSL_ERROR_BAD_CERT_DOMAIN ) {
	rv = SEC_MakeCertDomainDialog(proto_win, cert);
    } else if ( err == SSL_ERROR_POST_WARNING ) {
	rv = SEC_MakeCertPostWarnDialog(proto_win, cert);
    } else if ( err == SEC_ERROR_EXPIRED_CERTIFICATE ) {
	rv = SEC_MakeCertExpiredDialog(proto_win, cert);
    } else if ( err == SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE ) {
	rv = SEC_MakeCAExpiredDialog(proto_win, cert);
    } else {
	rv = SECFailure;
    }
    return(rv);
}

static int
MakeCertDialog(int err, void *proto_win, SSLSocket *ss)
{
    CERTCertificate *cert;
    SECStatus rv;
    
    if ( ss->sec->app_operation == SSLAppOpHeader ) {
	/* just return an error for HEAD operations, rather than a dialog */
	return(-1);
    }

    cert = ss->sec->peerCert;
    AddSockToCert(ss, cert);
    if ( cert->socketcount == 1 ) {
	/* if this is the first time for this cert, then put up a dialog */
	rv = MakeBadCertDialog(err, proto_win, cert);

	if ( rv != SECSuccess ) {
	    return(-1);
	}
    }
    
    /* block on future attempts to handshake */
    SetAlwaysBlock(ss);
    
    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
    return(-2);
}

int
SECNAV_HandleBadCert(void *arg, int fd)
{
    void *proto_win;
    SSLSocket *ss;
    int err;
    SECStatus rv;
    
    err = PORT_GetError();
    ss = ssl_FindSocket(fd);
    if (ss == NULL) {
	return(-1);
    }
    
    /* only handle these errors */
    if ( !ErrorNeedsDialog(err) ) {
	return(-1);
    }
    
    proto_win = (void *)arg;
    
    rv = MakeCertDialog(err, proto_win, ss);
    
    return(rv);
}

static XPDialogInfo certDisplayDialog = {
    XP_DIALOG_OK_BUTTON,
    NULL,
    600,
    400
};

void
SECNAV_ViewUserCertificate(CERTCertificate *cert, void *proto_win, void *arg)
{
    XPDialogStrings *strings;
    char *certstr;
    
    strings = XP_GetDialogStrings(XP_CERT_VIEW_STRINGS);
    if ( strings == NULL ) {
	return;
    }

    if ( !cert ) {
	XP_FreeDialogStrings(strings);
	return;
    }
	
    certstr = CERT_HTMLCertInfo(cert, PR_TRUE);
    XP_SetDialogString(strings, 0, certstr);
	
    XP_MakeHTMLDialog(proto_win, &certDisplayDialog,
		      XP_VIEWPCERT_TITLE_STRING, strings, 0);

    XP_FreeDialogStrings(strings);
    PORT_Free(certstr);
    CERT_DestroyCertificate(cert);
    
    return;
}

SECNAVDeleteCertCallback *
SECNAV_MakeDeleteCertCB(void (* callback)(void *), void *cbarg)
{
    SECNAVDeleteCertCallback *cb;
    
    cb = (SECNAVDeleteCertCallback *)
	PORT_Alloc(sizeof(SECNAVDeleteCertCallback));
    if ( cb ) {
	cb->callback = callback;
	cb->cbarg = cbarg;
    }
    
    return(cb);
}

typedef struct {
    CERTCertificate *cert;
    SECNAVDeleteCertCallback *cb;
    PRBool user_cert;
    PK11SlotInfo *slot;
    CK_OBJECT_HANDLE certID;
} deleteCertState;

static PRBool
deleteCertHandler(XPDialogState *state, char **argv, int argc,
		      unsigned int button)
{
    CERTCertificate *cert;
    SECStatus rv;
    deleteCertState *delstate;

    delstate = (deleteCertState *)state->arg;

    cert = delstate->cert;

    if ( button == XP_DIALOG_OK_BUTTON ) {
	/* if deleting user cert, delete the key along with it */
	if ( delstate->user_cert ) {
	    PK11SlotInfo *slot = delstate->slot;
	    CK_OBJECT_HANDLE certID = delstate->certID;
	    CK_OBJECT_HANDLE privKey = 
				PK11_MatchItem(slot,certID,CKO_PRIVATE_KEY);

	    /* don't delete the key in our database... we may have
	     * multiple certs pointing to the same key.
	     */
	    if (!PK11_IsInternal(slot)) {
		PK11_DestroyTokenObject(slot,privKey);
	    }
	    rv = PK11_DestroyTokenObject(slot,certID);
	} else {
	    /* clear the SSL session cache so that we don't continue talking
	     * to sites who are no longer trusted.
	     */
	    SSL_ClearSessionCache();

	    rv = SEC_DeletePermCertificate(cert);
	}
        if (rv != SECSuccess) {
	    FE_Alert(state->window,XP_GetString(PORT_GetError()));
	}
	if ( delstate->cb->callback ) {
	    (* delstate->cb->callback)(delstate->cb->cbarg);
	}
    }
    
    CERT_DestroyCertificate(cert);
    if (delstate->slot) PK11_FreeSlot(delstate->slot);

    PORT_Free(delstate->cb);
    PORT_Free(delstate);
    
    return(PR_FALSE);
}

static XPDialogInfo deleteUserCertDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    deleteCertHandler,
    600,
    400
};

void
SECNAV_DeleteUserCertificate(CERTCertificate *cert, void *proto_win, void *arg)
{
    char *certstr;
    XPDialogStrings *strings;
    deleteCertState *delstate;
    PK11SlotInfo    *slot;
    CK_OBJECT_HANDLE certID;
    SECNAVDeleteCertCallback *cb;
    
    cb = (SECNAVDeleteCertCallback *)arg;

    certstr = NULL;
    delstate = NULL;
    strings = NULL;
    
    delstate = (deleteCertState *)PORT_ZAlloc(sizeof(deleteCertState));
    if ( delstate == NULL ) {
	goto loser;
    }

    certID = PK11_FindObjectForCert(cert, proto_win, &slot);
    
    if (slot) {
	if (PK11_IsReadOnly(slot)) {
	    char *message = PR_smprintf(XP_GetString(XP_SEC_SLOT_READONLY), 
				PK11_GetTokenName(slot));
    	    FE_Alert(proto_win,message);
	    PORT_Free(message);
	    goto loser;
	}
	delstate->cert = cert;
	delstate->cb = cb;
	delstate->user_cert = PR_TRUE;
	delstate->slot = slot;
	delstate->certID = certID;
    } else {
	/* key not in Database, delete the orphaned cert */
	delstate->cert = cert;
	delstate->cb = cb;
	delstate->user_cert = PR_FALSE;
	delstate->slot = NULL;
	delstate->certID = CK_INVALID_KEY;
    }

    /* get certificate HTML */
    certstr = CERT_HTMLCertInfo(cert, PR_TRUE);
    if ( certstr == NULL ) {
	goto loser;
    }
    
    strings = XP_GetDialogStrings(XP_DELETE_USER_CERT_STRINGS);
    if ( strings == NULL ) {
	goto loser;
    }
    
    /* fill in dialog contents */
    XP_SetDialogString(strings, 0, certstr);
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &deleteUserCertDialog,
		      XP_DELPCERT_TITLE_STRING, strings, (void *)delstate);

    goto done;
    
loser:
    /* if we didn't make the dialog then free the cert */
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    if (slot) PK11_FreeSlot(slot);
    /* if we didn't make the dialog then free the state */
    if ( delstate ) {
	PORT_Free(delstate);
    }

    /* allow our caller to free his state */
    if (cb->callback) {
	(*cb->callback)(cb->cbarg);
    }
    
done:
    if ( strings ) {
	/* free any allocated strings */
	XP_FreeDialogStrings(strings);
    }
    
    if ( certstr ) {
	PORT_Free(certstr);
    }
    
    return;
}

static XPDialogInfo deleteCertDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    deleteCertHandler,
    600,
    400
};

static void
DeleteCertificate(CERTCertificate *cert, void *proto_win,
		  SECNAVDeleteCertCallback *cb, XPDialogStrings *strings,
		  int title)
{
    char *certstr;
    PRBool isca = PR_FALSE;
    deleteCertState *delstate;
    
    certstr = NULL;
    delstate = NULL;
    
    delstate = (deleteCertState *)PORT_ZAlloc(sizeof(deleteCertState));
    if ( delstate == NULL ) {
	goto loser;
    }

    delstate->cert = cert;
    delstate->cb = cb;
    delstate->user_cert = PR_FALSE;

    /* get certificate HTML */
    certstr = CERT_HTMLCertInfo(cert, PR_TRUE);
    if ( certstr == NULL ) {
	goto loser;
    }
	
    /* fill in dialog contents */
    XP_SetDialogString(strings, 0, certstr);
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &deleteCertDialog, title, strings,
		      (void *)delstate);

    goto done;
loser:
    /* if we didn't make the dialog then free the cert */
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    /* if we didn't make the dialog then free the state */
    if ( delstate ) {
	PORT_Free(delstate);
    }

done:
    if ( strings ) {
	/* free any allocated strings */
	XP_FreeDialogStrings(strings);
    }
    
    if ( certstr ) {
	PORT_Free(certstr);
    }
    
    return;
}

void
SECNAV_DeleteSiteCertificate(CERTCertificate *cert, void *proto_win, void *arg)
{
    XPDialogStrings *strings;
    CERTCertTrust *trust;
    SECNAVDeleteCertCallback *cb;
    int title;
    
    cb = (SECNAVDeleteCertCallback *)arg;
    
    strings = NULL;
    
    trust = cert->trust;

    if ( ( ( trust->sslFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
	( ( trust->emailFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ||
	( ( trust->objectSigningFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA ) ) {
	strings = XP_GetDialogStrings(XP_DELETE_CA_CERT_STRINGS);
	title = XP_DELCACERT_TITLE_STRING;
    } else {
	strings = XP_GetDialogStrings(XP_DELETE_SITE_CERT_STRINGS);
	title = XP_DELSCERT_TITLE_STRING;
    }
    
    if ( strings == NULL ) {
	return;
    }

    DeleteCertificate(cert, proto_win, cb, strings, title);

    return;
}

void
SECNAV_DeleteEmailCertificate(CERTCertificate *cert, void *proto_win,
			      void *arg)
{
    XPDialogStrings *strings;
    SECNAVDeleteCertCallback *cb;
    
    cb = (SECNAVDeleteCertCallback *)arg;
    
    strings = XP_GetDialogStrings(XP_DELETE_EMAIL_CERT_STRINGS);
    
    if ( strings == NULL ) {
	return;
    }

    DeleteCertificate(cert, proto_win, cb, strings,
		      XP_DEL_EMAIL_CERT_TITLE_STRING);

    return;
}

static PRBool
editSiteCertHandler(XPDialogState *state, char **argv, int argc,
		    unsigned int button)
{
    char *allowstring;
    char *warnstring;
    PRBool allow;
    PRBool warn;
    CERTCertTrust trust;
    CERTCertificate *cert;
    SECStatus rv;
    
    if ( button == XP_DIALOG_CANCEL_BUTTON ) {
	return(PR_FALSE);
    }

    cert = (CERTCertificate *)state->arg;
    
    allowstring = XP_FindValueInArgs("allow", argv, argc);
    warnstring = XP_FindValueInArgs("postwarn", argv, argc);

    allow = PR_FALSE;
    if ( allowstring ) {
	if ( PORT_Strncmp(allowstring, "yes", 3) == 0 ) {
	    allow = PR_TRUE;
	}
    }

    warn = PR_FALSE;
    if ( warnstring ) {
	if ( PORT_Strncmp(warnstring, "yes", 3) == 0 ) {
	    warn = PR_TRUE;
	}
    }

    rv = CERT_GetCertTrust(cert, &trust);
    if ( rv == SECSuccess ) {
	if ( allow ) {
	    trust.sslFlags |= CERTDB_TRUSTED;
	} else {
	    trust.sslFlags &= ( ~CERTDB_TRUSTED );
	}
	if ( warn ) {
	    trust.sslFlags |= CERTDB_SEND_WARN;
	} else {
	    trust.sslFlags &= ( ~CERTDB_SEND_WARN );
	}
	rv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), cert, &trust);

	/* clear the SSL session cache so that we don't continue talking
	 * to sites who are no longer trusted.
	 */
	SSL_ClearSessionCache();

    }

    CERT_DestroyCertificate(cert);
    
    return(PR_FALSE);
}

static XPDialogInfo editSiteCertDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    editSiteCertHandler,
    600,
    400
};

static void
editSiteCertificate(void *proto_win, CERTCertificate *cert)
{
    char *certstr;
    XPDialogStrings *strings;
    int sslflags;

    sslflags = cert->trust->sslFlags;

    /* get certificate HTML */
    certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

    strings = XP_GetDialogStrings(XP_EDIT_SITE_CERT_STRINGS);
    if ( strings == NULL ) {
	return;
    }
    
    /* fill in dialog contents */
    XP_SetDialogString(strings, 0, certstr);

    if ( sslflags & CERTDB_TRUSTED ) {
	XP_CopyDialogString(strings, 1, "checked");
	XP_CopyDialogString(strings, 2, "");
    } else {
	XP_CopyDialogString(strings, 1, "");
	XP_CopyDialogString(strings, 2, "checked");
    }

    if ( sslflags & CERTDB_SEND_WARN ) {
	XP_CopyDialogString(strings, 3, "checked");
    } else {
	XP_CopyDialogString(strings, 3, "");
    }
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &editSiteCertDialog,
		      XP_EDITSCERT_TITLE_STRING, strings, (void *)cert);

    /* free any allocated strings */
    XP_FreeDialogStrings(strings);
    if ( certstr ) {
	PORT_Free(certstr);
    }

    return;
}

static PRBool
editCACertHandler(XPDialogState *state, char **argv, int argc,
		  unsigned int button)
{
    char *acceptSSLString, *acceptEmailString, *acceptObjectSigningString;
    char *warnstring;
    PRBool warn;
    CERTCertTrust trust;
    CERTCertificate *cert;
    SECStatus rv;
    PRBool acceptSSL = PR_FALSE;
    PRBool acceptEmail = PR_FALSE;
    PRBool acceptObjectSigning = PR_FALSE;
    unsigned int certtype;
    
    cert = (CERTCertificate *)state->arg;
    
    certtype = cert->nsCertType;

    /* if no CA bits in cert type, then set all CA bits */
    if ( ! ( certtype & NS_CERT_TYPE_CA ) ) {
	certtype = NS_CERT_TYPE_CA;
    }

    if ( button == XP_DIALOG_CANCEL_BUTTON ) {
	return(PR_FALSE);
    }

    acceptSSLString = XP_FindValueInArgs("acceptssl", argv, argc);
    acceptEmailString = XP_FindValueInArgs("acceptsmime", argv, argc);
    acceptObjectSigningString = XP_FindValueInArgs("acceptobjectsigning",
						   argv, argc);
    warnstring = XP_FindValueInArgs("postwarn", argv, argc);

    if ( acceptSSLString ) {
	if ( PORT_Strncmp(acceptSSLString, "yes", 3) == 0 ) {
	    acceptSSL = PR_TRUE;
	}
    }
    if ( acceptEmailString ) {
	if ( PORT_Strncmp(acceptEmailString, "yes", 3) == 0 ) {
	    acceptEmail = PR_TRUE;
	}
    }
    if ( acceptObjectSigningString ) {
	if ( PORT_Strncmp(acceptObjectSigningString, "yes", 3) == 0 ) {
	    acceptObjectSigning = PR_TRUE;
	}
    }

    warn = PR_FALSE;
    if ( warnstring ) {
	if ( PORT_Strncmp(warnstring, "yes", 3) == 0 ) {
	    warn = PR_TRUE;
	}
    }

    rv = CERT_GetCertTrust(cert, &trust);
    if ( rv == SECSuccess ) {

	if ( certtype & NS_CERT_TYPE_SSL_CA ) {
	    trust.sslFlags |= CERTDB_VALID_CA;
	}
	
	if ( acceptSSL ) {
	    trust.sslFlags |= CERTDB_TRUSTED_CA;
	} else {
	    trust.sslFlags &= ( ~CERTDB_TRUSTED_CA );
	}
	if ( warn ) {
	    trust.sslFlags |= CERTDB_SEND_WARN;
	} else {
	    trust.sslFlags &= ( ~CERTDB_SEND_WARN );
	}

	if ( certtype & NS_CERT_TYPE_EMAIL_CA ) {
	    trust.emailFlags |= CERTDB_VALID_CA;
	}
	
	if ( acceptEmail ) {
	    trust.emailFlags |= CERTDB_TRUSTED_CA;
	} else {
	    trust.emailFlags &= ( ~CERTDB_TRUSTED_CA );
	}

	if ( certtype & NS_CERT_TYPE_OBJECT_SIGNING_CA ) {
	    trust.objectSigningFlags |= CERTDB_VALID_CA;
	}
	
	if ( acceptObjectSigning ) {
	    trust.objectSigningFlags |= CERTDB_TRUSTED_CA;
	} else {
	    trust.objectSigningFlags &= ( ~CERTDB_TRUSTED_CA );
	}

	/* clear the SSL session cache so that we don't continue talking
	 * to sites who are no longer trusted.
	 */
	SSL_ClearSessionCache();
	
	rv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), cert, &trust);
    }

    CERT_DestroyCertificate(cert);

    return(PR_FALSE);
}

static XPDialogInfo editCACertDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    editCACertHandler,
    600,
    400
};

static void
editCACertificate(void *proto_win, CERTCertificate *cert)
{
    char *certstr;
    XPDialogStrings *strings;
    unsigned int certtype;
    PRBool acceptSSL = PR_FALSE;
    PRBool acceptEmail = PR_FALSE;
    PRBool acceptObjectSigning = PR_FALSE;
    char *sslstr = NULL, *emailstr = NULL, *objectsigningstr = NULL;
    char *name;
    
    certtype = cert->nsCertType;

    /* if no CA bits in cert type, then set all CA bits */
    if ( ! ( certtype & NS_CERT_TYPE_CA ) ) {
	certtype = NS_CERT_TYPE_CA;
    }

    if ( cert->trust->sslFlags & CERTDB_TRUSTED_CA ) {
	acceptSSL = PR_TRUE;
    }

    if ( cert->trust->emailFlags & CERTDB_TRUSTED_CA ) {
	acceptEmail = PR_TRUE;
    }

    if ( cert->trust->objectSigningFlags & CERTDB_TRUSTED_CA ) {
	acceptObjectSigning = PR_TRUE;
    }

    if ( certtype & NS_CERT_TYPE_SSL_CA ) {
	name = "acceptssl";
	sslstr = PR_smprintf("<input type=checkbox name=%s value=yes %s>%s",
			     name, acceptSSL ? "checked" : "",
			     XP_GetString(XP_CA_CERT_SSL_OK_STRING));
    }

    if ( sslstr == NULL ) {
	sslstr = "";
    }

    if ( certtype & NS_CERT_TYPE_EMAIL_CA ) {
	name = "acceptsmime";
	emailstr = PR_smprintf("<input type=checkbox name=%s value=yes %s>%s",
			       name, acceptEmail ? "checked" : "",
			       XP_GetString(XP_CA_CERT_EMAIL_OK_STRING));
    }
    if ( emailstr == NULL ) {
	emailstr = "";
    }

    if ( certtype & NS_CERT_TYPE_OBJECT_SIGNING_CA ) {
	name = "acceptobjectsigning";
	objectsigningstr =
	    PR_smprintf("<input type=checkbox name=%s value=yes %s>%s",
			name, acceptObjectSigning ? "checked" : "",
			XP_GetString(XP_CA_CERT_OBJECT_SIGNING_OK_STRING));
    }
    if (objectsigningstr == NULL ) {
	objectsigningstr = "";
    }

    /* get certificate HTML */
    certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

    strings = XP_GetDialogStrings(XP_EDIT_CA_CERT_STRINGS);
    if ( strings == NULL ) {
	return;
    }
    
    /* fill in dialog contents */
    XP_SetDialogString(strings, 0, certstr);

    XP_CopyDialogString(strings, 1, sslstr);
    XP_CopyDialogString(strings, 2, emailstr);
    XP_CopyDialogString(strings, 3, objectsigningstr);

    if ( cert->trust->sslFlags & CERTDB_SEND_WARN ) {
	XP_CopyDialogString(strings, 4, "checked");
    } else {
	XP_CopyDialogString(strings, 4, "");
    }
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &editCACertDialog,
		      XP_EDITCACERT_TITLE_STRING, strings, (void *)cert);

    /* free any allocated strings */
    XP_FreeDialogStrings(strings);

    if ( *sslstr != 0 ) {
	PORT_Free(sslstr);
    }

    if ( *emailstr != 0 ) {
	PORT_Free(emailstr);
    }

    if ( *objectsigningstr != 0 ) {
	PORT_Free(objectsigningstr);
    }

    if ( certstr ) {
	PORT_Free(certstr);
    }

    return;
}

void
SECNAV_EditSiteCertificate(CERTCertificate *cert, void *proto_win,
			   void *arg)
{
    PORT_Assert(cert->trust != NULL);
    
    if ( ( cert->trust->sslFlags & CERTDB_VALID_CA ) ||
	( cert->trust->emailFlags & CERTDB_VALID_CA ) ||
	( cert->trust->objectSigningFlags & CERTDB_VALID_CA ) ) {
	editCACertificate(proto_win, cert);
    } else {
	editSiteCertificate(proto_win, cert);
    }
    
    return;
}

/*
 * get the private key and restart the connection after getting
 * a client cert from the user
 */
SECStatus
GetKeyAndRestart(char *certname, void *proto_win, CERTCertificate *cert,
		 CERTCertificate *serverCert)
{
    SECKEYPrivateKey *privkey;
    CERTCertificateList *certChain;
    SECStatus rv;
    
    certChain = NULL;
    
    privkey = PK11_FindKeyByAnyCert(cert, proto_win);
    if ( privkey == NULL ) {
	goto loser;
    }
	
    certChain = CERT_CertChainFromCert(CERT_GetDefaultCertDB(), cert);
    if ( certChain == NULL ) {
	goto loser;
    }
    
    RestartHandshakeAfterClientAuth(serverCert, cert, privkey, certChain);

    rv = SECSuccess;
    goto done;
    
loser:
    rv = SECFailure;
    
done:
    if ( privkey ) {
	SECKEY_DestroyPrivateKey(privkey);
    }
    
    return(rv);
}

static PRBool
clientAuthNoCertDialogHandler(XPDialogState *state, char **argv, int argc,
			      unsigned int button)
{
    CERTCertificate *serverCert;
    SSLSocket *ss;

    ss = (SSLSocket *)state->arg;
    serverCert = ss->sec->peerCert;
    RestartHandshakeAfterClientAuth(serverCert, NULL, NULL, NULL);
    return(PR_FALSE);
}

static XPDialogInfo clientAuthNoCertDialog = {
    XP_DIALOG_OK_BUTTON,
    clientAuthNoCertDialogHandler,
    550,
    350
};

static SECStatus
MakeClientAuthNoCertDialog(void *proto_win,
			   CERTCertificate *serverCert,
			   SSLSocket *ss,
			   CERTDistNames *caNames)
{
    XPDialogStrings *strings;
    char *str;
    
    strings = XP_GetDialogStrings(XP_NO_CERT_CLIENT_AUTH_DIALOG_STRINGS);
    if ( strings == NULL ) {
	goto loser;
    }

    str = SECNAV_GetHostFromURL(ss->sec->url);
    if ( str != NULL ) {
	XP_CopyDialogString(strings, 0, str);
	PORT_Free(str);
    } else {
	XP_CopyDialogString(strings, 0, " ");
    }
    
    /* mark socket as dialog pending */
    ss->dialogPending = PR_TRUE;
	
    XP_MakeHTMLDialog(proto_win, &clientAuthNoCertDialog,
		      XP_NOPCERT_TITLE_STRING, strings, (void *)ss);
    XP_FreeDialogStrings(strings);

    return(SECSuccess);
    
loser:
    return(SECFailure);
}

typedef struct {
    CERTCertificate *cert;
    CERTCertificate *serverCert;
    void *proto_win;
    char *certname;
    char *renewalurl;
} clientCertExpiredState;

static void
fetchURL(void *closure)
{
    URL_Struct *fetchurl;
    
    fetchurl = (URL_Struct *)closure;
    
    FE_GetURL((MWContext *)fetchurl->sec_private, fetchurl);
    return;
}

static void
HandleInternalCertRenewalURL(void *proto_win, CERTCertificate *cert,
			     char *urlstring)
{
    URL_Struct *fetchurl = NULL;
    char *serialnumber = NULL;
    char *fetchstring = NULL;
    
    serialnumber = secmoz_GetCertSNString(cert);
    
    if ( serialnumber == NULL ) {
	goto loser;
    }

    fetchstring = (char *)PORT_Alloc(PORT_Strlen(serialnumber) +
				   PORT_Strlen(urlstring) + 1);
    if ( fetchstring == NULL ) {
	goto loser;
    }

    /* copy the base url data */
    PORT_Strcpy(fetchstring, urlstring);

    /* copy the serial number */
    XP_STRCAT(fetchstring, serialnumber);
    
    fetchurl = NET_CreateURLStruct(fetchstring, NET_DONT_RELOAD);
    StrAllocCopy(fetchurl->window_target, "_CA_RENEW");

    fetchurl->sec_private = (void *)proto_win;

    (void)FE_SetTimeout(fetchURL, (void *)fetchurl, 0);

    goto done;
    
loser:
    if ( fetchurl ) {
	NET_FreeURLStruct(fetchurl);
    }

done:
    if ( serialnumber ) {
	PORT_Free(serialnumber);
    }

    if ( fetchstring ) {
	PORT_Free(fetchstring);
    }
    
    return;
}

static PRBool
clientCertExpiredDialogHandler(XPDialogState *state, char **argv, int argc,
			       unsigned int button)
{
    clientCertExpiredState *mystate;
    SECStatus rv;
    char *buttonString;
    
    mystate = (clientCertExpiredState *)state->arg;
    
    if ( button == 0 ) {
	buttonString = XP_FindValueInArgs("button", argv, argc);
	if ( buttonString != NULL ) {
	    if ( PORT_Strcasecmp(buttonString,
			       XP_GetString(XP_SEC_RENEW)) == 0 ) {
		HandleInternalCertRenewalURL(mystate->proto_win,
					     mystate->cert,
					     mystate->renewalurl);
	    }
	}
    }
    if ( button == XP_DIALOG_CONTINUE_BUTTON ) {
	rv = GetKeyAndRestart(mystate->certname, mystate->proto_win, mystate->cert,
			      mystate->serverCert);
	if ( rv != SECSuccess ) {
	    SendError(mystate->serverCert);
	}
    } else {
	SendError(mystate->serverCert);
    }
    
    PORT_Free(mystate->renewalurl);
    PORT_Free(mystate);
    
    return(PR_FALSE);
}

static XPDialogInfo clientCertExpiredDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    clientCertExpiredDialogHandler,
    500,
    250,
};

static SECStatus
MakeClientCertExpiredDialog(CERTCertificate *cert,
			    char *certname,
			    void *proto_win,
			    CERTCertificate *serverCert,
			    SECCertTimeValidity validity)
{
    XPDialogStrings *strings;
    clientCertExpiredState *mystate;
    char *renewalurl;
    
    mystate = (clientCertExpiredState *)
	PORT_Alloc(sizeof(clientCertExpiredState));

    if ( mystate == NULL ) {
	return(SECFailure);
    }

    /* is there a renewal url? */
    renewalurl = CERT_FindCertURLExtension(cert, SEC_OID_NS_CERT_EXT_CERT_RENEWAL_URL,0);

    mystate->cert = cert;
    mystate->serverCert = serverCert;
    mystate->proto_win = proto_win;
    mystate->certname = certname;
    mystate->renewalurl = renewalurl;

    if ( validity == secCertTimeExpired ) {
	if ( renewalurl ) {
	    strings = XP_GetDialogStrings(XP_CLIENT_CERT_EXPIRED_RENEWAL_DIALOG_STRINGS);
	} else {
	    strings = XP_GetDialogStrings(XP_CLIENT_CERT_EXPIRED_DIALOG_STRINGS);
	}
    } else {
	    strings = XP_GetDialogStrings(XP_CLIENT_CERT_NOT_YET_GOOD_DIALOG_STRINGS);
    }
    
    if ( strings != NULL ) {
	XP_MakeHTMLDialog(proto_win, &clientCertExpiredDialog,
			  XP_CLCERTEXP_TITLE_STRING, strings,
			  (void *)mystate);
	XP_FreeDialogStrings(strings);

	return(SECSuccess);
    }
    
    PORT_Free(mystate);
    
    return(SECFailure);
}

static PRBool
clientAuthWhichCertDialogHandler(XPDialogState *state, char **argv, int argc,
				 unsigned int button)
{
    char *certstr;
    XPDialogStrings *strings;
    CERTCertificate *serverCert, *cert = NULL;
    SSLSocket *ss;
    char *certname;
    SECStatus rv;
    SECCertTimeValidity validity;
    
    ss = (SSLSocket *)state->arg;
    serverCert = ss->sec->peerCert;
    
    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(serverCert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(state->window, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	return(PR_TRUE);
    }

    if ( button == XP_DIALOG_CANCEL_BUTTON ) {
	/* Don't send a cert, but try to continue anyway. */
	RestartHandshakeAfterClientAuth(serverCert, NULL, NULL, NULL);
	goto done;
    }

    if ( button == XP_DIALOG_CONTINUE_BUTTON ) {
	certname = XP_FindValueInArgs("cert", argv, argc);

	if ( certname == NULL ) {
	    /* no cert available */
	    RestartHandshakeAfterClientAuth(serverCert, NULL, NULL, NULL);
	    goto done;
	}
	
	cert = CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(),
						  certname);
	if (cert == NULL) {
	    cert = PK11_FindCertFromNickname(certname,state->proto_win);
	}

	if ( cert == NULL ) {
	    goto loser;
	}
	validity = CERT_CheckCertValidTimes(cert, PR_Now());
	if ( validity != secCertTimeValid ) {
	    rv = MakeClientCertExpiredDialog(cert, certname,
					     state->proto_win, serverCert,
					     validity);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	    goto done;
	}
	
	rv = GetKeyAndRestart(certname, state->proto_win, cert, serverCert);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
	
	goto done;
    }

loser:
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    
    SendError(serverCert);

done:
    return(PR_FALSE);
}

static XPDialogInfo clientAuthWhichCertDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    clientAuthWhichCertDialogHandler,
    550,
    350
};

static SECStatus
SEC_MakeClientAuthDialog(void *proto_win,
			 CERTCertificate *serverCert,
			 SSLSocket *ss,
			 CERTDistNames *caNames)
{
    XPDialogStrings *strings;
    char buf[20];
    char *str, *strtmp;
    int i, len;
    int secret;
    CERTCertNicknames *nicknames;
    
    nicknames = CERT_GetCertNicknames(CERT_GetDefaultCertDB(),
				      SEC_CERT_NICKNAMES_USER, proto_win);
    if ( nicknames == NULL || ( nicknames->numnicknames == 0 ) ) {
	return(MakeClientAuthNoCertDialog(proto_win, serverCert, ss, caNames));
    }

    strings = XP_GetDialogStrings(XP_WHICH_CERT_CLIENT_AUTH_DIALOG_STRINGS);
    if ( strings != NULL ) {
	str = SECNAV_GetHostFromURL(ss->sec->url);
	if ( str != NULL ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	}
	
	str = CERT_GetOrgName(&serverCert->subject);
	XP_CopyDialogString(strings, 1, str);
	PORT_Free(str);

	str = CERT_GetOrgName(&serverCert->issuer);
	XP_CopyDialogString(strings, 2, str);
	PORT_Free(str);

	secret = ss->sec->secretKeyBits;
    
	if ( secret <= 40 ) {
	    str = "Export";
	} else if ( secret <= 80 ) {
	    str = "Medium";
	} else {
	    str = "Highest";
	}
	XP_CopyDialogString(strings, 3, str);

	/* name of cipher */
	str = (ss->version == SSL_LIBRARY_VERSION_2) ?
	    ssl_cipherName[ss->sec->cipherType] :
	    ssl3_cipherName[ss->sec->cipherType];
	XP_CopyDialogString(strings, 4, str);

	/* secret key size */
	XP_SPRINTF(buf, "%d", secret);
	XP_CopyDialogString(strings, 5, buf);

	/* build menu of cert names */
	strtmp = str = (char *)PORT_Alloc(nicknames->totallen +
					nicknames->numnicknames * 10 + 10);
	if ( str ) {
	    for ( i = 0; i < nicknames->numnicknames; i++ ) {
		PORT_Memcpy(strtmp, "<option>", 8);
		strtmp += 8;
		len = PORT_Strlen(nicknames->nicknames[i]);
		PORT_Memcpy(strtmp, nicknames->nicknames[i], len);
		strtmp += len;
	    }
	    *strtmp = 0;
	    XP_CopyDialogString(strings, 6, str);
	    PORT_Free(str);
	}

	/* mark socket as dialog pending */
	ss->dialogPending = PR_TRUE;
	
	XP_MakeHTMLDialog(proto_win, &clientAuthWhichCertDialog,
			  XP_SELPCERT_TITLE_STRING, strings, (void *)ss);
	XP_FreeDialogStrings(strings);

	return(SECSuccess);
    }
    
    return(SECFailure);
}

int
SECNAV_MakeClientAuthDialog(void *proto_win, int fd, CERTDistNames *caNames)
{
    CERTCertificate *cert;
    SECStatus rv;
    SSLSocket *ss;
    
    ss = ssl_FindSocket(fd);
    
    if ( ss->sec->app_operation == SSLAppOpHeader ) {
	/* just return an error for HEAD operations, rather than a dialog */
	return(-1);
    }

    cert = ss->sec->peerCert;
    AuthAddSockToCert(ss, cert);
    if ( cert->authsocketcount == 1 ) {
	/* if this is the first time for this cert, then put up a dialog */
	rv = SEC_MakeClientAuthDialog(proto_win, cert, ss, caNames);

	if ( rv != SECSuccess ) {
	    return(-1);
	}
    }
    
    /* block on future attempts to handshake */
    SetAlwaysBlock(ss);
    
    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
    return(-2);
}

static PRBool
browserSecInfoDialogHandler(XPDialogState *state, char **argv, int argc,
			    unsigned int button)
{
    if ( button == 0 ) {
	FE_GetURL((MWContext *)state->arg,
		  NET_CreateURLStruct("about:document", NET_DONT_RELOAD));
    }
    return(PR_FALSE);
}


static XPDialogInfo browserSecInfoDialog = {
    0,
    browserSecInfoDialogHandler,
    550,
    275
};


void
SECNAV_BrowserSecInfoDialog(void *proto_win, URL_Struct *url)
{
    XPDialogStrings *dialogstrings;

    if ( ( url == NULL ) || ( proto_win == NULL ) ) {
	return;
    }
    
    switch ( url->security_on ) {
      case SSL_SECURITY_STATUS_OFF:
	dialogstrings =
	    XP_GetDialogStrings(XP_BROWSER_SEC_INFO_CLEAR_STRINGS);
	break;
      case SSL_SECURITY_STATUS_ON_LOW:
	dialogstrings =
	    XP_GetDialogStrings(XP_BROWSER_SEC_INFO_ENCRYPTED_STRINGS);
	break;
      case SSL_SECURITY_STATUS_ON_HIGH:
      case SSL_SECURITY_STATUS_FORTEZZA:
	dialogstrings =
	    XP_GetDialogStrings(XP_BROWSER_SEC_INFO_ENCRYPTED_STRINGS);
	break;
      default:
	PORT_Assert (0);
	return;
    }

    XP_MakeHTMLDialog(proto_win, &browserSecInfoDialog,
		      XP_SECINFO_TITLE_STRING, dialogstrings, (void *)proto_win);
    XP_FreeDialogStrings(dialogstrings);
}

typedef struct {
    CERTCertificate *cert;
    CERTDERCerts *derCerts;
    char *nickname;
    void *proto_win;
} UserCertDownloadPanelState;

static void
user_cert_pkcs12_cb(void *closure)
{
    UserCertDownloadPanelState *mystate = (UserCertDownloadPanelState *)closure;

    /* XXX the certificate is destroyed within PKCS12 code.  there is no
     * need to destroy it here.  this is due to the initial behavior of
     * SECNAV_SelectCert and PKCS12 implementation.  This should be cleaned
     * up.
     */
    SECNAV_ExportPKCS12Object(mystate->cert, mystate->proto_win, 
    			      (void *)mystate->cert->nickname);

done:
    if (mystate->derCerts) {
	PORT_FreeArena(mystate->derCerts->arena, PR_FALSE);
    }
    if (mystate->nickname) {
	PORT_Free(mystate->nickname);
    }
    PORT_Free(mystate);
}

static PRBool
userCertSaveHandler(XPDialogState *state, char **argv, int argc,
		    unsigned int button)
{
    UserCertDownloadPanelState *mystate;

    mystate = (UserCertDownloadPanelState *)(state->arg);

    if (button == 0) {
	state->deleteCallback = user_cert_pkcs12_cb;
	state->cbarg = (void *)mystate;
	return PR_FALSE;
    }

    if (mystate->derCerts) {
	PORT_FreeArena(mystate->derCerts->arena, PR_FALSE);
    }
    if (mystate->cert) {
	CERT_DestroyCertificate(mystate->cert);
    }
    if (mystate->nickname) {
	PORT_Free(mystate->nickname);
    }
    PORT_Free(mystate);

    return PR_FALSE;
}

static XPDialogInfo saveDialog = {
    XP_DIALOG_CONTINUE_BUTTON,
    userCertSaveHandler,
    500,
    350
};

static void
user_cert_save_cb(void *closure)
{
    UserCertDownloadPanelState *mystate = (UserCertDownloadPanelState *)closure;
    XPDialogStrings *strings;

    strings = XP_GetDialogStrings(XP_USER_CERT_SAVE_STRINGS);
    if (strings) {
	XP_MakeHTMLDialog(mystate->proto_win, &saveDialog,
			  XP_USER_CERT_SAVE_TITLE,
			  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }
}

static PRBool
userCertDownloadHandler(XPDialogState *state, char **argv, int argc,
		unsigned int button)
{
    UserCertDownloadPanelState *mystate;
    char *nickname, *str, *caname;
    CERTCertDBHandle *handle;
    PK11SlotInfo *slot;
    int numcacerts;
    SECItem *cacerts;
    SECStatus rv;
    XPDialogStrings *strings;

    mystate = (UserCertDownloadPanelState *)(state->arg);
    handle = mystate->cert->dbhandle;

    if (button == XP_DIALOG_CANCEL_BUTTON) {
	goto done;
    }

    if (button == XP_DIALOG_MOREINFO_BUTTON) {
	strings = XP_GetDialogStrings(XP_USER_CERT_DL_MOREINFO_STRINGS);
	if (strings) {
	    caname = CERT_GetOrgName(&mystate->cert->issuer);
	    if (caname == NULL) {
		caname = CERT_GetCommonName(&mystate->cert->issuer);
	    }
	    if (caname != NULL) {
		XP_CopyDialogString(strings, 0, caname);
		PORT_Free(caname);
	    }
	    XP_MakeHTMLDialog(state->window, &infoDialog,
			      XP_CERT_DL_MOREINFO_TITLE_STRING,
			      strings, NULL);
	    XP_FreeDialogStrings(strings);
	}
	return PR_TRUE;
    }

    if (button == 0) {
	SECNAV_DisplayCertDialog(state->window, mystate->cert);
	return PR_TRUE;
    }

    if (mystate->nickname == NULL) {
	nickname = XP_FindValueInArgs("nickname", argv, argc);
	compress_spaces(nickname);
    
	if ((nickname == NULL) || (PORT_Strlen(nickname) == 0) ||
	    SEC_CertNicknameConflict(nickname, &mystate->cert->derSubject,
				     handle)) {
	    FE_Alert(state->window, XP_GetString(SEC_ERROR_BAD_NICKNAME));
	    return PR_TRUE;
	}
    } else {
	nickname = mystate->nickname;
    }

    slot = PK11_ImportCertForKey(mystate->cert, nickname, state->window);
    if (slot == NULL) {
	FE_Alert(mystate->proto_win, XP_GetString(PORT_GetError()));
	goto done;
    }
    PK11_FreeSlot(slot);

    /* now add other ca certs */
    numcacerts = mystate->derCerts->numcerts - 1;
	   
    if (numcacerts) {
	cacerts = mystate->derCerts->rawCerts+1;
        /* should we try to put the chain on the card first */ 
	    
	rv = CERT_ImportCAChain(cacerts, numcacerts, certUsageSSLCA);
	if ( rv != SECSuccess ) {
	    FE_Alert(mystate->proto_win, XP_GetString(PORT_GetError()));
	    goto done;
	}
    }

    str = XP_FindValueInArgs("useformail", argv, argc);
    if ((str != NULL) && (PORT_Strcmp(str, "yes") == 0)) {
	PREF_SetCharPref("security.default_mail_cert", mystate->cert->nickname);
	PREF_SavePrefFile();
    }

    /* Since we succeeded, we now pop up the save dialog. */
    state->deleteCallback = user_cert_save_cb;
    state->cbarg = (void *)mystate;
    /* Don't destroy state yet.  We still need it for save dialog. */
    return PR_FALSE;

done:
    if (mystate->derCerts) {
	PORT_FreeArena(mystate->derCerts->arena, PR_FALSE);
    }
    if (mystate->cert) {
	CERT_DestroyCertificate(mystate->cert);
    }
    if (mystate->nickname) {
	PORT_Free(mystate->nickname);
    }
    PORT_Free(mystate);
    return PR_FALSE;
}

static XPDialogInfo userCertDownloadDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_MOREINFO_BUTTON,
    userCertDownloadHandler,
    600,
    400
};

static char *
default_nickname(CERTCertificate *cert)
{
    char *username = NULL;
    char *caname = NULL;
    char *nickname = NULL;
    int count;
    CERTCertificate *dummycert;
    
    username = CERT_GetCommonName(&cert->subject);
    if ( username == NULL ) {
	username = PORT_Strdup("");
    }
    if ( username == NULL ) {
	goto loser;
    }
    
    caname = CERT_GetOrgName(&cert->issuer);
    if ( caname == NULL ) {
	caname = PORT_Strdup("");
    }
    if ( caname == NULL ) {
	goto loser;
    }
    
    count = 1;
    while ( 1 ) {
	
	if ( count == 1 ) {
	    nickname = PR_smprintf("%s's %s ID", username, caname);
	} else {
	    nickname = PR_smprintf("%s's %s ID #%d", username,
				   caname, count);
	}
	if ( nickname == NULL ) {
	    goto loser;
	}

	compress_spaces(nickname);
	
	/* look up the nickname to make sure it isn't in use already */
	dummycert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), nickname);

	if ( dummycert == NULL ) {
	    goto done;
	}
	
	/* found a cert, destroy it and loop */
	CERT_DestroyCertificate(dummycert);

	/* free the nickname */
	PORT_Free(nickname);

	count++;
    }
    
loser:
    if ( nickname ) {
	PORT_Free(nickname);
    }

    nickname = NULL;
    
done:
    if ( caname ) {
	PORT_Free(caname);
    }
    if ( username ) {
	PORT_Free(username);
    }
    
    return(nickname);
}

void
SECNAV_MakeUserCertDownloadDialog(void *proto_win, CERTCertificate *cert,
				  CERTDERCerts *derCerts)
{
    UserCertDownloadPanelState *mystate;
    CERTCertificate *tmpcert;
    XPDialogStrings *strings;
    char *nickname, *str;
    
    mystate = (UserCertDownloadPanelState *)PORT_ZAlloc(
					    sizeof(UserCertDownloadPanelState));
    if ( mystate == NULL )
	return;

    strings = XP_GetDialogStrings(XP_USER_CERT_DOWNLOAD_STRINGS);
    if (strings == NULL)
	goto loser;
    
    mystate->cert = cert;
    mystate->derCerts = derCerts;
    mystate->proto_win = proto_win;

    if ((mystate->cert->subjectList != NULL) &&
	(mystate->cert->subjectList->entry != NULL) &&
	(mystate->cert->subjectList->entry->nickname != NULL)) {
	/* We already have a nickname for this DN, so we just show it to
	 * the user. */
	nickname = mystate->cert->subjectList->entry->nickname;
	XP_CopyDialogString(strings, 1, nickname);
	mystate->nickname = PORT_Strdup(nickname);
    } else {
	/* We don't have a nickname for this DN yet, so we let the
	 * user edit it. */
	nickname = default_nickname(cert);
	if (nickname == NULL)
	    goto loser;
	str = PR_smprintf("<input type=text size=60 name=nickname "
			  "value=\042%s\042>", nickname);
	PORT_Free(nickname);
	if (str == NULL)
	    goto loser;
	XP_CopyDialogString(strings, 1, str);
	/* Add message telling the user he can edit the nickname */
	str = XP_GetString(XP_USER_CERT_NICKNAME_STRINGS);
	XP_CopyDialogString(strings, 0, str);
    }

    str = CERT_GetCommonName(&cert->subject);
    if (str != NULL) {
	XP_CopyDialogString(strings, 2, str);
	PORT_Free(str);
    }
    str = CERT_GetOrgName(&cert->issuer);
    if (str == NULL) {
	str = CERT_GetCommonName(&cert->issuer);
    }
    if (str != NULL) {
	XP_CopyDialogString(strings, 3, str);
	PORT_Free(str);
    }

    if (cert->emailAddr != NULL) {
	str = XP_GetString(XP_SET_EMAIL_CERT_STRING);

	tmpcert = SECNAV_GetDefaultEMailCert(proto_win);
	if (tmpcert == NULL) {
	    str = PR_smprintf(str, "checked");
	} else {
	    str = PR_smprintf(str, "");
	    CERT_DestroyCertificate(tmpcert);
	}
	if (str != NULL) {
	    XP_CopyDialogString(strings, 4, str);
	    PORT_Free(str);
	}
    }

    XP_MakeHTMLDialog(proto_win, &userCertDownloadDialog,
		      XP_NEWPCERT_TITLE_STRING, strings, (void *)mystate);
    XP_FreeDialogStrings(strings);

    return;

loser:
    if (strings != NULL)
	XP_FreeDialogStrings(strings);
    if (mystate->nickname != NULL)
	PORT_Free(mystate->nickname);
    PORT_Free(mystate);
    return;
}

typedef struct {
    LO_Element *form;
    char **pValue;
    PRBool *pDone;
} KeyGenFormState;

typedef struct {
    int keysize;
    char *challenge;
    KeyType type;
    PQGParams *pqg;
    KeyGenFormState *formState;
    void *win;
    PK11SlotList *slotList;
    PK11SlotInfo *slot;
} KeyGenDialogState;

static SECStatus
GenKey(int keysize, char *challenge, KeyType type, PQGParams *pqg,
       void *proto_win, KeyGenFormState *formState, PK11SlotInfo *slot)
{
    SECKEYPrivateKey *privateKey = NULL;
    SECKEYPublicKey *publicKey = NULL;
    CERTSubjectPublicKeyInfo *spkInfo = NULL;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    SECItem spkiItem;
    SECItem pkacItem;
    SECItem signedItem;
    CERTPublicKeyAndChallenge pkac;
    char *keystring = NULL;
    SECStatus ret = SECFailure;
    CK_MECHANISM_TYPE mechanism;
    PK11RSAGenParams rsaParams;
    void *params;
    SECOidTag algTag;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto done;
    }

    /*
     * Create a new key pair.
     */
    /* Switch statement goes here for DSA */
    switch (type) {
      case rsaKey:
	rsaParams.keySizeInBits = keysize;
	rsaParams.pe = 65537L;
	mechanism = CKM_RSA_PKCS_KEY_PAIR_GEN;
	algTag = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
	params = &rsaParams;
	break;
      case dsaKey:
	params = pqg;
	mechanism = CKM_DSA_KEY_PAIR_GEN;
	algTag = SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;
	break;
      default:
	break;
    }
    privateKey = PK11_GenerateKeyPair(slot, mechanism, params,
				     &publicKey, PR_TRUE, PR_TRUE, proto_win);
    if (privateKey == NULL) {
	goto done;
    }
    
    /*
     * Create a subject public key info from the public key.
     */
    spkInfo = SECKEY_CreateSubjectPublicKeyInfo(publicKey);
    if ( spkInfo == NULL ) {
	goto done;
    }
    
    /*
     * Now DER encode the whole subjectPublicKeyInfo.
     */
    rv=DER_Encode(arena, &spkiItem, CERTSubjectPublicKeyInfoTemplate, spkInfo);
    if (rv != SECSuccess) {
	goto done;
    }

    /*
     * set up the PublicKeyAndChallenge data structure, then DER encode it
     */
    pkac.spki = spkiItem;
    if ( challenge ) {
	pkac.challenge.len = PORT_Strlen(challenge);
    } else {
	pkac.challenge.len = 0;
    }
    pkac.challenge.data = (unsigned char *)challenge;
    
    rv = DER_Encode(arena, &pkacItem, CERTPublicKeyAndChallengeTemplate, &pkac);
    if ( rv != SECSuccess ) {
	goto done;
    }

    /*
     * now sign the DER encoded PublicKeyAndChallenge
     */
    rv = SEC_DerSignData(arena, &signedItem, pkacItem.data, pkacItem.len,
			 privateKey, algTag);
    if ( rv != SECSuccess ) {
	goto done;
    }
    
    /*
     * Convert the signed public key and challenge into base64/ascii.
     */
    keystring = BTOA_DataToAscii(signedItem.data, signedItem.len);

    /* key is in DB now, so post form */
    *(formState->pDone) = PR_TRUE;
    *(formState->pValue) = keystring;
    ret = SECSuccess;
done:
    
    /*
     * Destroy the private key ASAP, so that we don't leave a copy in
     * memory that some evil hacker can get at.
     */
    if ( privateKey ) {
	if (ret != SECSuccess) {
	    PK11_DestroyTokenObject
				(privateKey->pkcs11Slot,privateKey->pkcs11ID);
	}
	SECKEY_DestroyPrivateKey(privateKey);
    }
    
    if ( spkInfo ) {
	SECKEY_DestroySubjectPublicKeyInfo(spkInfo);
    }
    if ( publicKey ) {
	SECKEY_DestroyPublicKey(publicKey);
    }
    
    if ( arena ) {
	PORT_FreeArena(arena, PR_TRUE);
    }
    
    return ret;
}

static SECStatus
PostKey(void *proto_win, KeyGenFormState *formState)
{
    FE_SubmitInputElement(proto_win, formState->form);
    if (formState) {
    	PORT_Free(formState);
    }
    return SECSuccess;
}

/* callback from SECNAV_MakePWSetupDialog() that stores the private key
 * in the database, and posts the KEYGEN form data
 */
static SECStatus
KeyGenPWCallback(void *arg, PRBool cancel)
{
    SECStatus rv = SECFailure;
    KeyGenDialogState *mystate;
    
    mystate = (KeyGenDialogState *) arg;
    if ( mystate == NULL ) {
	goto done;
    }

    if ( cancel ) {
	if (mystate->slot) PK11_FreeSlot(mystate->slot);
	/* free formState */
	if (mystate->challenge) PORT_Free(mystate->challenge);
	if (mystate->pqg) PQG_DestroyParams(mystate->pqg);
	PORT_Free(mystate);
	return SECSuccess;
    }
   
    rv = GenKey(mystate->keysize, mystate->challenge, mystate->type,
		mystate->pqg, mystate->win, mystate->formState,
		mystate->slot);
    if (rv != SECSuccess) {
	FE_Alert((MWContext *)mystate->win,
		 XP_GetString(SEC_ERROR_KEYGEN_FAIL));
    }

done:
    return(rv);
}

static SECStatus
PostKeyPWCallback(void *arg, PRBool cancel)
{
    SECStatus rv = SECFailure;
    KeyGenDialogState *mystate;
    
    mystate = (KeyGenDialogState *) arg;
    if ( mystate == NULL ) {
	goto done;
    }

    if ( cancel ) {
	goto done;
    }

    rv = PostKey(mystate->win, mystate->formState);
    if (rv != SECSuccess) {
	FE_Alert((MWContext *)mystate->win,
		 XP_GetString(SEC_ERROR_KEYGEN_FAIL));
    }

done:
    if (mystate != NULL) {
	if (mystate->slot) PK11_FreeSlot(mystate->slot);
	/* free formState */
	if (mystate->challenge) PORT_Free(mystate->challenge);
	if (mystate->pqg) PQG_DestroyParams(mystate->pqg);
	PORT_Free(mystate);
    }
    return(rv);
}

static void
do_keygen_pw_cb(void *closure)
{
    KeyGenDialogState *mystate = (KeyGenDialogState *)closure;

    SECNAV_MakePWSetupDialog(mystate->win, KeyGenPWCallback,
			     PostKeyPWCallback,
			     mystate->slot, (void *)mystate);
    return;
}

static void
do_post_key_cb(void *closure)
{
    KeyGenDialogState *mystate = (KeyGenDialogState *)closure;

    PostKeyPWCallback(mystate, PR_FALSE);
    return;
}

static PRBool
keyGenDialogHandler(XPDialogState *state, char **argv, int argc,
		    unsigned int button)
{
    KeyGenDialogState *mystate;
    SECStatus rv;
    char *buttonString;

    mystate = (KeyGenDialogState *)state->arg;
    mystate->slot = NULL;

    if (button == XP_DIALOG_CANCEL_BUTTON) {
	goto done;
    }

    if (button == XP_DIALOG_MOREINFO_BUTTON) {
	XPDialogStrings *strings;

	strings = XP_GetDialogStrings(XP_KEY_GEN_MOREINFO_STRINGS);
	if (strings) {
	    XP_MakeHTMLDialog(state->window, &infoDialog,
			      XP_KEY_GEN_MOREINFO_TITLE_STRING,
			      strings, 0);
	    XP_FreeDialogStrings(strings);
	    return PR_TRUE;
	}
    }

    if (button == XP_DIALOG_OK_BUTTON) {
	/* pick slot */
	char *tokenName = XP_FindValueInArgs("tokenName", argv, argc);
	PK11SlotListElement *le;

	if (tokenName) {	
	    for (le = mystate->slotList->head; le; le = le->next) {
		char *thisToken = PK11_GetTokenName(le->slot);
		if (PORT_Strcmp(tokenName,thisToken) == 0) {
		    break;
		}
	    }
	    mystate->slot = PK11_ReferenceSlot(le->slot);
	} else {
	    mystate->slot = PK11_ReferenceSlot(mystate->slotList->head->slot);
	}

	PK11_FreeSlotList(mystate->slotList);
	mystate->slotList = NULL;

	if (PK11_NeedLogin(mystate->slot) && PK11_NeedUserInit(mystate->slot)) {
	    /* dialog to initialize PIN */
	    mystate->win = state->proto_win;
	    state->deleteCallback = do_keygen_pw_cb;
	    state->cbarg = (void *)mystate;
	    return PR_FALSE;
	} 

	if (PK11_NeedLogin(mystate->slot) && !PK11_IsLoggedIn(mystate->slot)) {
	    rv = PK11_DoPassword(mystate->slot,state->window);
	}

	rv = GenKey(mystate->keysize, mystate->challenge, mystate->type,
		    mystate->pqg, state->proto_win, mystate->formState,
		    mystate->slot);
	if (rv != SECSuccess) {
	    FE_Alert((MWContext *)state->proto_win,
		     XP_GetString(SEC_ERROR_KEYGEN_FAIL));
	} else {
	    state->deleteCallback = do_post_key_cb;
	    state->cbarg = (void *)mystate;
	    return PR_FALSE;
	}
    }
done:
    if (mystate->slotList) PK11_FreeSlotList(mystate->slotList);
    if (mystate->slot) PK11_FreeSlot(mystate->slot);
    PORT_Free(mystate->challenge);
    if (mystate->pqg) PQG_DestroyParams(mystate->pqg);
    PORT_Free(mystate);

    return(PR_FALSE);
}

static XPDialogInfo keyGenDialog = {
    XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_OK_BUTTON | XP_DIALOG_MOREINFO_BUTTON,
    keyGenDialogHandler,
    500,
    300
};

SECStatus
SEC_MakeKeyGenDialog(void *proto_win, LO_Element *form, int keysize,
		     char *challenge, KeyType type, PQGParams *pqg,
		     char **pValue, PRBool *pDone)
{
    XPDialogStrings *strings = NULL;
    KeyGenDialogState *mystate = NULL;
    KeyGenFormState *formState = NULL;
    
    mystate = (KeyGenDialogState *)PORT_Alloc(sizeof(KeyGenDialogState));
    if ( mystate == NULL ) { goto fail; } 
    mystate->challenge = NULL;
    mystate->slotList = NULL;
    formState = (KeyGenFormState *)PORT_Alloc(sizeof(KeyGenFormState));
    if ( formState == NULL ) { goto fail; }

    mystate->formState = formState;
    mystate->keysize = keysize;
    if ( challenge ) {
	mystate->challenge = PORT_Strdup(challenge);
    } else {
	mystate->challenge = PORT_Strdup("");
    }
    mystate->type = type;
    mystate->pqg = pqg;
    mystate->win = proto_win;
    
    if ( mystate->challenge == NULL ) { goto fail; }

    /* OK, now go look up the tokens we can use */
    /* NOTE: When We add DSA Key Gen tags we need to handle it here as
     * well */
    switch (type) {
      case rsaKey:
        mystate->slotList = PK11_GetAllTokens(CKM_RSA_PKCS,
				PR_TRUE /* Need R/W tokens for key Gen */);
	break;
      case dsaKey:
        mystate->slotList = PK11_GetAllTokens(CKM_DSA,
				PR_TRUE /* Need R/W tokens for key Gen */);
	break;
      default:
	mystate->slotList == NULL;
	break;
    }

    /* make sure we got at least one */
    if (mystate->slotList == NULL && mystate->slotList->head == NULL) {
	PORT_SetError(SEC_ERROR_NO_MODULE);
	goto fail;
    }
    
    formState->pValue = pValue;
    formState->pDone = pDone;
    formState->form = form;
    
    strings = XP_GetDialogStrings(XP_KEY_GEN_DIALOG_STRINGS);
    if ( strings == NULL ) { goto fail; }

    /* this magic checks to see if there are more than one slot */
    if (mystate->slotList->head->next != NULL) {
	PK11SlotListElement *le;
	char *buf, *bp;
	int count = 0;
	char option[] = { '<','O','P','T','I','O','N','>' };

	XP_CopyDialogString(strings, 0,XP_GetString(XP_KEY_GEN_TOKEN_SELECT));
	for (le = mystate->slotList->head; le; le = le->next) count++;
	buf  = PORT_Alloc((sizeof(option) + 32)*count + 1);
	if (buf == NULL) goto fail;
	bp = buf;
	for (le = mystate->slotList->head; le; le = le->next) {
	    char *token_name = PK11_GetTokenName(le->slot);
	    int len = PORT_Strlen(token_name);
	    PORT_Memcpy(bp,option,sizeof(option));
	    bp += sizeof(option);
	    PORT_Memcpy(bp,token_name,len);
	    bp += len;
	}
	*bp = 0;
	XP_CopyDialogString(strings,1,buf);
	PORT_Free(buf);
	XP_CopyDialogString(strings,2,"</SELECT>");
    } else {
	XP_SetDialogString(strings,0,"");
	XP_SetDialogString(strings,1,"");
	XP_SetDialogString(strings,2,"");
    }
    XP_MakeHTMLDialog(proto_win, &keyGenDialog,
			  XP_GENKEY_TITLE_STRING, strings, (void *)mystate);
    XP_FreeDialogStrings(strings);
    return(SECSuccess);
fail:
    if (strings) XP_FreeDialogStrings(strings);
    if (mystate) {
	if (mystate->slotList) PK11_FreeSlotList(mystate->slotList);
	if (mystate->challenge) PORT_Free(mystate->challenge);
	PORT_Free(mystate);
    }
    if (formState) PORT_Free(formState);
    return SECFailure;
}

void
SECNAV_DisplayCertDialog(MWContext *cx, CERTCertificate *cert)
{
    char *certstr;
    XPDialogStrings *strings;
    
    certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

    strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
    if ( strings ) {
	XP_SetDialogString(strings, 0, certstr);
	XP_MakeHTMLDialog((void *)cx, &certDialog,
			  XP_VIEWCERT_TITLE_STRING, strings, 0);
	XP_FreeDialogStrings(strings);
    }
    PORT_Free(certstr);

    return;
}

typedef struct {
    CERTCertificate *cert;
    CERTDERCerts *derCerts;
} EmailCertDownloadDialogState;

static PRBool
emailCertDownloadDialogHandler(XPDialogState *state, char **argv, int argc,
			       unsigned int button)
{
    XPDialogStrings *strings;
    CERTCertificate *cert;
    EmailCertDownloadDialogState *mystate;
    SECStatus rv;
    
    char *certstr;
    
    mystate = (EmailCertDownloadDialogState *)state->arg;

    cert = mystate->cert;

    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(state->window, &certDialog,
			      XP_VIEWCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	return(PR_TRUE);
    }

    if ( button == XP_DIALOG_OK_BUTTON ) {
	SECItem **rawCerts;
	int numcerts;
	int i;
	
	numcerts = mystate->derCerts->numcerts;
    
	rawCerts = (SECItem **) PORT_Alloc(sizeof(SECItem *) * numcerts);
	if ( rawCerts ) {
	    for ( i = 0; i < numcerts; i++ ) {
		rawCerts[i] = &mystate->derCerts->rawCerts[i];
	    }
	    
	    rv = CERT_ImportCerts(cert->dbhandle, certUsageEmailSigner,
				  numcerts, rawCerts, NULL, PR_TRUE, PR_FALSE,
				  NULL);
	    if ( rv == SECSuccess ) {
		rv = CERT_SaveSMimeProfile(cert, NULL, NULL);
	    }
	    
	    PORT_Free(rawCerts);
	}
    }
    
    CERT_DestroyCertificate(cert);
    PORT_Free(mystate);

    if ( mystate->derCerts ) {
	PORT_FreeArena(mystate->derCerts->arena, PR_FALSE);
    }

    return(PR_FALSE);
}

static XPDialogInfo emailCertDownloadDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    emailCertDownloadDialogHandler,
    550,
    350
};

void
SECNAV_MakeEmailCertDownloadDialog(void *proto_win, CERTCertificate *cert,
				   CERTDERCerts *derCerts)
{
    XPDialogStrings *strings = NULL;
    EmailCertDownloadDialogState *mystate;
    char *str;
    
    if ( cert->emailAddr == NULL ) {
	goto loser;
    }
    
    strings = XP_GetDialogStrings(XP_DOWNLOAD_EMAIL_CERT_DIALOG_STRINGS);
    
    if ( strings != NULL ) {

	str = CERT_GetCommonName(&cert->subject);
	if ( str != NULL ) {
	    XP_CopyDialogString(strings, 0, str);
	    PORT_Free(str);
	} else {
	    XP_CopyDialogString(strings, 0, " ");
	}

	XP_CopyDialogString(strings, 1, cert->emailAddr);
	
	str = CERT_GetOrgName(&cert->issuer);
	if ( str != NULL ) {
	    XP_CopyDialogString(strings, 2, str);
	    PORT_Free(str);
	} else {
	    str = CERT_GetCommonName(&cert->issuer);
	    if ( str != NULL ) {
		XP_CopyDialogString(strings, 2, str);
		PORT_Free(str);
	    } else {
		XP_CopyDialogString(strings, 2, " ");
	    }
	}

	mystate = (EmailCertDownloadDialogState *)
	    PORT_Alloc(sizeof(EmailCertDownloadDialogState));
	if ( mystate == NULL ) {
	    goto loser;
	}

	mystate->cert = cert;
	mystate->derCerts = derCerts;
	
	XP_MakeHTMLDialog(proto_win, &emailCertDownloadDialog,
			  XP_EMAIL_CERT_DOWNLOAD_TITLE_STRING,
			  strings, (void *)mystate);
	
	XP_FreeDialogStrings(strings);

	return;
    }

loser:    
    CERT_DestroyCertificate(cert);
    if ( derCerts ) {
	PORT_FreeArena(derCerts->arena, PR_FALSE);
    }
    if ( strings ) {
	XP_FreeDialogStrings(strings);
    }

    return;
}

/*
 * This function is called by all of the dialogs that select a cert by
 * nickname/email addr and then perform operations on them.  If there is
 * only one cert with the correct subject name, then just call the callback
 * function (which will probably put up a dialog and perform some operation).
 * If there are multiple certs, then this function will put up a dialog
 * asking the user to select one operate on.
 */

typedef struct {
    char *str;
    int count;
    CERTCertificate **certs;
    CERTCertificate *cert;
    CERTSelectCallback callback;
    void *cbarg;
    void *proto_win;
} collectSubjectCertsState;

static SECStatus
collectSubjectCerts(CERTCertificate *cert, void *arg)
{
    collectSubjectCertsState *state;
    char *notBefore;
    char *notAfter;
    
    if ( ! cert->isperm ) {
	return(SECSuccess);
    }
    
    state = (collectSubjectCertsState *)arg;
    
    notBefore = DER_UTCDayToAscii(&cert->validity.notBefore);
    notAfter = DER_UTCDayToAscii(&cert->validity.notAfter);

    state->str =
	PR_sprintf_append(state->str,
			  "<input type=radio name=cert value=cert%d %s>Valid from %s to %s<br>",
			  state->count, (state->count == 0) ? "checked" : "",
			  notBefore, notAfter);

    PORT_Free(notBefore);
    PORT_Free(notAfter);
    
    state->certs[state->count] = CERT_DupCertificate(cert);
    
    state->count++;
    
    return(SECSuccess);
}

static void
doSelectCallback(void *closure)
{
    collectSubjectCertsState *mystate;
    
    mystate = (collectSubjectCertsState *)closure;
    
    (* mystate->callback)(mystate->cert, mystate->proto_win,
			  mystate->cbarg);

    PORT_Free(mystate);

    return;
}

static PRBool
certSelectDialogHandler(XPDialogState *state, char **argv, int argc,
			unsigned int button)
{
    collectSubjectCertsState *mystate;
    char *certstring;
    int certoff;
    int i;
    PRBool usedTimeout = PR_FALSE;
    
    mystate = (collectSubjectCertsState *)state->arg;
    
    if ( button == XP_DIALOG_CONTINUE_BUTTON ) {
	certstring = XP_FindValueInArgs("cert", argv, argc);
	if ( certstring == NULL ) {
	    goto done;
	}
	certoff = PORT_Atoi(&certstring[4]);

	mystate->cert = CERT_DupCertificate(mystate->certs[certoff]);

#ifndef AKBAR
	state->deleteCallback = doSelectCallback;
	state->cbarg = (void *)mystate;
#endif /* !AKBAR */
	
	usedTimeout = PR_TRUE;
    }
    
done:
    for ( i = 0; i < mystate->count; i++ ) {
	CERT_DestroyCertificate(mystate->certs[i]);
    }

    if ( !usedTimeout ) {
	PORT_Free(mystate);
    }
    
    return(PR_FALSE);
}

static XPDialogInfo certSelectDialog = {
    XP_DIALOG_CONTINUE_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    certSelectDialogHandler,
    640,
    400
};

SECStatus
SECNAV_SelectCert(CERTCertDBHandle *handle, char *name, int messagestr,
		  void *proto_win, CERTSelectCallback callback,
		  void *cbarg)
{
    CERTCertificate *firstcert= NULL, *cert = NULL;
    int ncerts;
    XPDialogStrings *strings = NULL;
    collectSubjectCertsState *state = NULL;
    SECStatus rv;

    firstcert = CERT_FindCertByNicknameOrEmailAddr(handle, name);
    if (firstcert == NULL) {
    	firstcert = PK11_FindCertFromNickname(name, proto_win);
    } 

    if (firstcert == NULL) {
	goto loser;
    }
    ncerts = CERT_NumPermCertsForCertSubject(firstcert);
    ncerts += PK11_NumberCertsForCertSubject(firstcert);
    
    if ( ncerts > 1 ) {

	state = (collectSubjectCertsState *)
	    PORT_Alloc(sizeof(collectSubjectCertsState));

	if ( state == NULL ) {
	    goto loser;
	}
	
	state->str = NULL;
	state->count = 0;
	state->callback = callback;
	state->cbarg = cbarg;
	state->proto_win = proto_win;
	state->cert = NULL;
	state->certs = (CERTCertificate **)PORT_Alloc(sizeof(CERTCertificate *)
						      * ncerts);
	
	if ( state->certs == NULL ) {
	    goto loser;
	}
	
	rv = CERT_TraverseCertsForSubject(handle, firstcert->subjectList,
					  collectSubjectCerts,
					  (void *)state);
        if (rv == SECSuccess) {
	    rv = PK11_TraverseCertsForSubject(firstcert,collectSubjectCerts,
						(void *)state);
	}

	state->count = ncerts;
	
	if ( rv != SECSuccess ) {
	    goto loser;
	}
	
	strings = XP_GetDialogStrings(XP_CERT_MULTI_SUBJECT_SELECT_STRING);
	if ( strings == NULL ) {
	    goto loser;
	}
	
	XP_CopyDialogString(strings, 0, XP_GetString(messagestr));
	XP_CopyDialogString(strings, 1, state->str);
	
	PORT_Free(state->str);
	state->str = NULL;
	
	XP_MakeHTMLDialog(proto_win, &certSelectDialog,
			  XP_CERT_MULTI_SUBJECT_SELECT_TITLE_STRING,
			  strings, (void *)state);
	XP_FreeDialogStrings(strings);
	rv = SECSuccess;
    } else {
	cert = firstcert;
	(* callback)(cert, proto_win, cbarg);
    }

    return(SECSuccess);
    
loser:
    return(SECFailure);
}

static XPDialogInfo verifyCertDialog = {
    XP_DIALOG_OK_BUTTON,
    0,
    600,
    400
};

void
SECNAV_VerifyCertificate(CERTCertificate *cert, void *proto_win, void *arg)
{
    SECStatus rv;
    CERTVerifyLog log;
    int error;
    CERTVerifyLogNode *node;
    char *name;
    CERTCertificate *errcert = NULL;
    int depth = -1;
    char *messagestr = NULL;
    char *errstr;
    SECCertUsage certUsage;
    XPDialogStrings *strings = NULL;
    PRBool freestr;
    char *tmpstr;
    unsigned int flags;
    
    certUsage = (SECCertUsage)arg;
    
    log.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( log.arena == NULL ) {
	goto done;
    }
    log.head = NULL;
    log.tail = NULL;
    log.count = 0;
    
    rv = CERT_VerifyCert(cert->dbhandle, cert, PR_TRUE,
			 certUsage, PR_Now(), proto_win, &log);
    
    if ( log.count > 0 ) {
	node = log.head;
	messagestr = PR_sprintf_append(NULL, "<dl>");
	while ( node ) {
	    
	    if ( depth != node->depth ) {
		depth = node->depth;
		/* draw a header */
		errcert = node->cert;
		if ( errcert->nickname != NULL ) {
		    name = errcert->nickname;
		} else if ( errcert->emailAddr != NULL ) {
		    name = errcert->emailAddr;
		} else {
		    name = errcert->subjectName;
		}

		messagestr = PR_sprintf_append(messagestr,
					       "<dt><b>%s</b>", name);
		if ( depth != 0 ) {
		    messagestr = PR_sprintf_append(messagestr,
					    "<b>[Certificate Authority]</b>");
		}
		
		PORT_Assert(name);
	    }

	    error = node->error;

	    freestr = PR_FALSE;
	    
	    if ( error == SEC_ERROR_EXPIRED_CERTIFICATE ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_EXPIRED);
	    } else if ( error == SEC_ERROR_INADEQUATE_KEY_USAGE ) {
		tmpstr = PORT_Strdup(XP_GetString(XP_VERIFY_ERROR_NOT_CERTIFIED));
		if ( tmpstr == NULL ) {
		    goto done;
		}
		
		flags = (unsigned int)node->arg;
		switch(flags) {
		  case KU_DIGITAL_SIGNATURE:
		    errstr = XP_GetString(XP_VERIFY_ERROR_SIGNING);
		    break;
		  case KU_KEY_ENCIPHERMENT:
		    errstr = XP_GetString(XP_VERIFY_ERROR_ENCRYPTION);
		    break;
		  case KU_KEY_CERT_SIGN:
		    errstr = XP_GetString(XP_VERIFY_ERROR_CERT_SIGNING);
		    break;
		  default:
		    errstr = XP_GetString(XP_VERIFY_ERROR_CERT_UNKNOWN_USAGE);
		    break;
		}
		errstr = PR_smprintf(tmpstr, errstr);
		if ( errstr == NULL ) {
		    goto done;
		}
		PORT_Free(tmpstr);
		freestr = PR_TRUE;
	    } else if ( error == SEC_ERROR_INADEQUATE_CERT_TYPE ) {
		tmpstr = PORT_Strdup(XP_GetString(XP_VERIFY_ERROR_NOT_CERTIFIED));
		if ( tmpstr == NULL ) {
		    goto done;
		}
		
		flags = (unsigned int)node->arg;
		switch(flags) {
		  case NS_CERT_TYPE_SSL_CLIENT:
		  case NS_CERT_TYPE_SSL_SERVER:
		    errstr = XP_GetString(XP_VERIFY_ERROR_SSL);
		    break;
		  case NS_CERT_TYPE_SSL_CA:
		    errstr = XP_GetString(XP_VERIFY_ERROR_SSL_CA);
		    break;
		  case NS_CERT_TYPE_EMAIL:
		    errstr = XP_GetString(XP_VERIFY_ERROR_EMAIL);
		    break;
		  case NS_CERT_TYPE_EMAIL_CA:
		    errstr = XP_GetString(XP_VERIFY_ERROR_EMAIL_CA);
		    break;
		  case NS_CERT_TYPE_OBJECT_SIGNING:
		    errstr = XP_GetString(XP_VERIFY_ERROR_OBJECT_SIGNING);
		    break;
		  case NS_CERT_TYPE_OBJECT_SIGNING_CA:
		    errstr = XP_GetString(XP_VERIFY_ERROR_OBJECT_SIGNING_CA);
		    break;
		  default:
		    errstr = XP_GetString(XP_VERIFY_ERROR_CERT_UNKNOWN_USAGE);
		    break;
		}
		errstr = PR_smprintf(tmpstr, errstr);
		if ( errstr == NULL ) {
		    goto done;
		}
		PORT_Free(tmpstr);
		freestr = PR_TRUE;
	    } else if ( error == SEC_ERROR_UNTRUSTED_CERT ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_NOT_TRUSTED);
		/* ssl flags - only ssl */
	    } else if ( error == SEC_ERROR_UNKNOWN_ISSUER ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_NO_CA);
	    } else if ( error == SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_EXPIRED);
	    } else if ( error == SEC_ERROR_BAD_SIGNATURE ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_BAD_SIG);
	    } else if ( error == SEC_ERROR_CRL_BAD_SIGNATURE ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_BAD_CRL);
	    } else if ( error == SEC_ERROR_CRL_EXPIRED ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_BAD_CRL);
	    } else if ( error == SEC_ERROR_REVOKED_CERTIFICATE ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_REVOKED);
	    } else if ( error == SEC_ERROR_CA_CERT_INVALID ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_NOT_CA);
	    } else if ( error == SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID ) {
		/* basicConstraint.pathLenConstraint */
		errstr = XP_GetString(XP_VERIFY_ERROR_NOT_CA);
	    } else if ( error == SEC_ERROR_UNTRUSTED_ISSUER ) {
		errstr = XP_GetString(XP_VERIFY_ERROR_NOT_TRUSTED);
	    } else {
		errstr = XP_GetString(XP_VERIFY_ERROR_INTERNAL_ERROR);
	    }

	    messagestr = PR_sprintf_append(messagestr, "<dd>%s", errstr);
	    if ( freestr ) {
		PORT_Free(errstr);
		freestr = PR_FALSE;
	    }
	    
	    CERT_DestroyCertificate(node->cert);

	    node = node->next;
	}
	messagestr = PR_sprintf_append(messagestr, "</dl>");

	if ( messagestr != NULL ) {
	    strings = XP_GetDialogStrings(XP_VERIFY_CERT_DIALOG_STRINGS);
	    if ( strings ) {
		XP_SetDialogString(strings, 0, messagestr);
		XP_MakeHTMLDialog((void *)proto_win, &verifyCertDialog,
				  XP_VERIFYCERT_TITLE_STRING, strings, 0);
		XP_FreeDialogStrings(strings);
	    }
	    PORT_Free(messagestr);
	}
    } else {
	/* tell the user cert is OK */
	strings = XP_GetDialogStrings(XP_VERIFY_CERT_OK_DIALOG_STRING);
	if ( strings ) {
	    XP_MakeHTMLDialog((void *)proto_win, &verifyCertDialog,
			      XP_VERIFYCERT_TITLE_STRING, strings, 0);
	    XP_FreeDialogStrings(strings);
	}
    }
    
done:
    if ( log.arena != NULL ) {
	PORT_FreeArena(log.arena, PR_FALSE);
    }
    
    CERT_DestroyCertificate(cert);
    return;
}
