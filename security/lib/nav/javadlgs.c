/*
 * Dialogs for java applet's privileges management
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: javadlgs.c,v 1.2.2.7 1997/05/03 09:25:07 jwz Exp $
 */
#include "xp.h"
#include "xpgetstr.h"
#include "key.h"
#include "secitem.h"
#include "net.h"
#include "secnav.h"
#include "htmldlgs.h"
#include "fe_proto.h"
#include "cert.h"

#ifdef JAVA
#include "javasec.h"
#endif

extern int XP_SEC_MOREINFO;
extern int XP_SEC_DELETE;
extern int XP_JAVA_SECURITY_TITLE_STRING;
extern int XP_DELETE_JAVA_PRIV_TITLE_STRING;
extern int XP_EDIT_JAVA_PRIV_TITLE_STRING;
extern int XP_DELETE_JAVA_PRIN_STRINGS;
extern int XP_EDIT_JAVA_PRIVILEGES_STRINGS;
extern int XP_EDIT_JAVA_PRIV_ALWAYS_STRINGS;
extern int XP_EDIT_JAVA_PRIV_SESSION_STRINGS;
extern int XP_EDIT_JAVA_PRIV_NEVER_STRINGS;
extern int XP_DELETE_JAVA_PRIV_STRINGS;
extern int XP_MOREINFO_JAVA_PRIV_STRINGS;
extern int XP_SIGNED_CERT_PRIV_STRINGS;
extern int XP_SIGNED_APPLET_PRIV_STRINGS;
extern int XP_SEC_SHOWCERT;

extern int XP_JAVA_CERT_NOT_EXISTS_ERROR;
extern int XP_JAVA_REMOVE_PRINCIPAL_ERROR;
extern int XP_JAVA_DELETE_PRIVILEGE_ERROR;
extern int XP_CERT_SELECT_VIEW_STRING;

void SECNAV_DeleteJavaPrivilege(void *proto_win, char *javaPrin, 
				char *javaTarget, SECNAVJavaPrinCallback f,
				void *arg);

SECStatus
SECNAV_IsJavaPrinCert(void * proto_win, char * certname)
{
    CERTCertificate *cert = NULL;
    int ret;
    
    cert = CERT_FindCertByNicknameOrEmailAddr(CERT_GetDefaultCertDB(),
					      certname);
    if ( cert == NULL) {
	FE_Alert(proto_win,
		 XP_GetString(XP_JAVA_CERT_NOT_EXISTS_ERROR));
	return(SECFailure);
    }

    return(SECSuccess);
}



typedef struct {
    char *javaPrin;
    void *arg;
    SECNAVJavaPrinCallback f;
    void *proto_win;
} deleteJavaPrinState;

static PRBool
deleteJavaPrinHandler(XPDialogState *state, char **argv, int argc,
		      unsigned int button)
{
    char *javaPrin;
    deleteJavaPrinState *delstate;

    delstate = (deleteJavaPrinState *)state->arg;

    javaPrin = delstate->javaPrin;

    if ( button == XP_DIALOG_OK_BUTTON ) {
#ifndef AKBAR
#ifdef JAVA
	if (!java_netscape_security_removePrincipal(javaPrin)) {
	    /* We couldn't remove the principal */
    	    FE_Alert(delstate->proto_win,
		     XP_GetString(XP_JAVA_REMOVE_PRINCIPAL_ERROR));
	}
#endif
#endif /* !AKBAR */
	if ( delstate->f ) {
	    (* delstate->f)(delstate->arg);
	}
    }
    
    PORT_Free(delstate->javaPrin);
    PORT_Free(delstate);
    
    return(PR_FALSE);
}


static XPDialogInfo deleteJavaPrinDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    deleteJavaPrinHandler,
    600,
    400
};

void
SECNAV_DeleteJavaPrincipal(void *proto_win, char *javaPrin,
			   SECNAVJavaPrinCallback f,
			   void *arg)
{
    XPDialogStrings *strings;
    deleteJavaPrinState *delstate;
    
    delstate = NULL;
    strings = NULL;
    
    delstate = (deleteJavaPrinState *)PORT_Alloc(sizeof(deleteJavaPrinState));
    if ( delstate == NULL ) {
	goto loser;
    }

    delstate->javaPrin = PORT_Strdup(javaPrin);
    delstate->f = f;
    delstate->arg = arg;
    delstate->proto_win = proto_win;

    strings = XP_GetDialogStrings(XP_DELETE_JAVA_PRIN_STRINGS);
    
    if ( strings == NULL ) {
	goto loser;
    }
    
    /* fill in dialog contents */
    XP_CopyDialogString(strings, 0, javaPrin);
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &deleteJavaPrinDialog,
		      XP_JAVA_SECURITY_TITLE_STRING,
		      strings, (void *)delstate);

    goto done;
