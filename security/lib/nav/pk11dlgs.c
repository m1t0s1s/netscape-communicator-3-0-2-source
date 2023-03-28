/*
 * Fortezza Security stuff specific to the client (mozilla).
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: pk11dlgs.c,v 1.1.2.9 1997/05/24 00:23:40 jwz Exp $
 */

#include "xp.h"
#include "seccomon.h"
#include "fe_proto.h"
#include "htmldlgs.h"
#include "secmod.h"
#include "pk11dlgs.h"
#include "xpgetstr.h"
#include "pk11func.h"
#include "pkcs11t.h"
#include "secnav.h"

extern int XP_DIALOG_SLOT_INFO;
extern int XP_DIALOG_SLOT_INFO_TITLE;
extern int XP_SEC_MODULE_NO_LIB;
extern int XP_DIALOG_EDIT_MODULE;
extern int XP_DIALOG_NEW_MODULE;
extern int XP_DIALOG_EDIT_MODULE_TITLE;
extern int XP_DIALOG_NEW_MODULE_TITLE;
extern int XP_FIPS_MESSAGE_1;
extern int XP_INT_MODULE_MESSAGE_1;
extern int SEC_INTERNAL_ONLY;
extern int SEC_ERROR_SLOT_SELECTED;
extern int SEC_ERROR_NO_TOKEN;
extern int SEC_ERROR_NO_MODULE;
extern int XP_SEC_MOREINFO;
extern int XP_SEC_EDIT;
extern int XP_SEC_LOGIN;
extern int XP_SEC_NO_LOGIN_NEEDED;
extern int XP_SEC_ALREADY_LOGGED_IN;
extern int XP_SEC_LOGOUT;
extern int XP_SEC_SETPASSWORD;
extern int XP_SLOT_LOGIN_REQUIRED;
extern int XP_SLOT_NO_LOGIN_REQUIRED;
extern int XP_SLOT_READY;
extern int XP_SLOT_NOT_LOGGED_IN;
extern int XP_SLOT_UNITIALIZED;
extern int XP_SLOT_NOT_PRESENT;
extern int XP_SLOT_DISABLED;
extern int XP_SLOT_DISABLED_2;
extern int XP_SLOT_PASSWORD_SET;
extern int XP_SLOT_PASSWORD_INIT;
extern int XP_SLOT_PASSWORD_CHANGE;
extern int XP_SLOT_PASSWORD_NO;

static PRBool
secnav_InfoDialogHandler(XPDialogState *state, char **argv,
				 	     int argc, unsigned int button) {
	return PR_FALSE;
}

static XPDialogInfo secnavInfoDialogInfo = {
    XP_DIALOG_OK_BUTTON,
    secnav_InfoDialogHandler,
    400,
    400
};

