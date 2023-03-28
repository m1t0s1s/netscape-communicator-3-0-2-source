/*
 * Dialogs for password management
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: pwdlgs.c,v 1.35.2.4 1997/05/24 00:23:41 jwz Exp $
 */
#include "xp.h"
#include "key.h"
#include "secitem.h"
#include "net.h"
#include "secnav.h"
#include "htmldlgs.h"
#include "secprefs.h"
#include "fe_proto.h"
#include "pk11func.h"
#include "xpgetstr.h"
#include "prefapi.h"

extern int XP_PW_SETUP_STRINGS;
extern int XP_SETUPNSPW_TITLE_STRING;
extern int XP_CHANGENSPW_TITLE_STRING;
extern int XP_PWERROR_TITLE_STRING;
extern int XP_PW_SETUP_SSO_FAIL_STRINGS;
extern int XP_SSO_GET_PW_STRINGS;
extern int XP_PW_RETRY_SSO_STRINGS;
extern int SEC_ERROR_RETRY_PASSWORD;
extern int SEC_ERROR_INVALID_PASSWORD;
extern int XP_KEY_GEN_MOREINFO_STRINGS;
extern int XP_KEY_GEN_MOREINFO_TITLE_STRING;
extern int XP_PW_MOREINFO_STRINGS;
extern int XP_PW_CHANGE_STRINGS;
extern int XP_PW_CHANGE_FAILURE_STRINGS;
extern int XP_PW_SETUP_REFUSED_STRINGS;
extern int SEC_ERROR_RETRY_OLD_PASSWORD;

extern void secmoz_SetPWEnabled(void *, PRBool enable);
static PRBool pwSetupHandler(XPDialogState *state, char **argv, int argc,
			     unsigned int button);
static PRBool pwSetupSSOHandler(XPDialogState *state, char **argv, int argc,
				unsigned int button);
static PRBool pwChangeHandler(XPDialogState *state, char **argv, int argc,
			      unsigned int button);

static XPDialogInfo failureDialog = {
    XP_DIALOG_OK_BUTTON,
    0,
    550,
    300
};

typedef struct {
    void *proto_win;
    char *sso;
    PK11SlotInfo *slot;
    SECNAVPWSetupCallback cb1;
    SECNAVPWSetupCallback cb2;
    void *cbarg;
    int error;
    int button;
} PWSetupPanelState;

static XPDialogInfo pwSetupDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_MOREINFO_BUTTON,
    pwSetupHandler,
    600,
    400
};

static void
pw_setup_retry_cb(void *closure)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)closure;
    XPDialogStrings *strings;
#ifndef AKBAR
    XPDialogState *state = NULL;
#endif /* !AKBAR */

    strings = XP_GetDialogStrings(XP_PW_SETUP_STRINGS);
    if (strings) {
#ifndef AKBAR
	state = 
#endif /* !AKBAR */
	        XP_MakeHTMLDialog(mystate->proto_win, &pwSetupDialog,
				  XP_SETUPNSPW_TITLE_STRING,
				  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }
    if ((mystate->error != 0)
#ifndef AKBAR
	&& (state != NULL) && (state->window != NULL)
#endif /* !AKBAR */
	) {
	FE_Alert(
#ifndef AKBAR
		 state->window,
#else /* AKBAR */
		 mystate->proto_win,
#endif /* AKBAR */
		 XP_GetString(mystate->error));
    }
}

static XPDialogInfo infoDialog = {
    XP_DIALOG_OK_BUTTON,
    0,
    500,
    300
};

static PRBool
pwSetupNoPWHandler(XPDialogState *state, char **argv, int argc,
		   unsigned int button)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)(state->arg);

    if ( mystate->cb2 ) {
	(* mystate->cb2)(mystate->cbarg, PR_FALSE);
    }

    if (mystate->sso != NULL) {
	/* clear and free the password */
	SECNAV_ZeroPassword(mystate->sso);
	PORT_Free(mystate->sso);
    }

    if (mystate->slot != NULL) {
	PK11_FreeSlot(mystate->slot);
    }
    PORT_Free(mystate);

    return PR_FALSE;
}