loser:
    /* if we didn't make the dialog then free the state */
    if ( delstate ) {
	PORT_Free(delstate->javaPrin);
	PORT_Free(delstate);
    }

done:

    if ( strings ) {
	/* free any allocated strings */
	XP_FreeDialogStrings(strings);
    }
    
    return;
}

typedef struct {
    char *javaPrin;
    char *javaTarget;
    void *arg;
    SECNAVJavaPrinCallback f;
    void *proto_win;
} deleteJavaPrivilegeState;

static PRBool
deleteJavaPrivilegeHandler(XPDialogState *state, char **argv, int argc,
			   unsigned int button)
{
    char *javaPrin;
    char *javaTarget;
    deleteJavaPrivilegeState *delstate;

    delstate = (deleteJavaPrivilegeState *)state->arg;

    javaPrin = delstate->javaPrin;
    javaTarget = delstate->javaTarget;

    if ( button == XP_DIALOG_OK_BUTTON ) {
#ifndef AKBAR
#ifdef JAVA
	if (!java_netscape_security_removePrivilege(javaPrin, javaTarget)) {
    	    FE_Alert(delstate->proto_win,
		     XP_GetString(XP_JAVA_DELETE_PRIVILEGE_ERROR));
	}
#endif
#endif /* !AKBAR */
	if ( delstate->f ) {
	    (* delstate->f)(delstate->arg);
	}
    }
    
    PORT_Free(delstate->javaTarget);
    PORT_Free(delstate->javaPrin);
    PORT_Free(delstate);
    
    return(PR_FALSE);
}


static XPDialogInfo deleteJavaPrivilegeDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    deleteJavaPrivilegeHandler,
    600,
    400
};

void
SECNAV_DeleteJavaPrivilege(void *proto_win, char *javaPrin, char *javaTarget,
			   SECNAVJavaPrinCallback f,
			   void *arg)
{
    XPDialogStrings *strings;
    deleteJavaPrivilegeState *delstate;
    
    delstate = NULL;
    strings = NULL;
    
    delstate = (deleteJavaPrivilegeState *)
			PORT_Alloc(sizeof(deleteJavaPrivilegeState));
    if ( delstate == NULL ) {
	goto loser;
    }

    delstate->javaPrin = PORT_Strdup(javaPrin);
    delstate->javaTarget = PORT_Strdup(javaTarget);
    delstate->f = f;
    delstate->arg = arg;
    delstate->proto_win = proto_win;

    strings = XP_GetDialogStrings(XP_DELETE_JAVA_PRIV_STRINGS);
    
    if ( strings == NULL ) {
	goto loser;
    }
    
    /* fill in dialog contents */
    XP_CopyDialogString(strings, 0, javaTarget);
    XP_CopyDialogString(strings, 1, javaPrin);
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &deleteJavaPrivilegeDialog,
		      XP_DELETE_JAVA_PRIV_TITLE_STRING,
		      strings, (void *)delstate);

    goto done;
loser:
    /* if we didn't make the dialog then free the state */
    if ( delstate ) {
	PORT_Free(delstate->javaTarget);
	PORT_Free(delstate->javaPrin);
	PORT_Free(delstate);
    }

done:

    if ( strings ) {
	/* free any allocated strings */
	XP_FreeDialogStrings(strings);
    }
    
    return;
}

static XPDialogInfo showMoreInfoDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    0,
    600,
    400
};


typedef struct {
    char *javaPrin;
    void *arg;
    SECNAVJavaPrinCallback f;
    void *proto_win;
} editJavaPrinState;