static void
secnav_SlotInfoDialog(MWContext *cx,PK11SlotInfo *slot, char *slot_name) {
	XPDialogStrings *strings;
	CK_SLOT_INFO slotInfo;
	CK_TOKEN_INFO tokenInfo;
	char buf[65];
	PRBool foundToken = PR_FALSE;

	strings = XP_GetDialogStrings(XP_DIALOG_SLOT_INFO);
	if (PK11_GetSlotInfo(slot,&slotInfo) == SECFailure) {
	    PORT_Memset(slotInfo.slotDescription,' ',
					sizeof(slotInfo.slotDescription));
	    PORT_Memset(slotInfo.manufacturerID,' ',
					sizeof(slotInfo.manufacturerID));
	    slotInfo.flags = 0;
	    slotInfo.hardwareVersion.major = 0;
	    slotInfo.hardwareVersion.minor = 0;
	    slotInfo.firmwareVersion.major = 0;
	    slotInfo.firmwareVersion.minor = 0;
	}
	PK11_MakeString(NULL,buf,(char *)slotInfo.slotDescription,
					sizeof(slotInfo.slotDescription));
	XP_CopyDialogString(strings,0,buf);

	PK11_MakeString(NULL,buf,(char *)slotInfo.manufacturerID,
					sizeof(slotInfo.manufacturerID));
	XP_CopyDialogString(strings,1,buf);

	PR_snprintf(buf, sizeof(buf), "%d.%d",
		slotInfo.hardwareVersion.major, slotInfo.hardwareVersion.minor);
	XP_CopyDialogString(strings,2,buf);

	PR_snprintf(buf, sizeof(buf), "%d.%d",
		slotInfo.firmwareVersion.major, slotInfo.firmwareVersion.minor);
	XP_CopyDialogString(strings,3,buf);

        if (slotInfo.flags & CKF_TOKEN_PRESENT) {
	    if (PK11_GetTokenInfo(slot,&tokenInfo) == SECSuccess) {
		foundToken = PR_TRUE;
	    }
	}

	if (!foundToken) {
	    PORT_Memset(tokenInfo.label,' ', sizeof(tokenInfo.label));
	    PORT_Memset(tokenInfo.manufacturerID,' ',
					sizeof(tokenInfo.manufacturerID));
	    PORT_Memset(tokenInfo.model,' ', sizeof(tokenInfo.model));
	    PORT_Memset(tokenInfo.serialNumber,'0',
					sizeof(tokenInfo.serialNumber));
	    tokenInfo.flags = 0;
	    tokenInfo.hardwareVersion.major = 0;
	    tokenInfo.hardwareVersion.minor = 0;
	    tokenInfo.firmwareVersion.major = 0;
	    tokenInfo.firmwareVersion.minor = 0;
	    XP_CopyDialogString(strings,4,"<I>");
	} else {
	    XP_CopyDialogString(strings,4,"");
	}

	PK11_MakeString(NULL,buf,(char *)tokenInfo.label,
						sizeof(tokenInfo.label));
	XP_CopyDialogString(strings,5,buf);
	PK11_MakeString(NULL,buf,(char *)tokenInfo.manufacturerID,
					sizeof(tokenInfo.manufacturerID));
	XP_CopyDialogString(strings,6,buf);
	PK11_MakeString(NULL,buf,(char *)tokenInfo.model,
						sizeof(tokenInfo.model));
	XP_CopyDialogString(strings,7,buf);
	PK11_MakeString(NULL,buf,(char *)tokenInfo.serialNumber,
					sizeof(tokenInfo.serialNumber));
	XP_CopyDialogString(strings,8,buf);

	PR_snprintf(buf, sizeof(buf), "%d.%d",
	    tokenInfo.hardwareVersion.major, tokenInfo.hardwareVersion.minor);
	XP_CopyDialogString(strings,9,buf);

	PR_snprintf(buf, sizeof(buf), "%d.%d",
	    tokenInfo.firmwareVersion.major, tokenInfo.firmwareVersion.minor);
	XP_CopyDialogString(strings,10,buf);

	/* also show enabled/disabled state, and token flags, 
	 * possibly slot flags */
	if (PK11_NeedLogin(slot)) {
	    XP_CopyDialogString(strings,11,
				XP_GetString(XP_SLOT_LOGIN_REQUIRED));
	} else {
	    XP_CopyDialogString(strings,11,
				XP_GetString(XP_SLOT_NO_LOGIN_REQUIRED));
	}

	if (PK11_IsDisabled(slot)) {
	    XP_CopyDialogString(strings,12,XP_GetString(XP_SLOT_DISABLED));
	    XP_CopyDialogString(strings,13,
				XP_GetString(PK11_GetDisabledReason(slot)));
	    XP_CopyDialogString(strings,14,XP_GetString(XP_SLOT_DISABLED_2));
	} else {
	    XP_CopyDialogString(strings,13,"");
	    XP_CopyDialogString(strings,14,"");
	    if (!PK11_IsPresent(slot)) {
		XP_CopyDialogString(strings,12,
					XP_GetString(XP_SLOT_NOT_PRESENT));
	    } else if (PK11_NeedLogin(slot) && PK11_NeedUserInit(slot)) {
		XP_CopyDialogString(strings,12,
					XP_GetString(XP_SLOT_UNITIALIZED));
	    } else if (PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot)) {
		XP_CopyDialogString(strings,12,
					XP_GetString(XP_SLOT_NOT_LOGGED_IN));
	    } else {
		/* must be ready... */
		XP_CopyDialogString(strings,12, XP_GetString(XP_SLOT_READY));
	    }
	}

	XP_MakeHTMLDialog(cx,&secnavInfoDialogInfo,
		 XP_DIALOG_SLOT_INFO_TITLE, strings, NULL);
	XP_FreeDialogStrings(strings);
	return;
}

