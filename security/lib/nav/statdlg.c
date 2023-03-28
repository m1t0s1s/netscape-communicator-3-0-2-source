/* -*- Mode: C; tab-width: 8 -*-
 * Security Status dialog for Mozilla
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: statdlg.c,v 1.2 1997/04/28 18:46:03 jsw Exp $
 */

#include "seccomon.h"
#include "secport.h"
#include "htmldlgs.h"
#include "libevent.h"
#include "statdlg.h"

extern int XP_SECURITY_ADVISOR_TITLE_STRING;
extern int XP_STATUS_DIALOG_STRINGS;

static char statDialogHeaderString[] =
"<head>\n"
"<script language=javascript>\n"
"var handle='";

static char statDialogMiddleString[] =
"';\n"
"var initstr = '%s';\n";

static char statDialogFooterString1[] =
"var msh='<html><body bgcolor=\"#c0c0c0\"><p>';\n"
"var msf='</body></html>';\n"
"var bs='<html><body bgcolor=\"#c0c0c0\"><p><form name=buttonform ACTION=\"internal-dialog-handler\" METHOD=POST><center>' +\n"
" '<input type=hidden name=handle value=' + handle + '>' +\n"
" '<input type=hidden name=xxxbuttonxxx>' +\n"
" '<input type=submit name=button value=\"Cancel\">' +\n"
" '</center></form></body></html>';\n"
"var tsh = '<html><head>' +\n"
" '<STYLE TYPE=\"text/css\">BODY{' +\n"
" 'font-family: Geneva,MS Sans Serif,Arial,Lucida,Helvetica,sans-serif;' +\n"
" 'font-size: 10pt;}</STYLE><body bgcolor=\"#c0c0c0\">&nbsp<br><center>';\n"
"var tsf = '</font></center></body></html>';\n"
"var ms;\n"
"var offset = 0;\n"
"var delta = 1;\n"
"var ts;\n"
"var dialogUp = false;\n"
"function drawstring(s) {\n"
"    ts = tsh + s + tsf;\n"
"    with ( window.frames[0] ) {\n"
"	document.write(ts);\n"
"	document.close();\n"
"    }\n"
"}\n"
"function drawdlg(win) {\n"
"    var i;\n"
"    var mystr;\n"
"    var vis;\n"
"    mystr = '<center><table border=0 cellspacing=1 cellpadding=2><tr>';\n"
"    for ( i = 0; i < 40; i++ ) {\n"
"	if ( i < 20 ) {\n"
"	    vis = '';\n"
"	} else {\n"
"	    vis = ' visibility=hide';\n"
"	}\n"
"	mystr += '<td><ilayer width=3 height=12 bgcolor=\"#0000ff\" ' +\n"
"	    'name=\"layer' + i + '\"' + vis + '></ilayer></td>';\n"
"    }\n"
"    mystr += '</tr></table></center>';\n"
"    mystr += '<script>parent.dialogUp = true;<\\/script>';\n"
"    with(win.frames[2]) {\n"
"	document.write(bs);\n"
"	document.close();\n"
"    }\n"
"    drawstring(initstr);\n"
"    with(win.frames[1]) {\n"
"	ms = msh + mystr + msf;\n"
"	document.write(ms);\n"
"	document.close();\n"
"    }\n"
"    setTimeout('shiftit()', 100);\n"
"    return false;\n"
"}\n";

