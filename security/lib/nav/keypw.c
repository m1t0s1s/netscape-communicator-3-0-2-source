/*
 * Password handling routines for Mozilla
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: keypw.c,v 1.34.2.2 1997/03/22 23:39:04 jwz Exp $
 */

#include "xp.h"
#include "secnav.h"
#include "pk11func.h"
#include "xpgetstr.h"
#include "key.h"
#include "secport.h"
#include "secitem.h"

extern int XP_SEC_ENTER_PWD;
extern int XP_SEC_ENTER_KEYFILE_PWD;

/*
 * routine to zero out a password.  We avoid PORT_Memset(ptr, 0, PORT_Strlen(ptr))
 * because the win16 compiler generates bogus register trashing code for it.
 */
void
SECNAV_ZeroPassword(char *password)
{
    PORT_Assert(password != NULL);
    
    if ( password == NULL ) {
	return;
    }
    
    while ( *password ) {
	*password++ = '\0';
    }
    
    return;
}

void
secmoz_SetPWEnabled(MWContext *context, PRBool enable)
{
    FE_SetPasswordEnabled(context, enable);
}

void
SECNAV_SetPasswordAskPrefs(int askPW, int timeout)
{
    PK11SlotInfo *slot = PK11_GetInternalKeySlot();

    PK11_SetSlotPWValues(slot,askPW,timeout);
    PK11_FreeSlot(slot);
}

/*
 * Routine to return the current private key password.
 * It is set up to be passed to the various key database routines that
 * require a password.
 */
SECItem *
SECNAV_GetKeyDBPassword(void *arg, SECKEYKeyDBHandle *keydb)
{
    char *pw;
    MWContext *cx;
    SECItem *pwitem;
    
    PORT_Assert(keydb == SECKEY_GetDefaultKeyDB());

    if (!SECNAV_GetKeyDBPasswordStatus()) {
	pwitem = SECKEY_HashPassword("", keydb->global_salt);
	return(pwitem);
    }

    cx = (MWContext*)arg;

    /*
     * loop until the password matches or the user gives up
     * (or something "bad" happens ;-)
     */
    while (1) {
	/*
	 * get the password
	 */
	pw = FE_PromptPassword(cx, XP_GetString(XP_SEC_ENTER_KEYFILE_PWD));
	if (pw == NULL)	{	/* user hit cancel */
	    goto loser;
	}

	/*
	 * and make a key from it
	 */
	
	pwitem = SECKEY_HashPassword(pw, keydb->global_salt);
	
	/*
	 * clear and free the password string
	 */
	SECNAV_ZeroPassword(pw);
	PORT_Free(pw);

	/*
	 * if the key-making failed, we can only give up
	 */
	if ( pwitem == NULL ) {
	    goto loser;
	}

	/*
	 * confirm the password
	 */
	if (SECKEY_CheckKeyDBPassword(keydb, pwitem) == SECSuccess) {
	    return pwitem;
	}

	SECITEM_ZfreeItem(pwitem, PR_TRUE);
	pwitem = NULL;
    }
    
    PORT_Assert(0);	/* should never get here!! */
    
loser:
    return(NULL);
}

/*
 * Is the security library using a password?
 */
PRBool
SECNAV_GetKeyDBPasswordStatus(void)
{
    SECItem *pwitem;
    PRBool pwset;
    
    SECKEYKeyDBHandle *keydb;

    /*
     * the first time, we need to check that they really are
     * already using, or not using, a password
     */
    keydb = SECKEY_GetDefaultKeyDB();
    pwset = PR_FALSE;
    if ((keydb != NULL) && (SECKEY_HasKeyDBPassword (keydb) == SECSuccess)) {
	pwitem = SECKEY_HashPassword("", keydb->global_salt);
	if ( pwitem ) {
	    if (SECKEY_CheckKeyDBPassword (keydb, pwitem) != SECSuccess) {
		pwset = PR_TRUE;
	    }
 	    SECITEM_ZfreeItem(pwitem, PR_TRUE);
	}
        pwset = PR_TRUE;
    }
    return pwset;
}

char *
SECNAV_PromptPassword(PK11SlotInfo *slot,void *wincx) {
    char *pw;
    char *prompt;
    MWContext *cx = (MWContext *)wincx;
    
    prompt = PR_smprintf(XP_GetString(XP_SEC_ENTER_PWD),
			 PK11_GetTokenName(slot));
    pw = FE_PromptPassword(cx,prompt);
    PORT_Free(prompt);

    return pw;
}