/*
 * handle the Edit dialog case
 */
static PRBool
secnav_EditDialogHandler(XPDialogState *state, char **argv,
				 	     int argc, unsigned int button) {
    SECMODModule *module = (SECMODModule *)state->arg;
    char *slot_name;
    char *buttonString;
    PK11SlotInfo *slot;

    if ( (button == 0) || (button == XP_DIALOG_MOREINFO_BUTTON) ) {
	slot_name = XP_FindValueInArgs("slots", argv, argc);
	if (slot_name == NULL) {
	    /* XXXXX */
	    FE_Alert(state->window,XP_GetString(SEC_ERROR_NO_TOKEN));
	    return PR_TRUE;
	}
	
	buttonString = XP_FindValueInArgs("button", argv, argc);
	slot = SECMOD_FindSlot(module,slot_name);
	if (slot == NULL) {
	    FE_Alert(state->window,XP_GetString(SEC_ERROR_NO_TOKEN));
	    return PR_TRUE;
	}
	if ( buttonString != NULL ) {
	    if ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SEC_MOREINFO)) == 0 ) {
		/* new dialog strings */
		secnav_SlotInfoDialog(state->window,slot,slot_name);
	    } else if ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SEC_EDIT)) == 0 ) {
		/* new dialog strings */
		/* return PR_TRUE;  don't free the slot yet */
	    } else if ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SEC_LOGIN)) == 0 ) {
		/*
		 * log into the selected slot as best you can...
		 */
		if (!PK11_NeedLogin(slot)) {
		    FE_Alert(state->window,
				XP_GetString(XP_SEC_NO_LOGIN_NEEDED));
		} else if (PK11_IsLoggedIn(slot)) {
		    FE_Alert(state->window,
				XP_GetString(XP_SEC_ALREADY_LOGGED_IN));
		} else {
		    PK11_DoPassword(slot,state->window);
		}
	    } else if ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SEC_LOGOUT)) == 0 ) {
		PK11_Logout(slot);
	    } else if (( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SLOT_PASSWORD_INIT)) == 0 ) ||
	           ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SLOT_PASSWORD_SET)) == 0 ) ||
	           ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SLOT_PASSWORD_NO)) == 0 ) ||
	           ( PORT_Strcasecmp(buttonString,
                               XP_GetString(XP_SLOT_PASSWORD_CHANGE)) == 0 )) {
		if (PK11_NeedLogin(slot) && PK11_NeedUserInit(slot)) {
			SECNAV_MakePWSetupDialog(state->window, NULL, NULL,
						 slot, NULL);
		} else if (!PK11_NeedLogin(slot) && !PK11_NeedUserInit(slot)) {
			SECNAV_MakePWSetupDialog(state->window, NULL, NULL,
						 slot, NULL);
		} else if (PK11_NeedLogin(slot)) {
			SECNAV_MakePWChangeDialog(state->window, slot);
		}
	    }
	}
	PK11_FreeSlot(slot);
	return PR_TRUE;
    }


    SECMOD_DestroyModule(module);
    return PR_FALSE;
}


static XPDialogInfo secnavEditDialogInfo = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    secnav_EditDialogHandler,
    450,
    420
};

