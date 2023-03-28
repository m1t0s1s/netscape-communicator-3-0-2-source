/*
 * Cross platform html dialogs
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: htmldlgs.c,v 1.26.26.6 1997/05/03 09:24:26 jwz Exp $
 */

#include "xp.h"
#include "fe_proto.h"

#ifdef HAVE_SECURITY /* jwz */
# include "seccomon.h"
#endif

#include "net.h"
#include "htmldlgs.h"
#include "xpgetstr.h"
#include "shist.h"
#ifndef AKBAR
#include "prlog.h"
#include "libmocha.h"
#endif /* !AKBAR */

#ifndef HAVE_SECURITY /* jwz */
# define PORT_Assert		XP_ASSERT
# define PORT_Alloc		XP_ALLOC
# define PORT_Free		XP_FREE
# define PORT_Memset		XP_MEMSET
# define PORT_Memcpy		XP_MEMCPY
# define PORT_Strchr		XP_STRCHR
# define PORT_Strcmp		XP_STRCMP
# define PORT_Strlen		XP_STRLEN
# define PORT_Strcasecmp	XP_STRCASECMP

# define PORT_SetError		XP_SetError

# define SEC_ERROR_NO_MEMORY -1
# define SECSuccess  0
# define SECFailure -1
# define SECStatus int