static XPDialogInfo setupNoPWDialog = {
    XP_DIALOG_OK_BUTTON,
    pwSetupNoPWHandler,
    500,
    300
};

static void
pw_setup_nopw_cb(void *closure)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)closure;
    XPDialogStrings *strings;

    strings = XP_GetDialogStrings(XP_PW_SETUP_REFUSED_STRINGS);
    if (strings) {
	XP_MakeHTMLDialog(mystate->proto_win, &setupNoPWDialog,
			  XP_SETUPNSPW_TITLE_STRING,
			  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }
}

static void
pw_setup_done_cb(void *closure)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)closure;

    if (mystate->cb2) {
	if (mystate->button == XP_DIALOG_CANCEL_BUTTON) {
	    (* mystate->cb2)(mystate->cbarg, PR_TRUE);
	} else {
	    (* mystate->cb2)(mystate->cbarg, PR_FALSE);
	}
    }

    if (mystate->sso != NULL) {
	/* clear and free the password */
	SECNAV_ZeroPassword(mystate->sso);
	PORT_Free(mystate->sso);
    }

    if (mystate->slot != NULL) {
	PK11_FreeSlot(mystate->slot);
    }
    PORT_Free(mystate);
}

static PRBool
pwSetupHandler(XPDialogState *state, char **argv, int argc, unsigned int button)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)(state->arg);
    char *pw1 = NULL, *pw2 = NULL;
    SECStatus rv;

    /* save button so we can use it in pw_setup_done_cb */
    mystate->button = button;

    if (button == XP_DIALOG_CANCEL_BUTTON) {
	if (mystate->cb1) {
	    (* mystate->cb1)(mystate->cbarg, PR_TRUE);
	}
	goto done;
    }

    if (button == XP_DIALOG_MOREINFO_BUTTON) {
	XPDialogStrings *strings;

	strings = XP_GetDialogStrings(XP_PW_MOREINFO_STRINGS);
	if ( strings ) {
	    XP_MakeHTMLDialog(state->window, &infoDialog,
			      XP_KEY_GEN_MOREINFO_TITLE_STRING,
			      strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	return PR_TRUE;
    }

    pw1 = XP_FindValueInArgs("password1", argv, argc);
    pw2 = XP_FindValueInArgs("password2", argv, argc);

    if ((pw1 != NULL) && (pw2 != NULL)) {
	if (PORT_Strcmp(pw1, pw2) != 0) {
	    /* passwords are different; force retry */
	    mystate->error = SEC_ERROR_RETRY_PASSWORD;
	    goto retry;
	}
	
	/* check for good password here */
        if (PK11_VerifyPW(mystate->slot, pw1) != SECSuccess) {
	    mystate->error = SEC_ERROR_INVALID_PASSWORD;
	    goto retry;
	}
    } else {
	PORT_Assert(0);
	return PR_FALSE;
    }

    if (PK11_NeedUserInit(mystate->slot)) {
    	rv = PK11_InitPin(mystate->slot, mystate->sso, pw1);
    } else {
    	rv = PK11_ChangePW(mystate->slot, NULL, pw1);
    }

    if (mystate->cb1) {
	(* mystate->cb1)(mystate->cbarg, PR_FALSE);
    }

    if (*pw1 == '\0') {
	/* no password, pop up the no password dialog.  It will do cleanup. */
	state->deleteCallback = pw_setup_nopw_cb;
	state->cbarg = (void *)mystate;
	goto finally;
    }

done:
    state->deleteCallback = pw_setup_done_cb;
    state->cbarg = (void *)mystate;

finally:
    if (pw1 != NULL)
	SECNAV_ZeroPassword(pw1);
    if (pw2 != NULL)
	SECNAV_ZeroPassword(pw2);
    return PR_FALSE;

retry:
    state->deleteCallback = pw_setup_retry_cb;
    state->cbarg = (void *)mystate;
    goto finally;
}

static XPDialogInfo pwSetupSSODialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    pwSetupSSOHandler,
    600,
    400
};