static SECStatus
secnav_EditDialog(MWContext *cx,SECMODModule *module) {
	XPDialogStrings *strings;
	CK_INFO modInfo;
	SECStatus rv;
	char buf[65];
	char *slotNames;
	char *slotTypes;
	int i , type = 0;
	char *lib = NULL;

	strings = XP_GetDialogStrings(XP_DIALOG_EDIT_MODULE);

	if (!module->internal) {
	    lib = module->dllName;
	} else {
	    lib = XP_GetString(SEC_INTERNAL_ONLY);
        }
	XP_CopyDialogString(strings,0, module->commonName);
	XP_CopyDialogString(strings,1,"hidden");  /* HTML: don't I18E */
	XP_CopyDialogString(strings,2, module->commonName);
	XP_CopyDialogString(strings,3, lib);
	XP_CopyDialogString(strings,4,"hidden");  /* HTML: don't I18E */
	XP_CopyDialogString(strings,5,"");

	rv = PK11_GetModInfo(module,&modInfo);
	if (rv == SECSuccess) {
	    char *tmp;
	    tmp = PK11_MakeString(NULL,buf,(char *)modInfo.manufacturerID,
					sizeof(modInfo.manufacturerID));
	    XP_CopyDialogString(strings,6,tmp);
	    tmp = PK11_MakeString(NULL,buf,(char *)modInfo.libraryDescription,
					sizeof(modInfo.libraryDescription));
	    XP_CopyDialogString(strings,8,tmp);
	    PR_snprintf(buf, sizeof(buf), "%d.%d",
		modInfo.cryptokiVersion.major, modInfo.cryptokiVersion.minor);
	    XP_CopyDialogString(strings,7,buf);
	    PR_snprintf(buf, sizeof(buf), "%d.%d",
		modInfo.libraryVersion.major, modInfo.libraryVersion.minor);
	    XP_CopyDialogString(strings,9,buf);
	} else {
	    XP_CopyDialogString(strings,6,"");
	    XP_CopyDialogString(strings,8,"");
	    XP_CopyDialogString(strings,7,"0.0");
	    XP_CopyDialogString(strings,9,"0.0");
	}

	slotNames = PORT_Strdup("");
	slotTypes = PORT_Strdup("initarray(");
	for (i=0; i < module->slotCount; i++) {
	    PK11SlotInfo *slot = module->slots[i];
	    if (PK11_IsPresent(slot)) {
		slotNames=PR_sprintf_append(slotNames,"<option>%s",
						PK11_GetTokenName(slot));
	    } else {
		slotNames=PR_sprintf_append(slotNames,"<option>%s",
						PK11_GetSlotName(slot));
	    }
	    if (PK11_NeedLogin(slot) && PK11_NeedUserInit(slot)) {
		type = XP_SLOT_PASSWORD_INIT;
	    } else if (!PK11_NeedLogin(slot) && !PK11_NeedUserInit(slot)) {
		type = XP_SLOT_PASSWORD_SET;
	    } else if (PK11_NeedLogin(slot)) {
		type = XP_SLOT_PASSWORD_CHANGE;
	    } else {
		type = XP_SLOT_PASSWORD_NO;
	    }
	    slotTypes=PR_sprintf_append(slotTypes,"%s\"%s\"",
			(i==0 ? " ":", "), XP_GetString(type));
	}
	slotTypes=PR_sprintf_append(slotTypes," );");
	XP_CopyDialogString(strings,10,slotNames);
	XP_CopyDialogString(strings,11,slotTypes);
	PORT_Free(slotNames);
	PORT_Free(slotTypes);

	XP_MakeHTMLDialog(cx,&secnavEditDialogInfo,
		 XP_DIALOG_EDIT_MODULE_TITLE, strings, (char *)module);
	XP_FreeDialogStrings(strings);
	return SECWouldBlock;
}


/*
 * strip the blanks from front and back from a string. *NOTE this
 * function will mangle any string it gets.
 */
static char *
stripblanks(char *in)
{
	char *str = in;
	int len = strlen(in)-1;
	char *end = &in[len];

	while (*str == ' ') str++;
	while (*end == ' ' && (end >= str)) end--;
	if (end >= str) end[1]=0;
	return str;
}