static void *
PORT_ZAlloc(size_t bytes)
{
    void *rv;

    /* Always allocate a non-zero amount of bytes */
    rv = XP_CALLOC(1, bytes ? bytes : 1);
    if (!rv) {
	XP_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

static PRArenaPool *
PORT_NewArena(unsigned long chunksize)
{
    PRArenaPool *arena;
    
    arena = PORT_ZAlloc(sizeof(PRArenaPool));
    if ( arena != NULL ) {
	PR_InitArenaPool(arena, "non-security", chunksize, sizeof(double));
    }

    return(arena);
}

static void
PORT_FreeArena(PRArenaPool *arena, PRBool zero)
{
    PR_FinishArenaPool(arena);
    PORT_Free(arena);
}

static void *
PORT_ArenaAlloc(PRArenaPool *arena, size_t size)
{
    void *p;

    PR_ARENA_ALLOCATE(p, arena, size);
    if (p == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    return(p);
}

static void *
PORT_ArenaZAlloc(PRArenaPool *arena, size_t size)
{
    void *p;

    PR_ARENA_ALLOCATE(p, arena, size);
    if (p == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    } else {
	PORT_Memset(p, 0, size);
    }

    return(p);
}

static void *
PORT_ArenaGrow(PRArenaPool *arena, void *ptr, size_t oldsize, size_t newsize)
{
    PORT_Assert(newsize >= oldsize);
    
    PR_ARENA_GROW(ptr, arena, oldsize, ( newsize - oldsize ) );
    
    return(ptr);
}

#endif


extern int XP_SEC_CANCEL;
extern int XP_SEC_OK;
extern int XP_SEC_CONTINUE;
extern int XP_SEC_NEXT;
extern int XP_SEC_BACK;
extern int XP_SEC_FINISHED;
extern int XP_SEC_MOREINFO;
extern int XP_SEC_SHOWCERT;
extern int XP_SEC_SHOWORDER;
extern int XP_SEC_SHOWDOCINFO;
extern int XP_SEC_NEXT_KLUDGE;
extern int XP_SEC_BACK_KLUDGE;
extern int XP_SEC_RENEW;
extern int XP_SEC_NEW;
extern int XP_SEC_RELOAD;
extern int XP_SEC_DELETE;
extern int XP_SEC_EDIT;
extern int XP_SEC_LOGIN;
extern int XP_SEC_LOGOUT;
extern int XP_SEC_SETPASSWORD;
extern int XP_SEC_SAVEAS;
extern int XP_SEC_FETCH;

extern int XP_DIALOG_CANCEL_BUTTON_STRINGS;
extern int XP_DIALOG_CANCEL_CONTINUE_BUTTON_STRINGS;
extern int XP_DIALOG_CANCEL_OK_BUTTON_STRINGS;
extern int XP_DIALOG_CANCEL_OK_MOREINFO_BUTTON_STRINGS;
extern int XP_DIALOG_FETCH_CANCEL_BUTTON_STRINGS;
extern int XP_DIALOG_CONTINUE_BUTTON_STRINGS;
extern int XP_DIALOG_FOOTER_STRINGS;
extern int XP_DIALOG_HEADER_STRINGS;
extern int XP_DIALOG_MIDDLE_STRINGS;
extern int XP_DIALOG_NULL_STRINGS;
extern int XP_DIALOG_OK_BUTTON_STRINGS;
extern int XP_EMPTY_STRINGS;
extern int XP_PANEL_FIRST_BUTTON_STRINGS;
extern int XP_PANEL_FOOTER_STRINGS;
extern int XP_PANEL_HEADER_STRINGS;
extern int XP_PANEL_LAST_BUTTON_STRINGS;
extern int XP_PANEL_MIDDLE_BUTTON_STRINGS;
extern int XP_PANEL_ONLY_BUTTON_STRINGS;
extern int XP_ALERT_TITLE_STRING;
extern int XP_DIALOG_JS_HEADER_STRINGS;
extern int XP_DIALOG_JS_MIDDLE_STRINGS;
extern int XP_DIALOG_JS_FOOTER_STRINGS;

void NET_AddExitCallback( void (* func)(void *arg), void *arg);

typedef struct {
    void *window;
    void (* deleteWinCallback)(void *arg);
    void *arg;
} deleteWindowCBArg;

static void
deleteWindow(void *closure)
{
    deleteWindowCBArg *state;
    
    state = (deleteWindowCBArg *)closure;
    
    FE_DestroyWindow((MWContext *)state->window);

    if ( state->deleteWinCallback ) {
	(* state->deleteWinCallback)(state->arg);
    }
    
    PORT_Free(state);
    
    return;
}

static SECStatus
putStringToStream(NET_StreamClass *stream, char *string, PRBool quote)
{
    int rv;
    char *tmpptr;
    char *destptr;
    char *savedest;
    int curcount;
    
    if (*string == 0) {
	return(SECSuccess);
    }

    savedest = destptr = (char *)PORT_Alloc( 8001 );

    curcount = 0;
    tmpptr = string;

    if ( quote ) {
	while ( *tmpptr ) {
	    if ( *tmpptr == '\'' ) {
		*destptr = '\\';
		destptr++;
		*destptr = *tmpptr;
		destptr++;
		curcount += 2;
	    } else if ( *tmpptr == '\\' ) {
		*destptr = '\\';
		destptr++;
		*destptr = '\\';
		destptr++;
		curcount += 2;
	    } else if ( *tmpptr == '\n' ) {
		*destptr = '\\';
		destptr++;
		*destptr = 'n';
		destptr++;
		curcount += 2;
	    } else if ( *tmpptr == '\r' ) {
		*destptr = '\\';
		destptr++;
		*destptr = 'r';
		destptr++;
		curcount += 2;
	    } else if ( *tmpptr == '/' ) {
		*destptr = '\\';
		destptr++;
		*destptr = '/';
		destptr++;
		curcount += 2;
	    } else {
		*destptr = *tmpptr;
		destptr++;
		curcount++;
	    }

	    tmpptr++;
	    if ( curcount >= 8000 ) {
		rv = (*(stream)->put_block)(stream->data_object, savedest,
					    curcount);
		if ( rv < 0 ) {
		    PORT_Free(savedest);
		    return(SECFailure);
		}
		curcount = 0;
		destptr = savedest;
	    }
	}
    } else {
	while ( *tmpptr ) {
	    *destptr = *tmpptr;
	    destptr++;
	    curcount++;
	    tmpptr++;
	    if ( curcount >= 8000 ) {
		rv = (*(stream)->put_block)(stream->data_object,
					    savedest, curcount);
		if ( rv < 0 ) {
		    PORT_Free(savedest);
		    return(SECFailure);
		}
		curcount = 0;
		destptr = savedest;
	    }
	}
    }
    
    rv = (*(stream)->put_block)(stream->data_object, savedest, curcount);

    PORT_Free(savedest);
    if ( rv < 0 ) {
	return(SECFailure);
    } else {
	return(SECSuccess);
    }
}

SECStatus
XP_PutDialogStringsToStream(NET_StreamClass *stream, XPDialogStrings *strs,
			    PRBool quote)
{
    SECStatus rv;
    char *src, *token, *junk;
    int argnum;
    void *proto_win = stream->window_id;    
    src = strs->contents;
    
    while ((token = PORT_Strchr(src, '%'))) {
	*token = 0;
	rv = putStringToStream(stream, src, quote);
	if (rv) {
	    return(SECFailure);
	}
	*token = '%';		/* non-destructive... */
	token++;		/* now points to start of token */
	src = PORT_Strchr(token, '%');
	*src = 0;
	
	argnum = (int)XP_STRTOUL(token, &junk, 10);
	if (junk != token) {	/* A number */
	    PORT_Assert(argnum <= strs->nargs);
	    if (strs->args[argnum]) {
		rv = putStringToStream(stream, strs->args[argnum],
				       quote);
		if (rv) {
 		    return(SECFailure);
		}
	    }
	}
	else if (!PORT_Strcmp(token, "-cont-")) /* continuation */
	    /* Do Nothing */;
	else if (*token == 0) {	/* %% -> % */
	    rv = putStringToStream(stream, "%", quote);
	    if (rv) {
 		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "cancel")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_CANCEL),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "ok")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_OK),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "continue")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_CONTINUE),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "next")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_NEXT),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "back")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_BACK),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "finished")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_FINISHED),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "moreinfo")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_MOREINFO),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "showcert")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_SHOWCERT),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "showorder")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_SHOWORDER),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "showdocinfo")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_SHOWDOCINFO),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "renew")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_RENEW),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "new")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_NEW),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "reload")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_RELOAD),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "delete")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_DELETE),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "edit")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_EDIT),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "login")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_LOGIN),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "logout")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_LOGOUT),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "setpassword")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_SETPASSWORD),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "fetch")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_FETCH),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "banner-sec")) {
	    rv = putStringToStream(stream,
				   "<table width=100%><tr><td bgcolor=#0000ff><img align=left src=about:security?banner-secure></td></tr></table><br>",
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}
	else if (!PORT_Strcmp(token, "saveas")) {
	    rv = putStringToStream(stream,
				   XP_GetString(XP_SEC_SAVEAS),
				   quote);
	    if (rv) {
		return(SECFailure);
	    }
	}

	else {
	    PORT_Assert(0);	/* Failure: Unhandled token. */
	}
	
	*src = '%';		/* non-destructive... */
	src++;			/* now points past %..% */
    }
    
    rv = putStringToStream(stream, src, quote);
    if (rv) {
 	return(SECFailure);
    }
    
    return(SECSuccess);
}