static PRBool
editJavaPrivilegesHandler(XPDialogState *state, char **argv, int argc,
			  unsigned int button)
{
    char *buttonString;
    char *javaPrin;
    char *target;
    char *targetDetails=NULL;
    char *risk=NULL;
    editJavaPrinState *editstate;
    XPDialogStrings *strings;


    editstate = (editJavaPrinState *)state->arg;
    javaPrin = editstate->javaPrin;

    if ( button == XP_DIALOG_CANCEL_BUTTON ) {
	goto done;
    }
    
    if ( (button == 0) || (button == XP_DIALOG_MOREINFO_BUTTON) ) {
	buttonString = XP_FindValueInArgs("button", argv, argc);
	if ( buttonString != NULL ) {
	    target = XP_FindValueInArgs("target", argv, argc);

	    if ( !target ) {
		goto done;
            }

	    if ( PORT_Strcasecmp(buttonString,
				 XP_GetString(XP_SEC_MOREINFO)) == 0 ) {
		strings = XP_GetDialogStrings(XP_MOREINFO_JAVA_PRIV_STRINGS);
		if ( strings ) {
#ifndef AKBAR
#ifdef JAVA
		    java_netscape_security_getTargetDetails(target, 
						&targetDetails, &risk);
#endif
#endif /* !AKBAR */

		    XP_CopyDialogString(strings, 0, target);
		    if (risk)
			XP_CopyDialogString(strings, 1, risk);
		    else
			XP_CopyDialogString(strings, 1, "");
		    if (targetDetails)
			XP_CopyDialogString(strings, 2, targetDetails);
		    else
			XP_CopyDialogString(strings, 2, "");

		    XP_MakeHTMLDialog(editstate->proto_win, 
				      &showMoreInfoDialog,
				      XP_JAVA_SECURITY_TITLE_STRING, 
				      strings, 0);
		    XP_FreeDialogStrings(strings);
		}
	    } else if ( PORT_Strcasecmp(buttonString,
					XP_GetString(XP_SEC_DELETE)) == 0 ) {
		SECNAV_DeleteJavaPrivilege(editstate->proto_win,
					   javaPrin,
					   target,
					   NULL,
					   (void *)NULL);
	    } 
	}
    }

done:

    PORT_Free(editstate->javaPrin);
    PORT_Free(editstate);
    return(PR_FALSE);
}


static XPDialogInfo editJavaPrivilegesDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    editJavaPrivilegesHandler,
    600,
    400
};

void
SECNAV_EditJavaPrivileges(void *proto_win, char *javaPrin)
{
    XPDialogStrings *strings;
    editJavaPrinState *editstate;
    char *allowed_forever=NULL;
    char *allowed_session=NULL;
    char *denied=NULL;

    editstate = NULL;
    strings = NULL;
    
    editstate = (editJavaPrinState *)PORT_Alloc(sizeof(editJavaPrinState));
    if ( editstate == NULL ) {
	goto loser;
    }

    editstate->javaPrin = PORT_Strdup(javaPrin);
    editstate->f = NULL;
    editstate->arg = NULL;
    editstate->proto_win = proto_win;

    strings = XP_GetDialogStrings(XP_EDIT_JAVA_PRIVILEGES_STRINGS);
    if ( strings == NULL ) {
	return;
    }
    
    /* fill in dialog contents */
    XP_CopyDialogString(strings, 0, javaPrin);

#ifndef AKBAR
#ifdef JAVA
    java_netscape_security_getPrivilegeDescs(javaPrin, &allowed_forever, 
					     &allowed_session, &denied);
#endif
#endif /* !AKBAR */

    if (allowed_forever != NULL) {
	XP_CopyDialogString(strings, 1, allowed_forever);
    } else {
	XP_CopyDialogString(strings, 1, "");
    }
    if (allowed_session != NULL) {
	XP_CopyDialogString(strings, 2, allowed_session);
    } else {
	XP_CopyDialogString(strings, 2, "");
    }
    if (denied != NULL) {
	XP_CopyDialogString(strings, 3, denied);
    } else {
	XP_CopyDialogString(strings, 3, "");
    }
    
    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &editJavaPrivilegesDialog,
		      XP_EDIT_JAVA_PRIV_TITLE_STRING, strings, 
		      (void *)editstate);

    goto done;

loser:
    /* if we didn't make the dialog then free the state */
    if ( editstate ) {
	PORT_Free(editstate->javaPrin);
	PORT_Free(editstate);
    }

done:

    if ( strings ) {
	/* free any allocated strings */
	XP_FreeDialogStrings(strings);
    }
    
    return;
}


typedef struct {
    char *javaPrin;
    void *arg;
    SECNAVJavaPrinCallback f;
    void *proto_win;
} signedAppletPriv;