static PRBool
secnav_NewDialogHandler(XPDialogState *state, char **argv,
				 	     int argc, unsigned int button) {
    SECNAVDeleteCertCallback *delcb = (SECNAVDeleteCertCallback *)state->arg;
    if (button == XP_DIALOG_OK_BUTTON) {
	char *name = XP_FindValueInArgs("name", argv, argc);
	char *dll = XP_FindValueInArgs("library", argv, argc);
        SECMODModule *module = SECMOD_NewModule();

        if (name) {	
		module->commonName=PORT_ArenaStrdup(module->arena,name);
		module->commonName=stripblanks(module->commonName);
	} else {
		module->commonName=NULL;
	}
	if (dll) {
		module->dllName=PORT_ArenaStrdup(module->arena,dll);
	} else {
		module->dllName=NULL;
	}
	if ((module->dllName == NULL) && (module->dllName[0] == 0)) {
		FE_Alert(state->window,XP_GetString(XP_SEC_MODULE_NO_LIB));
    		SECMOD_DestroyModule(module);
		return PR_TRUE;
	}
	SECMOD_AddModule(module);
	(*delcb->callback)(delcb->cbarg);
    } else {
    PORT_Free(delcb->cbarg);
	} 
    PORT_Free(delcb);
    return PR_FALSE;
}


static XPDialogInfo secnavNewDialogInfo = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    secnav_NewDialogHandler,
    350,
    200
};


void
SECNAV_EditModule(MWContext *context, char *name) {
    SECMODModule *mod = NULL;

    if (name) mod = SECMOD_FindModule(name);
    if (mod == NULL) {
	/*FE_Alert(context,XP_GetString(XP_SEC_MODULE_MISSING));*/
	return;
    }

    secnav_EditDialog(context,mod);
   /* free module later */
}

void
SECNAV_LogoutSecurityModule(MWContext *context, char *name) {

    /* don't do anything for now */
    SECMODModule *mod = SECMOD_FindModule(name);

    if (mod == NULL) {
	/* FE_Alert(context,XP_GetString(XP_SEC_MODULE_MISSING));*/
	return;
    }

    /* secnav_logout(mod); */
    SECMOD_DestroyModule(mod);

}

void
SECNAV_LogoutAllSecurityModules(MWContext *context) {

    PK11_LogoutAll();
}

void
SECNAV_NewSecurityModule(MWContext *context,void (*func)(void*),void *arg)
{
     /* put up new dialog */
    XPDialogStrings *strings;
    SECNAVDeleteCertCallback *redraw = (SECNAVDeleteCertCallback *) 
				PORT_Alloc(sizeof(SECNAVDeleteCertCallback));
    if (redraw == NULL) {
	PORT_Free(arg);
	return;
    }
    redraw->callback = func;
    redraw->cbarg = arg;

    strings = XP_GetDialogStrings(XP_DIALOG_NEW_MODULE);

    XP_MakeHTMLDialog(context, &secnavNewDialogInfo,
			XP_DIALOG_NEW_MODULE_TITLE, strings, redraw);
    XP_FreeDialogStrings(strings);
}

void
SECNAV_DeleteSecurityModule(MWContext *context, char *name,
			SECNAVDeleteSecModCallback func, void *arg) {
    SECStatus rv;
    int type;
    int message_id= 0;
    char *message = NULL;

    rv = SECMOD_DeleteModule(name,&type);
    if (rv == SECSuccess) {
	(*func)(arg);
    } else {
	switch (type) {
        case SECMOD_INTERNAL:
		message_id = XP_FIPS_MESSAGE_1;
		break;
	case SECMOD_FIPS:
		message_id = XP_INT_MODULE_MESSAGE_1;
		break;
	default:
		FE_Alert(context,XP_GetString(SEC_ERROR_NO_MODULE));
		PORT_Free(arg);
		return;
	}
	message = PORT_Strdup(XP_GetString(message_id));
	if (message) {
		message = PR_sprintf_append(message,XP_GetString(message_id+1));
	}
	if (message && FE_Confirm(context,message)) {
	    PORT_Free(message);
	    rv = SECMOD_DeleteInternalModule(name);
	    (*func)(arg);
	    return;
	}
	if (message) PORT_Free(message);
	PORT_Free(arg);
    }
}