SECStatus
XP_PutDialogStringsTagToStream(NET_StreamClass *stream, int num, PRBool quote)
{
    SECStatus rv;
    XPDialogStrings *strings;
    
    strings = XP_GetDialogStrings(num);
    if ( strings == NULL ) {
	return(SECFailure);
    }
    
    rv = XP_PutDialogStringsToStream(stream, strings, quote);
    
    XP_FreeDialogStrings(strings);
    
    return(rv);
}

char **
cgi_ConvertStringToArgVec(char *str, int length, int *argcp)
{
    char *cp, **avp;
    int argc;
    char c;
    char tmp;
    char *destp;
    
    char **av = 0;
    if (!str) {
	if (argcp) *argcp = 0;
	return 0;
    }

    cp = str + length - 1;
    while ( (*cp == '\n') || (*cp == '\r') ) {
	*cp-- = '\0';
    }
    
    /* 1. Count number of args in input */
    argc = 1;
    cp = str;
    for (;;) {
	cp = PORT_Strchr(cp, '=');
	if (!cp) break;
	cp++;
	argc++;
	cp = PORT_Strchr(cp, '&');
	if (!cp) break;
	cp++;
	argc++;
    }

    /* 2. Allocate space for argvec array and copy of strings */
    *argcp = argc;
    av = (char**) PORT_ZAlloc(((argc + 1) * sizeof(char*)) + strlen(str) + 1);
    destp = ((char *)av) + ( (argc + 1) * sizeof(char *) );
    
    if (!av) goto loser;

    /*
    ** 3. Break up input into each arg chunk. Once we break an entry up,
    ** compress it in place, replacing the magic characters with their
    ** correct value.
    */
    avp = av;
    cp = str;
    *avp++ = destp;

    while( ( c = (*cp++) ) != '\0' ) {
	switch( c ) {
	  case '&':
	  case '=':
	    *destp++ = '\0';
	    *avp++ = destp;
	    break;
	  case '+':
	    *destp++ = ' ';
	    break;
	  case '%':
	    c = *cp++;
	    if ((c >= 'a') && (c <= 'f')) {
		tmp = c - 'a' + 10;
	    } else if ((c >= 'A') && (c <= 'F')) {
		tmp = c - 'A' + 10;
	    } else {
		tmp = c - '0';
	    }
	    tmp = tmp << 4;
	    c = *cp++;
	    if ((c >= 'a') && (c <= 'f')) {
		tmp |= ( c - 'a' + 10 );
	    } else if ((c >= 'A') && (c <= 'F')) {
		tmp |= ( c - 'A' + 10 );
	    } else {
		tmp |= ( c - '0' );
	    }
	    *destp++ = tmp;
	    break;
	  default:
	    *destp++ = c;
	    break;
	}
	*destp = '\0';
    }
    return av;

  loser:
    PORT_Free(av);
    return 0;
}

/*
 * return the value of a name value pair
 */
char *
XP_FindValueInArgs(const char *name, char **av, int ac)
{
    for( ;ac > 0; ac -= 2, av += 2 ) {
	if ( PORT_Strcmp(name, av[0]) == 0 ) {
	    return(av[1]);
	}
    }
    return(0);
}

void
XP_HandleHTMLDialog(URL_Struct *url)
{
    char **av = NULL;
    int ac;
    char *handleString;
    char *buttonString;
    XPDialogState *handle = NULL;
    unsigned int button;
    PRBool ret;
    
    /* collect post data */
    av = cgi_ConvertStringToArgVec(url->post_data, url->post_data_size, &ac);
    if ( av == NULL ) {
	goto loser;
    }
    
    /* get the handle */
    handleString = XP_FindValueInArgs("handle", av, ac);
    if ( handleString == NULL ) {
	goto loser;
    }

    /* get the button value */
    buttonString = XP_FindValueInArgs("button", av, ac);
    if ( buttonString == NULL ) {
	goto loser;
    }

    handle = NULL;
#if defined(__sun) && !defined(SVR4) /* sun 4.1.3 */
    sscanf(handleString, "%lu", &handle);
#else
    sscanf(handleString, "%p", &handle);
#endif
    if ( handle == NULL ) {
	goto loser;
    }
    
    if ( PORT_Strcasecmp(buttonString, XP_GetString(XP_SEC_OK)) == 0 ) {
	button = XP_DIALOG_OK_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_CANCEL)) == 0 ) {
	button = XP_DIALOG_CANCEL_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_CONTINUE)) == 0 ) {
	button = XP_DIALOG_CONTINUE_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_MOREINFO)) == 0 ) {
	button = XP_DIALOG_MOREINFO_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_FETCH)) == 0 ) {
	button = XP_DIALOG_FETCH_BUTTON;
    } else {
	button = 0;
    }

    /* call the application handler */
    ret = PR_FALSE;
    if ( handle->dialogInfo->handler != NULL ) {
	ret = (* handle->dialogInfo->handler)(handle, av, ac, button);
    }

loser:

    /* free arg vector */
    if ( av != NULL ) {
	PORT_Free(av);
    }

    if ( ( handle != NULL ) && ( ret == PR_FALSE ) ) {
	deleteWindowCBArg *delstate;
	
	/* set callback to destroy the window */
	delstate = (deleteWindowCBArg *)PORT_Alloc(sizeof(deleteWindowCBArg));
	if ( delstate ) {
	    delstate->window = (void *)handle->window;
	    delstate->arg = handle->cbarg;
	    delstate->deleteWinCallback = handle->deleteCallback;
#if defined(XP_MAC) || defined(XP_UNIX)
	    (void)FE_SetTimeout(deleteWindow, (void *)delstate, 0);
#else
	    NET_AddExitCallback(deleteWindow, (void *)delstate);
#endif	
	}
	
	/* free dialog data */
	PORT_FreeArena(handle->arena, PR_FALSE);
    }
    
    return;
}