static void
pw_setup_sso_retry_cb(void *closure)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)closure;
    XPDialogStrings *strings;
    char *tokenName;

    strings = XP_GetDialogStrings(XP_PW_RETRY_SSO_STRINGS);
    if (strings) {
	tokenName = PK11_GetTokenName(mystate->slot);
	XP_CopyDialogString(strings, 0, tokenName);
	XP_CopyDialogString(strings, 1, tokenName);
	XP_MakeHTMLDialog(mystate->proto_win, &pwSetupSSODialog,
			  XP_SETUPNSPW_TITLE_STRING,
			  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }
}

static XPDialogInfo pwSSOFailDialog = {
    XP_DIALOG_OK_BUTTON,
    NULL,
    400,
    200
};

static void
pw_setup_sso_fail_cb(void *closure)
{
    PWSetupPanelState *mystate = (PWSetupPanelState *)closure;
    XPDialogStrings *strings;
    char *tokenName;

    strings = XP_GetDialogStrings(XP_PW_SETUP_SSO_FAIL_STRINGS);
    if (strings) {
	tokenName = PK11_GetTokenName(mystate->slot);
	XP_CopyDialogString(strings, 0, tokenName);
	XP_CopyDialogString(strings, 1, XP_GetString(mystate->error));
	XP_MakeHTMLDialog(mystate->proto_win, &pwSetupSSODialog,
			  XP_SETUPNSPW_TITLE_STRING,
			  strings, NULL);
	XP_FreeDialogStrings(strings);
    }
    if (mystate->sso != NULL) {
	/* clear and free the password */
	SECNAV_ZeroPassword(mystate->sso);
	PORT_Free(mystate->sso);
    }
    if (mystate->slot != NULL) {
	PK11_FreeSlot(mystate->slot);
    }
    PORT_Free(mystate);
}

static PRBool
pwSetupSSOHandler(XPDialogState *state, char **argv, int argc,
		  unsigned int button)
{
    PWSetupPanelState *mystate;
    char *ssopw;
    SECStatus rv;
    
    mystate = (PWSetupPanelState *)(state->arg);

    ssopw = XP_FindValueInArgs("ssopassword", argv, argc);
    if (ssopw == NULL) {
	return PR_TRUE;
    } else {
	/* check to make sure it's valid */
	rv = PK11_CheckSSOPassword(mystate->slot,ssopw);
	switch (rv) {
	case SECSuccess:
	  /* checked out, not logout */
	  mystate->sso = PORT_Strdup(ssopw);
	  state->deleteCallback = pw_setup_retry_cb;
	  mystate->error = 0;
	  break;
	case SECFailure:
	  state->deleteCallback = pw_setup_sso_fail_cb;
	  mystate->error = PORT_GetError();
	  break;
	default:
	  /* everything is functioning, the Pin was just entered wrong */
	  state->deleteCallback = pw_setup_sso_retry_cb;
	  break;
	}
	SECNAV_ZeroPassword(ssopw);
    }
    state->cbarg = (void *)mystate;
    return PR_FALSE;
}

void
SECNAV_MakePWSetupDialog(void *proto_win, SECNAVPWSetupCallback cb1,
			 SECNAVPWSetupCallback cb2, PK11SlotInfo *slot,
			 void *cbarg)
{
    PWSetupPanelState *mystate;
    XPDialogStrings *strings = NULL;
    char *tokenName;
    
    mystate = (PWSetupPanelState *)PORT_ZAlloc(sizeof(PWSetupPanelState));
    if ( mystate == NULL ) {
	return;
    }
    
    mystate->proto_win = proto_win;
    mystate->sso = NULL;
    mystate->cb1 = cb1;
    mystate->cb2 = cb2;
    mystate->slot = PK11_ReferenceSlot(slot);
    mystate->cbarg = cbarg;
    mystate->error = 0;

    if (PK11_IsInternal(slot) || !PK11_NeedUserInit(slot)) {
	strings = XP_GetDialogStrings(XP_PW_SETUP_STRINGS);
	if (strings) {
	    XP_MakeHTMLDialog(proto_win, &pwSetupDialog,
			      XP_SETUPNSPW_TITLE_STRING,
			      strings, (void *)mystate);
	}
    } else {
	strings = XP_GetDialogStrings(XP_SSO_GET_PW_STRINGS);
	tokenName = PK11_GetTokenName(mystate->slot);
	XP_CopyDialogString(strings, 0, tokenName);
	XP_CopyDialogString(strings, 1, tokenName);
	if (strings) {
	    XP_MakeHTMLDialog(proto_win, &pwSetupSSODialog,
			      XP_SETUPNSPW_TITLE_STRING,
			      strings, (void *)mystate);
	}
    }
    if (strings) {
	XP_FreeDialogStrings(strings);
    }
}