static PRBool
signedAppletPrivilegesHandler(XPDialogState *state, char **argv, int argc,
			  unsigned int button)
{
    char *buttonString;
    char *javaPrin;
    char *target;
    char *targetDetails=NULL;
    char *risk=NULL;
    char *perm;
    char *remember;
    signedAppletPriv *signedstate;
    XPDialogStrings *strings;
    int permission = BLANK_SESSION;
    SECStatus rv = PR_TRUE;

    signedstate = (signedAppletPriv *)state->arg;
    javaPrin = signedstate->javaPrin;

    if ( button == XP_DIALOG_CANCEL_BUTTON ) {
	goto done;
    }

    perm = XP_FindValueInArgs("perm", argv, argc);
    remember = XP_FindValueInArgs("remember", argv, argc);
    
    if (perm && (PORT_Strcasecmp(perm, "yes") == 0)) {
	if (remember && (PORT_Strcasecmp(remember, "on") == 0)) {
	    permission = ALLOWED_FOREVER;
	} else {
	    permission = ALLOWED_SESSION;
	}
    } else {
	if (remember && (PORT_Strcasecmp(remember, "on") == 0)) {
	    permission = FORBIDDEN_FOREVER;
	} 
    }

    
    /* Need to implement the callback to grant the prvilege */
    if ( (button == 0) || (button == XP_DIALOG_MOREINFO_BUTTON) ) {
	buttonString = XP_FindValueInArgs("button", argv, argc);
	if ( buttonString != NULL ) {
	    if ( PORT_Strcasecmp(buttonString,
				 XP_GetString(XP_SEC_MOREINFO)) == 0 ) {

		target = XP_FindValueInArgs("target", argv, argc);

		if ( !target ) {
		    goto done;
		}

		strings = XP_GetDialogStrings(XP_MOREINFO_JAVA_PRIV_STRINGS);
		if ( strings ) {
#ifndef AKBAR
#ifdef JAVA
		    java_netscape_security_getTargetDetails(target, 
				&targetDetails, &risk);
#endif
#endif /* !AKBAR */

		    XP_CopyDialogString(strings, 0, target);
		    if (risk)
			XP_CopyDialogString(strings, 1, risk);
		    else
			XP_CopyDialogString(strings, 1, "");
		    if (targetDetails)
		        XP_CopyDialogString(strings, 2, targetDetails);
		    else
			XP_CopyDialogString(strings, 2, "");

		    XP_MakeHTMLDialog(signedstate->proto_win, 
				      &showMoreInfoDialog,
				      XP_JAVA_SECURITY_TITLE_STRING, 
				      strings, 0);
		    XP_FreeDialogStrings(strings);
		}
	    } else if ( PORT_Strcasecmp(buttonString,
			   XP_GetString(XP_SEC_SHOWCERT)) == 0 ) {
			rv = SECNAV_SelectCert(CERT_GetDefaultCertDB(),
					       javaPrin,
					       XP_CERT_SELECT_VIEW_STRING,
					       signedstate->proto_win,
					       SECNAV_ViewUserCertificate,
					       NULL);

	    }
	}
    }

done:

    if ( ( button == XP_DIALOG_CANCEL_BUTTON ) || 
	 ( button == XP_DIALOG_OK_BUTTON ) ) {
#ifndef AKBAR
#ifdef JAVA
	java_netscape_security_savePrivilege(permission);
#endif
#endif /* !AKBAR */
	rv = PR_FALSE;
	PORT_Free(signedstate->javaPrin);
	PORT_Free(signedstate);
    }

    return(rv);
}


static XPDialogInfo signedAppletPrivilegesDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    signedAppletPrivilegesHandler,
    600,
    400
};


void
SECNAV_signedAppletPrivileges(void *proto_win, char *javaPrin, 
			      char *javaTarget, char *risk, int isCert)
{
    XPDialogStrings *strings=NULL;
    signedAppletPriv *signedstate;

    signedstate = (signedAppletPriv *)PORT_Alloc(sizeof(signedAppletPriv));
    if ( signedstate == NULL ) {
	goto loser;
    }

    signedstate->javaPrin = PORT_Strdup(javaPrin);
    signedstate->f = NULL;
    signedstate->arg = NULL;
    signedstate->proto_win = proto_win;

    if (isCert) {
	strings = XP_GetDialogStrings(XP_SIGNED_CERT_PRIV_STRINGS);
    } else {
	strings = XP_GetDialogStrings(XP_SIGNED_APPLET_PRIV_STRINGS);
    }

    if ( strings == NULL ) {
	return;
    }
    
    /* fill in dialog contents */
    XP_CopyDialogString(strings, 0, javaPrin);
    XP_CopyDialogString(strings, 1, risk);
    XP_CopyDialogString(strings, 2, javaTarget);

    if (isCert) {
	XP_CopyDialogString(strings, 3, XP_GetString(XP_SEC_SHOWCERT));
    }

    /* put up the dialog */
    XP_MakeHTMLDialog(proto_win, &signedAppletPrivilegesDialog,
		      XP_JAVA_SECURITY_TITLE_STRING, strings,
		      (void *)signedstate);

    goto done;

loser:
    /* if we didn't make the dialog then free the state */
    if ( signedstate ) {
	PORT_Free(signedstate->javaPrin);
	PORT_Free(signedstate);
    }

done:

    if ( strings ) {
	/* free any allocated strings */
	XP_FreeDialogStrings(strings);
    }
    
    return;
}