static PRBool html_dialog_show_scrollbar;

void
XP_MakeHTMLDialog(void *proto_win, XPDialogInfo *dialogInfo, 
		  int titlenum, XPDialogStrings *strings, void *arg)
{
    Chrome chrome;

    PORT_Memset(&chrome, 0, sizeof(chrome));
    chrome.type = MWContextDialog;
    chrome.w_hint = dialogInfo->width;
    chrome.h_hint = dialogInfo->height;
    chrome.is_modal = TRUE;
    chrome.show_scrollbar = html_dialog_show_scrollbar;
    
    /* Try to make the dialog fit on the screen, centered over the proto_win */
    if (proto_win && chrome.w_hint && chrome.h_hint) {
        int32 screenWidth = 0;
        int32 screenHeight = 0;
        
        Chrome protoChrome;
    
        PORT_Memset(&protoChrome, 0, sizeof(protoChrome));
        
#ifndef AKBAR
        /* We want to shrink the dialog to fit on the screen if necessary.
         *
         * The inner dimensions of the dialog have to be specified in the Chrome.
         * However, we want to OUTER dimensions to fit on the screen.
         * We don't know the difference between the inner and outer dimensions.
         * We could try to find that difference information from the proto_win's
         * Chrome structure, but the proto_win probably has a completely different
         * structure from our dialog windows. So we just guess...
         */
        FE_GetScreenSize ((MWContext *) proto_win, &screenWidth, &screenHeight);
        
        if (screenWidth && screenHeight) {
            if (chrome.w_hint > screenWidth - 20)   /* arbitrary slop around dialog */
                chrome.w_hint = screenWidth - 20;
            if (chrome.h_hint > screenHeight - 30)
                chrome.h_hint = screenHeight - 30;
        }

        /* Now, try to find out where the proto_win is so we can center on it. */
        FE_QueryChrome((MWContext *)proto_win, &protoChrome);
        
        /* Did we get anything useful? */
        if (protoChrome.location_is_chrome && protoChrome.w_hint && protoChrome.h_hint) {
            chrome.location_is_chrome = TRUE;
            chrome.l_hint = protoChrome.l_hint +
                            (protoChrome.w_hint - chrome.w_hint) / 2;
            chrome.t_hint = protoChrome.t_hint +
                            (protoChrome.h_hint - chrome.h_hint) / 2;
        }
        
        /* If we moved the dialog, make sure we didn't move it off the screen. */
        if (chrome.location_is_chrome && screenWidth && screenHeight) {
            if (chrome.l_hint < 0)
                chrome.l_hint = 0;
            if (chrome.t_hint < 0)
                chrome.t_hint = 0;
            if (chrome.l_hint > screenWidth - chrome.w_hint)
                chrome.l_hint = screenWidth - chrome.w_hint;
            if (chrome.t_hint > screenHeight - chrome.h_hint)
                chrome.t_hint = screenHeight - chrome.h_hint;
        }
#endif /* AKBAR */        
    }
    
    XP_MakeHTMLDialogWithChrome (proto_win, dialogInfo, titlenum, 
				 strings, &chrome, arg);
}