static char statDialogFooterString2[] =
"function shiftit()\n"
"{\n"
"    if ( dialogUp == false ) {\n"
"	setTimeout('shiftit()', 100);\n"
"	return;\n"
"    }\n"
"    with ( window.frames[1].document ) {\n"
"	if ( delta == 1 ) {\n"
"	    layers['layer' + offset].visibility = 'hide';\n"
"	    layers['layer' + ( offset + 20 ) ].visibility = 'show';\n"
"	} else {\n"
"	    layers['layer' + offset].visibility = 'show';\n"
"	    layers['layer' + ( offset + 20 ) ].visibility = 'hide';\n"
"	}\n"
"	offset = offset + delta;\n"
"	if ( offset == 19 ) {\n"
"	    delta = -1;\n"
"	}\n"
"	if ( offset == 0 ) {\n"
"	    delta = 1;\n"
"	}\n"
"	setTimeout('shiftit()', 100);\n"
"    }\n"
"}\n"
"var closed=false;\n"
"function closeit()\n"
"{\n"
"    if ( closed == false ) {\n"
"	closed = true;\n"
"	window.buttonframe.document.buttonform.xxxbuttonxxx.value ='Cancel';\n"
"	window.buttonframe.document.buttonform.xxxbuttonxxx.name = 'button';\n"
"	window.buttonframe.document.buttonform.submit();\n"
"    }\n"
"}\n"
"</SCRIPT></HEAD>\n"
"<FRAMESET ROWS=\"50,*,75\" ONLOAD=\"drawdlg(window)\" BORDER=0>\n"
"<FRAME SRC=\"about:blank\" MARGINWIDTH=0 MARGINHEIGHT=0 NORESIZE BORDER=NO>\n"
"<FRAME SRC=\"about:blank\" MARGINWIDTH=0 MARGINHEIGHT=0 NORESIZE BORDER=NO>\n"
"<FRAME SRC=\"about:blank\" NAME=buttonframe MARGINWIDTH=0 MARGINHEIGHT=0 NORESIZE BORDER=NO>\n"
"</FRAMESET></HTML>\n";

static PRBool
statusDialogHandler(XPDialogState *dlgstate, char **argv, int argc,
		    unsigned int button)
{
    SECStatusDialog *dlg;
    
    dlg = (SECStatusDialog *)dlgstate->arg;
    
    dlg->dialogUp = PR_FALSE;
    
    return(PR_FALSE);
}

static XPDialogInfo statusDialog = {
    0,
    statusDialogHandler,
    350, 200
};

#define CLOSE_WINDOW_STRING "closeit()"
#define CLOSE_WINDOW_STRING_LEN (sizeof(CLOSE_WINDOW_STRING)-1)

void
SECNAV_CloseStatusDialog(SECStatusDialog *dlg)
{
    ET_EvaluateBuffer((MWContext *)dlg->window, CLOSE_WINDOW_STRING,
		      CLOSE_WINDOW_STRING_LEN, 0, NULL, NULL, NULL,
		      JSVERSION_DEFAULT, NULL);
    
    return;
}

SECStatusDialog *
SECNAV_MakeStatusDialog(char *initialstr, void *window,
			SECNAVStatusCancelCallback cb,
			void *cbarg)
{
    XPDialogStrings *strings;
    SECStatusDialog *dlg;
    XPDialogState *state;
    char *midstr;
    
    strings = XP_GetDialogStrings(XP_STATUS_DIALOG_STRINGS);
    if ( strings == NULL ) {
	return(NULL);
    }
    
    midstr = PR_smprintf(statDialogMiddleString, initialstr);
    if ( midstr == NULL ) {
	goto loser;
    }
    
    XP_SetDialogString(strings, 0, statDialogHeaderString);
    XP_CopyDialogString(strings, 2, midstr);
    XP_SetDialogString(strings, 3, statDialogFooterString1);
    XP_SetDialogString(strings, 4, statDialogFooterString2);

    PORT_Free(midstr);
    
    dlg = PORT_ZAlloc(sizeof(SECStatusDialog));
    if ( dlg == NULL ) {
	goto loser;
    }

    dlg->parent_window = window;

    
    dlg->dialogUp = PR_TRUE;
    
    dlg->state = XP_MakeRawHTMLDialog(window, &statusDialog,
				      XP_SECURITY_ADVISOR_TITLE_STRING,
				      strings, 1, (void *)dlg);

    dlg->state->deleteCallback = cb;
    dlg->state->cbarg = cbarg;
    dlg->window = dlg->state->window;
    
    XP_FreeDialogStrings(strings);
    return(dlg);
loser:
    XP_FreeDialogStrings(strings);
    return(NULL);
}