typedef struct {
    void *proto_win;
    char *oldpw;
    PK11SlotInfo *slot;
    int error;
} PWChangePanelState;

static XPDialogInfo pwChangeDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_MOREINFO_BUTTON,
    pwChangeHandler,
    600,
    400
};

static void
pw_change_retry_cb(void *closure)
{
    PWChangePanelState *mystate = (PWChangePanelState *)closure;
    XPDialogStrings *strings;
#ifndef AKBAR
    XPDialogState *state = NULL;
#endif /* !AKBAR */
    char *tokenName;

    strings = XP_GetDialogStrings(XP_PW_CHANGE_STRINGS);
    if (strings) {
	tokenName = PK11_GetTokenName(mystate->slot);
	if (tokenName != NULL) {
	    XP_CopyDialogString(strings, 0, tokenName);
	}
	if (mystate->oldpw != NULL)
	    XP_CopyDialogString(strings, 1, mystate->oldpw);
#ifndef AKBAR
	state = 
#endif /* !AKBAR */
	        XP_MakeHTMLDialog(mystate->proto_win, &pwChangeDialog,
				  XP_CHANGENSPW_TITLE_STRING,
				  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }
    if ((mystate->error != 0)
#ifndef AKBAR
	&& (state != NULL) &&
	(state->window != NULL)
#endif /* !AKBAR */
	) {
	FE_Alert(
#ifndef AKBAR
		 state->window,
#else  /* AKBAR */
		 mystate->proto_win,
#endif /* AKBAR */
		 XP_GetString(mystate->error));
    }
}

static void
pw_change_fail_cb(void *closure)
{
    XPDialogStrings *strings;

    strings = XP_GetDialogStrings(XP_PW_CHANGE_FAILURE_STRINGS);
    if (strings != NULL) {
	XP_MakeHTMLDialog((MWContext *)closure, &failureDialog,
			  XP_PWERROR_TITLE_STRING, strings, NULL);
	XP_FreeDialogStrings(strings);
    }
}

static XPDialogInfo changeNoPWDialog = {
    XP_DIALOG_OK_BUTTON,
    NULL,
    500,
    300
};

static void
pw_change_nopw_cb(void *closure)
{
    PWChangePanelState *mystate = (PWChangePanelState *)closure;
    XPDialogStrings *strings;

    strings = XP_GetDialogStrings(XP_PW_SETUP_REFUSED_STRINGS);
    if (strings) {
	XP_MakeHTMLDialog(mystate->proto_win, &changeNoPWDialog,
			  XP_CHANGENSPW_TITLE_STRING,
			  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }

    if (mystate->oldpw != NULL) {
	SECNAV_ZeroPassword(mystate->oldpw);
	PORT_Free(mystate->oldpw);
    }
    
    if (mystate->slot != NULL) {
	PK11_FreeSlot(mystate->slot);
    }
    PORT_Free(mystate);
}

static PRBool
pwChangeHandler(XPDialogState *state, char **argv, int argc,
		unsigned int button)
{
    PWChangePanelState *mystate;
    SECStatus rv;
    char *pw1 = NULL, *pw2 = NULL, *oldpw = NULL;

    mystate = (PWChangePanelState *)(state->arg);

    if (button == XP_DIALOG_CANCEL_BUTTON) {
	goto done;
    }

    if (button == XP_DIALOG_MOREINFO_BUTTON) {
	XPDialogStrings *strings;

	strings = XP_GetDialogStrings(XP_PW_MOREINFO_STRINGS);
	if ( strings ) {
	    XP_MakeHTMLDialog(state->window, &infoDialog,
			      XP_CHANGENSPW_TITLE_STRING,
			      strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	return PR_TRUE;
    }

    oldpw = XP_FindValueInArgs("password", argv, argc);
    pw1 = XP_FindValueInArgs("password1", argv, argc);
    pw2 = XP_FindValueInArgs("password2", argv, argc);

    if (oldpw == NULL) {
	PORT_Assert(0);
	return PR_FALSE;
    } else if (PORT_Strlen(oldpw) != 0) {
	/* Maybe this should pop up a special error dialog.  It gets the 
	 * normal bad password one anyway. */
    }

    rv = PK11_CheckUserPassword(mystate->slot, oldpw);
    if (rv != SECSuccess) {
	mystate->error = SEC_ERROR_RETRY_OLD_PASSWORD;
	SECNAV_ZeroPassword(oldpw);
	oldpw = NULL;
	goto retry;
    }

    if ((pw1 != NULL) && (pw2 != NULL)) {
	if (PORT_Strcmp(pw1, pw2) != 0) {
	    /* passwords are different; force retry */
	    mystate->error = SEC_ERROR_RETRY_PASSWORD;
	    goto retry;
	}
	
	/* check for good password here */
        if (PK11_VerifyPW(mystate->slot, pw1) != SECSuccess) {
	    mystate->error = SEC_ERROR_INVALID_PASSWORD;
	    goto retry;
	}
    } else {
	PORT_Assert(0);
	return PR_FALSE;
    }

    rv = PK11_ChangePW(mystate->slot, oldpw, pw1);

    if (rv != SECSuccess) {
	state->deleteCallback = pw_change_fail_cb;
	state->cbarg = (void *)mystate->proto_win;
	goto done;
    }

    if (*pw1 == '\0') {
	/* no password, pop up the no password dialog.  It will do cleanup. */
	state->deleteCallback = pw_change_nopw_cb;
	state->cbarg = (void *)mystate;
	goto finally;
    }

done:
    if (mystate->oldpw != NULL) {
	SECNAV_ZeroPassword(mystate->oldpw);
	PORT_Free(mystate->oldpw);
    }
    
    if (mystate->slot != NULL) {
	PK11_FreeSlot(mystate->slot);
    }
    PORT_Free(mystate);

finally:
    if (oldpw != NULL)
	SECNAV_ZeroPassword(oldpw);
    if (pw1 != NULL)
	SECNAV_ZeroPassword(pw1);
    if (pw2 != NULL)
	SECNAV_ZeroPassword(pw2);
    return PR_FALSE;

retry:
    if (mystate->oldpw != NULL) {
	SECNAV_ZeroPassword(mystate->oldpw);
	PORT_Free(mystate->oldpw);
	mystate->oldpw = NULL;
    }
    if (oldpw != NULL) {
	mystate->oldpw = PORT_Strdup(oldpw);
    }
    state->deleteCallback = pw_change_retry_cb;
    state->cbarg = (void *)mystate;
    goto finally;
}

void
SECNAV_MakePWChangeDialog(void *proto_win, PK11SlotInfo *slot)
{
    PWChangePanelState *mystate;
    XPDialogStrings *strings;
    char *tokenName;
    
    mystate = (PWChangePanelState *)PORT_ZAlloc(sizeof(PWChangePanelState));
    if ( mystate == NULL ) {
	return;
    }
    
    mystate->proto_win = proto_win;
    mystate->oldpw = NULL;
    mystate->slot = PK11_ReferenceSlot(slot);

    strings = XP_GetDialogStrings(XP_PW_CHANGE_STRINGS);
    if (strings) {
	tokenName = PK11_GetTokenName(mystate->slot);
	if (tokenName != NULL) {
	    XP_CopyDialogString(strings, 0, tokenName);
	}
	XP_MakeHTMLDialog(proto_win, &pwChangeDialog,
			  XP_CHANGENSPW_TITLE_STRING,
			  strings, (void *)mystate);
	XP_FreeDialogStrings(strings);
    }
}