void
XP_MakeHTMLDialogWithChrome(void *proto_win, XPDialogInfo *dialogInfo,
			    int titlenum, XPDialogStrings *strings,
			    Chrome *chrome, void *arg)
{
    MWContext *cx;
    NET_StreamClass *stream = NULL;
    SECStatus rv;
    XPDialogState *state;
    PRArenaPool *arena = NULL;
    char buf[50];
    XPDialogStrings *dlgstrings;
    int buttontag;
    URL_Struct *url;
    
    arena = PORT_NewArena(1024);
    
    if ( arena == NULL ) {
	return;
    }

    /* allocate the state structure */
    state = (XPDialogState *)PORT_ArenaAlloc(arena, sizeof(XPDialogState));
    if ( state == NULL ) {
	goto loser;
    }
    
    state->arena = arena;
    state->dialogInfo = dialogInfo;
    state->arg = arg;
    state->deleteCallback = NULL;
    state->cbarg = NULL;
    
    /* make the window */
    cx = FE_MakeNewWindow((MWContext *)proto_win, NULL, NULL, chrome);
    if ( cx == NULL ) {
	goto loser;
    }
#ifndef AKBAR
    LM_ForceJSEnabled(cx);
#endif /* AKBAR */
    
    state->window = (void *)cx;
    state->proto_win = proto_win;

    /* XXX - get rid of session history */
    SHIST_EndSession(cx);
    PORT_Memset((void *)&cx->hist, 0, sizeof(cx->hist));
    
    /* create a URL struct */
    url = NET_CreateURLStruct("", NET_DONT_RELOAD);
    StrAllocCopy(url->content_type, TEXT_HTML);

    /* make a stream to display in the window */
    stream = NET_StreamBuilder(FO_PRESENT, url, cx);

    dlgstrings = XP_GetDialogStrings(XP_DIALOG_JS_HEADER_STRINGS);
    if ( dlgstrings == NULL ) {
	goto loser;
    }

    /* put the title in header */
    XP_CopyDialogString(dlgstrings, 0, XP_GetString(titlenum));

    /* send html header stuff */
    rv = XP_PutDialogStringsToStream(stream, dlgstrings, PR_FALSE);
    XP_FreeDialogStrings(dlgstrings);
    
    if ( rv != SECSuccess ) {
	goto loser;
    }

    dlgstrings = XP_GetDialogStrings(XP_DIALOG_HEADER_STRINGS);
    if ( dlgstrings == NULL ) {
	goto loser;
    }

    /* put handle in header */
#if defined(__sun) && !defined(SVR4) /* sun 4.1.3 */
    sprintf(buf, "%lu", state);
#else
    sprintf(buf, "%p", state);
#endif
    XP_SetDialogString(dlgstrings, 0, buf);
    
    /* send html header stuff */
    rv = XP_PutDialogStringsToStream(stream, dlgstrings, PR_TRUE);
    XP_FreeDialogStrings(dlgstrings);
    
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* send caller's message */
    rv = XP_PutDialogStringsToStream(stream, strings, PR_TRUE);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* send trailing info */
    rv = XP_PutDialogStringsTagToStream(stream, XP_DIALOG_FOOTER_STRINGS,
					PR_TRUE);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* send button strings */
    switch(dialogInfo->buttonFlags) {
      case XP_DIALOG_CANCEL_BUTTON:
	buttontag = XP_DIALOG_CANCEL_BUTTON_STRINGS;
	break;
      case XP_DIALOG_CONTINUE_BUTTON:
	buttontag = XP_DIALOG_CONTINUE_BUTTON_STRINGS;
	break;
      case XP_DIALOG_OK_BUTTON:
	buttontag = XP_DIALOG_OK_BUTTON_STRINGS;
	break;
      case XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_OK_BUTTON:
	buttontag = XP_DIALOG_CANCEL_OK_BUTTON_STRINGS;
	break;
      case XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_CONTINUE_BUTTON:
	buttontag = XP_DIALOG_CANCEL_CONTINUE_BUTTON_STRINGS;
	break;
      case XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_OK_BUTTON |
	  XP_DIALOG_MOREINFO_BUTTON:
	buttontag = XP_DIALOG_CANCEL_OK_MOREINFO_BUTTON_STRINGS;
	break;
      case XP_DIALOG_CANCEL_BUTTON | XP_DIALOG_FETCH_BUTTON:
	buttontag = XP_DIALOG_FETCH_CANCEL_BUTTON_STRINGS;
	break;
      default:
	buttontag = XP_DIALOG_NULL_STRINGS;
	break;
    }

    rv = XP_PutDialogStringsTagToStream(stream, XP_DIALOG_JS_MIDDLE_STRINGS,
					PR_FALSE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = XP_PutDialogStringsTagToStream(stream, buttontag, PR_TRUE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

#ifdef AKBAR
    {
      const char *str = ("';\n"
			 "var Event = new Object;\n"
			 "Event.MOUSEDOWN = null;\n"
			 "function captureEvents(x) {};\n"
			 "var akbar_kludge = '"
			 );
      rv = putStringToStream(stream, (char *) str, PR_FALSE);
      if ( rv != SECSuccess ) {
	goto loser;
      }
    }
#endif /* AKBAR */

    rv = XP_PutDialogStringsTagToStream(stream, XP_DIALOG_JS_FOOTER_STRINGS,
					PR_FALSE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* complete the stream */
    (*stream->complete) (stream->data_object);
    PORT_Free(stream);
    NET_FreeURLStruct(url);
    return;
    
loser:    
    /* XXX free URL and CX ???*/

    /* abort the stream */
    if ( stream != NULL ) {
	(*stream->abort) (stream->data_object, rv);
	PORT_Free(stream);
    }
    
    if (url != NULL)
	NET_FreeURLStruct(url);

    /* free the arena */
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return;
}

int
XP_RedrawRawHTMLDialog(XPDialogState *state,
		       XPDialogStrings *strings,
		       int handlestring)
{
    NET_StreamClass *stream = NULL;
    SECStatus rv;
    char buf[50];
    URL_Struct *url;

    /* create a URL struct */
    url = NET_CreateURLStruct("", NET_DONT_RELOAD);
    StrAllocCopy(url->content_type, TEXT_HTML);

    /* make a stream to display in the window */
    stream = NET_StreamBuilder(FO_PRESENT, url, state->window);
 
    /* put handle in header */
#if defined(__sun) && !defined(SVR4) /* sun 4.1.3 */
    sprintf(buf, "%lu", state);
#else
    sprintf(buf, "%p", state);
#endif
    XP_SetDialogString(strings, handlestring, buf);
    
    /* send caller's message */
    rv = XP_PutDialogStringsToStream(stream, strings, PR_FALSE);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* complete the stream */
    (*stream->complete) (stream->data_object);
    PORT_Free(stream);
    NET_FreeURLStruct(url);
    return((int)SECSuccess);
    
loser:    
    /* XXX free URL and CX ???*/

    /* abort the stream */
    if ( stream != NULL ) {
	(*stream->abort) (stream->data_object, rv);
	PORT_Free(stream);
    }
    
    if (url != NULL)
	NET_FreeURLStruct(url);

    return((int)SECFailure);
}

XPDialogState *
XP_MakeRawHTMLDialog(void *proto_win, XPDialogInfo *dialogInfo,
		     int titlenum, XPDialogStrings *strings,
		     int handlestring, void *arg)
{
    MWContext *cx = NULL;
    SECStatus rv;
    XPDialogState *state;
    PRArenaPool *arena = NULL;
    Chrome chrome;
    PORT_Memset(&chrome, 0, sizeof(chrome));
    chrome.type = MWContextDialog;
    chrome.w_hint = dialogInfo->width;
    chrome.h_hint = dialogInfo->height;

#ifdef AKBAR
    /* Akbar freaks out and walks off the end of an array in Xt if there are
       two modal dialogs on the screen at the same time, as happens when you
       bring up the security advisor; bring up a certificate dialog on top of
       it; dismiss the cert dialog; then dismiss the SA.  Boom.

       The problem goes away if the SA is made non-modal.
     */
    chrome.is_modal = FALSE;
#else  /* !AKBAR */
    chrome.is_modal = TRUE;
#endif /* !AKBAR */

    chrome.show_scrollbar = html_dialog_show_scrollbar;
    
    arena = PORT_NewArena(1024);
    
    if ( arena == NULL ) {
	return(NULL);
    }

    /* allocate the state structure */
    state = (XPDialogState *)PORT_ArenaAlloc(arena, sizeof(XPDialogState));
    if ( state == NULL ) {
	goto loser;
    }
    
    state->arena = arena;
    state->dialogInfo = dialogInfo;
    state->arg = arg;
    state->deleteCallback = NULL;
    state->cbarg = NULL;
    
    /* make the window */
    cx = FE_MakeNewWindow((MWContext *)proto_win, NULL, NULL, &chrome);
    if ( cx == NULL ) {
	goto loser;
    }
#ifndef AKBAR
    LM_ForceJSEnabled(cx);
#endif /* AKBAR */
    
    state->window = (void *)cx;
    state->proto_win = proto_win;

    /* XXX - get rid of session history */
    SHIST_EndSession(cx);
    PORT_Memset((void *)&cx->hist, 0, sizeof(cx->hist));

    rv = (SECStatus)XP_RedrawRawHTMLDialog(state, strings, handlestring);

    if ( rv != SECSuccess ) {
	goto loser;
    }
    return(state);

loser:
    /* free the arena */
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

static void
displayPanelCB(void *arg)
{
    XPPanelState *state;
    URL_Struct *url;
    NET_StreamClass *stream;
    XPDialogStrings *contentstrings;
    SECStatus rv;
    char buf[50];
    XPDialogStrings *dlgstrings;
    int buttontag;
    
    state = (XPPanelState *)arg;
    
    /* create a URL struct */
    url = NET_CreateURLStruct("", NET_DONT_RELOAD);
    if (url == NULL)
	goto loser;
    StrAllocCopy(url->content_type, TEXT_HTML);
    
    /* make a stream to display in the window */
    stream = NET_StreamBuilder(FO_PRESENT, url, (MWContext *)state->window);
    if (stream == NULL) {
	goto loser;
    }
    
    dlgstrings = XP_GetDialogStrings(XP_DIALOG_JS_HEADER_STRINGS);
    if ( dlgstrings == NULL ) {
	goto loser;
    }

    /* put the title in header */
    XP_CopyDialogString(dlgstrings, 0, XP_GetString(state->titlenum));

    /* send html header stuff */
    rv = XP_PutDialogStringsToStream(stream, dlgstrings, PR_FALSE);
    XP_FreeDialogStrings(dlgstrings);
    
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* get header strings */
    dlgstrings = XP_GetDialogStrings(XP_PANEL_HEADER_STRINGS);
    if ( dlgstrings == NULL ) {
	goto loser;
    }

    /* put handle in header */
#if defined(__sun) && !defined(SVR4) /* sun 4.1.3 */
    sprintf(buf, "%lu", state);
#else
    sprintf(buf, "%p", state);
#endif
    XP_SetDialogString(dlgstrings, 0, buf);
    
    /* send html header stuff */
    rv = XP_PutDialogStringsToStream(stream, dlgstrings, PR_TRUE);

    /* free the strings */
    XP_FreeDialogStrings(dlgstrings);
    
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* send panel message */
    contentstrings = (* state->panels[state->curPanel].content)(state);
    if ( contentstrings == NULL ) {
	goto loser;
    }
    
    state->curStrings = contentstrings;
    
    rv = XP_PutDialogStringsToStream(stream, contentstrings, PR_TRUE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    XP_FreeDialogStrings(contentstrings);

    /* send html middle stuff */
    rv = XP_PutDialogStringsTagToStream(stream, XP_DIALOG_FOOTER_STRINGS,
					PR_TRUE);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    if ( state->panels[state->curPanel].flags & XP_PANEL_FLAG_ONLY ) {
	/* if its the only panel */
	buttontag = XP_PANEL_ONLY_BUTTON_STRINGS;
    } else if ( ( state->curPanel == ( state->panelCount - 1 ) ) ||
	( state->panels[state->curPanel].flags & XP_PANEL_FLAG_FINAL ) ) {
	/* if its the last panel or has the final flag set */
	buttontag = XP_PANEL_LAST_BUTTON_STRINGS;
    } else if ( ( state->curPanel == 0 ) ||
	( state->panels[state->curPanel].flags & XP_PANEL_FLAG_FIRST ) ) {
	buttontag = XP_PANEL_FIRST_BUTTON_STRINGS;
    } else {
	buttontag = XP_PANEL_MIDDLE_BUTTON_STRINGS;
    }
    
    rv = XP_PutDialogStringsTagToStream(stream, XP_DIALOG_JS_MIDDLE_STRINGS,
					PR_FALSE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* send button strings */
    rv = XP_PutDialogStringsTagToStream(stream, buttontag, PR_TRUE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

#ifdef AKBAR
    {
      const char *str = ("';\n"
			 "var Event = new Object;\n"
			 "Event.MOUSEDOWN = null;\n"
			 "function captureEvents(x) {};\n"
			 "var akbar_kludge = '"
			 );
      rv = putStringToStream(stream, (char *) str, PR_FALSE);
      if ( rv != SECSuccess ) {
	goto loser;
      }
    }
#endif /* AKBAR */

    rv = XP_PutDialogStringsTagToStream(stream, XP_DIALOG_JS_FOOTER_STRINGS,
					PR_FALSE);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    /* complete the stream */
    (*stream->complete) (stream->data_object);
    PORT_Free(stream);
    NET_FreeURLStruct(url);
    return;

loser:    
    /* XXX free URL???*/

    /* abort the stream */
    if ( stream != NULL ) {
	(*stream->abort) (stream->data_object, rv);
	PORT_Free(stream);
    }
    if (url != NULL)
	NET_FreeURLStruct(url);
    return;
}

static void
displayPanel(XPPanelState *state)
{
    (void)FE_SetTimeout(displayPanelCB, (void *)state, 0);
}

void
XP_MakeHTMLPanel(void *proto_win, XPPanelInfo *panelInfo,
		 int titlenum, void *arg)
{
    PRArenaPool *arena;
    XPPanelState *state = NULL;
    Chrome chrome;
    MWContext *cx;
    
    arena = PORT_NewArena(1024);
    if ( arena == NULL ) {
	return;
    }

    state = (XPPanelState *)PORT_ArenaAlloc(arena, sizeof(XPPanelState));
    if ( state == NULL ) {
	return;
    }
    
    state->arena = arena;
    state->panels = panelInfo->desc;
    state->panelCount = panelInfo->panelCount;
    state->curPanel = 0;
    state->arg = arg;
    state->finish = panelInfo->finishfunc;
    state->info = panelInfo;
    state->titlenum = titlenum;
    state->deleteCallback = NULL;
    state->cbarg = NULL;
    
    /* make the window */
    PORT_Memset(&chrome, 0, sizeof(chrome));
    chrome.type = MWContextDialog;
    chrome.w_hint = panelInfo->width;
    chrome.h_hint = panelInfo->height;
    chrome.is_modal = TRUE;
    cx = FE_MakeNewWindow((MWContext *)proto_win, NULL, NULL, &chrome);
    state->window = (void *)cx;
    
    if ( state->window == NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
	return;
    }
#ifndef AKBAR
    LM_ForceJSEnabled(cx);
#endif /* AKBAR */

    /* XXX - get rid of session history */
    SHIST_EndSession(cx);
    PORT_Memset((void *)&cx->hist, 0, sizeof(cx->hist));
    
    /* display the first panel */
    displayPanel(state);
    
    return;
}

void
XP_HandleHTMLPanel(URL_Struct *url)
{
    char **av = NULL;
    int ac;
    char *handleString;
    char *buttonString;
    XPPanelState *state = NULL;
    unsigned int button;
    int nextpanel;
    
    /* collect post data */
    av = cgi_ConvertStringToArgVec(url->post_data, url->post_data_size, &ac);
    if ( av == NULL ) {
	goto loser;
    }
    
    /* get the handle */
    handleString = XP_FindValueInArgs("handle", av, ac);
    if ( handleString == NULL ) {
	goto loser;
    }

    /* get the button value */
    buttonString = XP_FindValueInArgs("button", av, ac);
    if ( buttonString == NULL ) {
	goto loser;
    }

    /* extract a handle pointer from the string */
    state = NULL;
#if defined(__sun) && !defined(SVR4) /* sun 4.1.3 */
    sscanf(handleString, "%lu", &state);
#else
    sscanf(handleString, "%p", &state);
#endif
    if ( state == NULL ) {
	goto loser;
    }
    
    /* figure out which button was pressed */
    if ( PORT_Strcasecmp(buttonString,
		       XP_GetString(XP_SEC_NEXT_KLUDGE)) == 0 ) {
	button = XP_DIALOG_NEXT_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_CANCEL)) == 0 ) {
	button = XP_DIALOG_CANCEL_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_BACK_KLUDGE)) == 0 ) {
	button = XP_DIALOG_BACK_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_FINISHED)) == 0 ) {
	button = XP_DIALOG_FINISHED_BUTTON;
    } else if ( PORT_Strcasecmp(buttonString,
			      XP_GetString(XP_SEC_MOREINFO)) == 0 ) {
	button = XP_DIALOG_MOREINFO_BUTTON;
    } else {
	button = 0;
    }

    /* call the application handler */
    if ( state->panels[state->curPanel].handler != NULL ) {
	nextpanel = (* state->panels[state->curPanel].handler)(state, av, ac,
							       button);
    } else {
	nextpanel = 0;
    }

    if ( button == XP_DIALOG_CANCEL_BUTTON ) {
	if ( state->finish ) {
	    (* state->finish)(state, PR_TRUE);
	}
	goto done;
    }
    
    if ( nextpanel != 0 ) {
	state->curPanel = nextpanel - 1;
    } else {
	switch ( button ) {
	  case XP_DIALOG_BACK_BUTTON:
	    PORT_Assert(state->curPanel > 0);
	    state->curPanel = state->curPanel - 1;
	    break;
	  case XP_DIALOG_NEXT_BUTTON:
	    PORT_Assert(state->curPanel < ( state->panelCount - 1 ) );
	    state->curPanel = state->curPanel + 1;
	    break;
	  case XP_DIALOG_FINISHED_BUTTON:
	    if ( state->finish ) {
		(* state->finish)(state, PR_FALSE);
	    }
	    goto done;
	}
    }
    
    displayPanel(state);

    /* free arg vector */
    PORT_Free(av);
    return;
    
loser:
done:

    /* free arg vector */
    if ( av != NULL ) {
	PORT_Free(av);
    }

    if ( state != NULL ) {
	/* destroy the window */
	deleteWindowCBArg *delstate;
	
	/* set callback to destroy the window */
	delstate = (deleteWindowCBArg *)PORT_Alloc(sizeof(deleteWindowCBArg));
	if ( delstate ) {
	    delstate->window = (void *)state->window;
	    delstate->arg = state->cbarg;
	    delstate->deleteWinCallback = state->deleteCallback;
#if defined(XP_MAC) || defined(XP_UNIX)
	    (void)FE_SetTimeout(deleteWindow, (void *)delstate, 0);
#else
	    NET_AddExitCallback(deleteWindow, (void *)delstate);
#endif
	}

/*	PORT_Free(handle->dialogInfo); */
	
	/* free dialog data */
	PORT_FreeArena(state->arena, PR_FALSE);
    }

    return;
}

/*
 * fetch a string from the dialog strings database
 */
XPDialogStrings *
XP_GetDialogStrings(int stringnum)
{
    XPDialogStrings *header = NULL;
    PRArenaPool *arena = NULL;
    char *dst, *src;
    int n, size, len, done = 0;
    
    /* get a new arena */
    arena = PORT_NewArena(XP_STRINGS_CHUNKSIZE);
    if ( arena == NULL ) {
	return(NULL);
    }

    /* allocate the header structure */
    header = (XPDialogStrings *)PORT_ArenaAlloc(arena, sizeof(XPDialogStrings));
    if ( header == NULL ) {
	goto loser;
    }

    /* init the header */
    header->arena = arena;
    header->basestringnum = stringnum;

    src = XP_GetString(stringnum);
    len = PORT_Strlen(src);
    size = len + 1;
    dst = header->contents =
	(char *)PORT_ArenaAlloc(arena, sizeof(char) * size);
    if (dst == NULL)
	goto loser;
    
    while (!done) {		/* Concatenate pieces to form message */
	PORT_Memcpy(dst, src, len+1);
	done = 1;
	if (XP_STRSTR(src, "%-cont-%")) { /* Continuation */
	    src = XP_GetString(++stringnum);
	    len = PORT_Strlen(src);
	    header->contents =
		(char *)PORT_ArenaGrow(arena,
				     header->contents, size, size + len);
	    if (header->contents == NULL)
		goto loser;
	    dst = header->contents + size - 1;
	    size += len;
	    done = 0;
	}
    }

    /* At this point we should have the complete message in
       header->contents, including like %-cont-%, which will be
       ignored later. */

    /* Count the arguments in the message */
    header->nargs = -1;		/* Support %0% as lowest token */
    src = header->contents;
    while ((src = PORT_Strchr(src, '%'))) {
	src++;
	n = (int)XP_STRTOUL(src, &dst, 10);
	if (dst == src)	{	/* Integer not found... */
	    src = PORT_Strchr(src, '%') + 1; /* so skip this %..% */
	    PORT_Assert(NULL != src-1); /* Unclosed %..% ? */
	    continue;
	}
	
	if (header->nargs < n)
	    header->nargs = n;
	src = dst + 1;
    }

    if (++(header->nargs) > 0)  /* Allocate space for arguments */
	header->args = (char **)PORT_ArenaZAlloc(arena, sizeof(char *) *
					      header->nargs);

    return(header);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(NULL);
}

/*
 * Set a dialog string to the given string.
 * The source string must be writable(not a static string), and will
 *  not be copied, so it is the responsibility of the caller to make
 *  sure it is freed after use.
 */
void
XP_SetDialogString(XPDialogStrings *strings, int argNum, char *string)
{
    /* make sure we are doing it right */
    PORT_Assert(argNum < strings->nargs);
    PORT_Assert(argNum >= 0);
    PORT_Assert(strings->args[argNum] == NULL);
    
    /* set the string */
    strings->args[argNum] = string;
    
    return;
}

/*
 * Copy a string to the dialog string
 */
void
XP_CopyDialogString(XPDialogStrings *strings, int argNum, char *string)
{
    int len;
    
    /* make sure we are doing it right */
    PORT_Assert(argNum < strings->nargs);
    PORT_Assert(argNum >= 0);
    PORT_Assert(strings->args[argNum] == NULL);
    
    /* copy the string */
    len = PORT_Strlen(string) + 1;
    strings->args[argNum] = (char *)PORT_ArenaAlloc(strings->arena, len);
    if ( strings->args[argNum] != NULL ) {
	PORT_Memcpy(strings->args[argNum], string, len);
    }
    
    return;
}

/*
 * free the dialog strings
 */
void
XP_FreeDialogStrings(XPDialogStrings *strings)
{
    PORT_FreeArena(strings->arena, PR_FALSE);
    
    return;
}

static XPDialogInfo alertDialog = {
    XP_DIALOG_OK_BUTTON,
    NULL,
    600,
    224
};

void
XP_MakeHTMLAlert(void *proto_win, char *string)
{
    XPDialogStrings *strings;
    
    /* get empty strings */
    strings = XP_GetDialogStrings(XP_EMPTY_STRINGS);
    if ( strings == NULL ) {
	return;
    }
    
    XP_CopyDialogString(strings, 0, string);

    /* put up the dialog */
    html_dialog_show_scrollbar = TRUE;
    XP_MakeHTMLDialog(proto_win, &alertDialog, XP_ALERT_TITLE_STRING,
		      strings, NULL);
    html_dialog_show_scrollbar = FALSE;

    return;
}
