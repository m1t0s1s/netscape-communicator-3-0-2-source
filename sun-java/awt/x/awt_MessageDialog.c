/*
 * @(#)awt_MessageDialog.c	1.5 95/10/02 Sami Shaio
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#include "awt_p.h"
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>
#include <Xm/MessageB.h>
#include "awt_WServer.h"
#include "awt_Frame.h"
#include "awt_motif_MFramePeer.h"
#include "awt_MessageDialog.h"
#include "awt_motif_MMessageDialogPeer.h"

static void
MessageDialog_ok(Widget w,
		 XtPointer client_data,
		 XtPointer call_data)
{
    dialogButtonChosen = 1;

    JAVA_UPCALL((EE(),
		(HObject *)client_data,
		"okCallback",
		"(Lawt/Dialog;)V",
		(void *)client_data));
}


static void
MessageDialog_cancel(Widget w,
		     XtPointer client_data,
		     XtPointer call_data)
{
    dialogButtonChosen = 2;

    JAVA_UPCALL((EE(),
		(HObject *)client_data,
		"cancelCallback",
		"(Lawt/Dialog;)V",
		client_data));
}


static void
MessageDialog_help(Widget w,
		   XtPointer client_data,
		   XtPointer call_data)
{
    JAVA_UPCALL((EE(),
		(HObject *)client_data,
		"helpCallback",
		"(Lawt/Dialog;)V",
		client_data));
}

void
awt_motif_MMessageDialogPeer_create(struct Hawt_motif_MMessageDialogPeer *this,
				    struct Hawt_MessageDialog *peer,
				    struct Hawt_Frame *parent,
				    Hjava_lang_String *title,
				    Hjava_lang_String *message,
				    long dialogType,
				    long nButtons,
				    /*boolean*/ long isModal,
				    Hjava_lang_String *okLabel,
				    Hjava_lang_String *cancelLabel,
				    Hjava_lang_String *helpLabel)
{
    Arg				args[20];
    Cardinal			argc;
    struct MessageDialogData	*mdata = 0;
    struct WindowData		*fdata;
    XmString			mOk = 0;
    XmString			mCancel = 0;
    XmString			mHelp = 0;
    XmString			mMessage = 0;
    XmString			mTitle = 0;
    Pixel			wbg;

    if (parent == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    fdata = PEER_PDATA(WindowData,Hawt_motif_MFramePeer,parent);

    if (!fdata) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    wbg = awt_getColor(unhand(parent)->background);
    mdata = ZALLOC(MessageDialogData);
    SET_PDATA(this, mdata);
    mdata->isModal = isModal;
    if (!mdata) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    argc = 0;
    XtSetArg(args[argc], XmNbackground, wbg); argc++;
    XtSetArg(args[argc], XmNvisual, awt_visual); argc++;
    if (title != 0) {
	mTitle = XmStringCreateLocalized(makeCString(title));
	XtSetArg(args[argc], XmNdialogTitle, mTitle); argc++;
    }
    if (okLabel != 0) {
	mOk = XmStringCreateLocalized(makeCString(okLabel));
	XtSetArg(args[argc], XmNokLabelString, mOk); argc++;
    }
    if (nButtons > 1 && cancelLabel != 0) {
	mCancel = XmStringCreateLocalized(makeCString(cancelLabel));
	XtSetArg(args[argc], XmNcancelLabelString, mCancel); argc++;
    }
    if (nButtons > 2 && helpLabel != 0) {
	mHelp = XmStringCreateLocalized(makeCString(helpLabel));
	XtSetArg(args[argc], XmNhelpLabelString, mHelp); argc++;
    }
    if (message != 0) {
	mMessage = XmStringCreateLocalized(makeCString(message));
	XtSetArg(args[argc], XmNmessageString, mMessage); argc++;
    }
    if (isModal) {
	XtSetArg(args[argc], XmNdialogStyle, XmDIALOG_SYSTEM_MODAL); argc++;
    }
    switch (dialogType) {
      case awt_MessageDialog_ERROR_TYPE:
	mdata->comp.widget = XmCreateErrorDialog(fdata->shell,
					    "Error",
					    args,
					    argc);
	break;
      case awt_MessageDialog_QUESTION_TYPE:
	mdata->comp.widget = XmCreateQuestionDialog(fdata->shell,
					       "Question",
					       args,
					       argc);
	break;
      case awt_MessageDialog_INFO_TYPE:
      default:
	mdata->comp.widget = XmCreateInformationDialog(fdata->shell,
						       "Info", args, argc);
	break;
    }
    switch (nButtons) {
      case 1:
	XtUnmanageChild(XmMessageBoxGetChild(mdata->comp.widget,
					     XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(mdata->comp.widget,
					     XmDIALOG_HELP_BUTTON));
	break;
      case 2:
	XtUnmanageChild(XmMessageBoxGetChild(mdata->comp.widget,
					     XmDIALOG_HELP_BUTTON));
	break;
      default:
	break;
    }

    if (mOk) {
	XmStringFree(mOk);
    }
    if (mCancel) {
	XmStringFree(mCancel);
    }
    if (mHelp) {
	XmStringFree(mHelp);
    }
    if (mMessage) {
	XmStringFree(mMessage);
    }
    if (mTitle) {
	XmStringFree(mTitle);
    }
    XtAddCallback(mdata->comp.widget,
		  XmNokCallback,
		  MessageDialog_ok,
		  (XtPointer)peer);
    XtAddCallback(mdata->comp.widget,
		  XmNcancelCallback,
		  MessageDialog_cancel,
		  (XtPointer)peer);
    XtAddCallback(mdata->comp.widget,
		  XmNhelpCallback,
		  MessageDialog_help,
		  (XtPointer)peer);
    AWT_UNLOCK();
}


void
awt_motif_MMessageDialogPeer_setMessage(struct Hawt_motif_MMessageDialogPeer *this,
					Hjava_lang_String *message)
{
    Arg				args[20];
    Cardinal			argc;
    XmString			mMessage;
    struct MessageDialogData	*mdata;

    AWT_LOCK();
    mdata = PDATA(MessageDialogData,this);
    if (mdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    argc = 0;
    if (message != 0) {
	mMessage = XmStringCreateLocalized(makeCString(message));
	XtSetArg(args[argc], XmNmessageString, mMessage); argc++;
    }
    XtSetValues(mdata->comp.widget, args, argc);
    AWT_UNLOCK();
}

static int
WaitForDialogButton(void *data)
{
    return dialogButtonChosen != 0;
}

/*
** XXX if you have more than one MMessageDialog up at a time, you are
** hosed because the first one you hit ok/cancel on will cause them all
** to finish. This is because a global variable is used
** (dialogButtonChosen).
*/

long
awt_motif_MMessageDialogPeer_map(struct Hawt_motif_MMessageDialogPeer *this)
{
    struct MessageDialogData	*mdata;

    AWT_LOCK();
    dialogButtonChosen = 0;
    mdata = PDATA(MessageDialogData,this);
    if (mdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtManageChild(mdata->comp.widget);
    if (mdata->isModal) {
	AWT_UNLOCK();
	awt_WServer_modalWait(WaitForDialogButton, 0);
	return dialogButtonChosen;
    }

    AWT_UNLOCK();
    return -1;
}

void
awt_motif_MMessageDialogPeer_unMap(struct Hawt_motif_MMessageDialogPeer *this)
{
    struct MessageDialogData	*mdata;

    AWT_LOCK();
    mdata = PDATA(MessageDialogData,this);
    if (mdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(mdata->comp.widget);
    AWT_UNLOCK();
}

void
awt_motif_MMessageDialogPeer_dispose(struct Hawt_motif_MMessageDialogPeer *this)
{
    struct MessageDialogData	*mdata;

    AWT_LOCK();
    mdata = PDATA(MessageDialogData,this);
    if (mdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtDestroyWidget(mdata->comp.widget);
    sysFree(mdata);
    SET_PDATA(this,0);
    AWT_UNLOCK();
}

