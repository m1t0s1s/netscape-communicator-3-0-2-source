/*
 * @(#)awt_Dialog.c	1.33 95/12/08 Sami Shaio
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
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
#include <X11/Shell.h>
#include <Xm/VendorS.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>
#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "java_awt_Dialog.h"
#include "sun_awt_motif_MDialogPeer.h"
#include "sun_awt_motif_MFramePeer.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "java_awt_Image.h"
#include "java_awt_Font.h"
#include "java_awt_Insets.h"
#include "canvas.h"
#include "image.h"
#include "color.h"
#include "awt_util.h"
#include "ustring.h"

static int inreshape = 0;

static void
Window_resize(Widget wd, XtPointer client_data, XtPointer call_data)
{
    if (!inreshape) {
	Dimension width;
	Dimension height;
	Hjava_awt_Component *target = unhand((struct Hsun_awt_motif_MComponentPeer*)client_data)->target;
	Classjava_awt_Component *targetPtr = unhand(target);

	XtVaGetValues(wd,
		      XmNwidth, &width,
		      XmNheight, &height,
		      NULL);
	targetPtr->width = width;
	targetPtr->height = height;
	JAVA_UPCALL((EE(), (void *)client_data, "handleResize","(II)V",
		     width, height));
    }
}

static void
Dialog_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont)
{
    struct Hsun_awt_motif_MDialogPeer *this = (struct Hsun_awt_motif_MDialogPeer *)data;
    struct FrameData	*wdata;
    Classjava_awt_Component *targetPtr;

    wdata = PDATA(FrameData,this);
    if (wdata == 0) {
	return;
    }
    switch (event->xany.type) {
      case MapNotify:
	if (wdata->mappedOnce == 0) {
	    wdata->mappedOnce = 1;
	} else {
	    JAVA_UPCALL((EE(),
			 (void *)data,
			 "handleDeiconify","()V"));
	}
	break;
      case UnmapNotify:
	JAVA_UPCALL((EE(),
		     (void *)data,
		     "handleIconify","()V"));
	break;
      case ConfigureNotify:
	targetPtr = unhand(unhand(this)->target);
	targetPtr->x = event->xconfigure.x;
	targetPtr->y = event->xconfigure.y;
	JAVA_UPCALL((EE(),
		     (void *)data,
		     "handleMoved","(II)V", targetPtr->x, targetPtr->y));
	break;
      default:
	break;
    }
}

static int
WaitForUnmap(void *data)
{
    return XtIsManaged((Widget)data) == False;
}

static void
Dialog_quit(Widget w,
	   XtPointer client_data,
	   XtPointer call_data)
{
    JAVA_UPCALL((EE(), (void *)client_data,"handleQuit","()V"));
}
	   
static void
setDeleteCallback(struct Hsun_awt_motif_MDialogPeer *this, struct FrameData *wdata)
{
    Atom xa_WM_DELETE_WINDOW;
    Atom xa_WM_PROTOCOLS;

    XtVaSetValues(wdata->winData.shell,
		  XmNdeleteResponse, XmDO_NOTHING,
		  NULL);
    xa_WM_DELETE_WINDOW = XmInternAtom(XtDisplay(wdata->winData.shell),
				       "WM_DELETE_WINDOW", False);
    xa_WM_PROTOCOLS = XmInternAtom(XtDisplay(wdata->winData.shell),
				   "WM_PROTOCOLS", False);

    XmAddProtocolCallback(wdata->winData.shell,
			  xa_WM_PROTOCOLS,
			  xa_WM_DELETE_WINDOW,
			  Dialog_quit, (XtPointer)this);
}

static void
changeInsets(struct Hsun_awt_motif_MDialogPeer *this,
	     struct FrameData *wdata)
{
    Classjava_awt_Insets *insetsPtr;

    if (unhand(this)->insets == 0) {
	return;
    }

    insetsPtr = unhand(unhand(this)->insets);

    insetsPtr->top = wdata->top;
    insetsPtr->left = wdata->left;
    insetsPtr->bottom = wdata->bottom;
    insetsPtr->right = wdata->right;
}

void
sun_awt_motif_MDialogPeer_create(struct Hsun_awt_motif_MDialogPeer *this,
				 struct Hsun_awt_motif_MComponentPeer *parent,
				 struct Hjava_lang_Object *arg)
{
    Arg			args[40];
    int			argc;
    struct FrameData	*wdata;
    struct FrameData	*parentData;
    Classjava_awt_Insets *insetsPtr;
    Classjava_awt_Dialog	*targetPtr;
    
    AWT_LOCK();
    if (unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (parent == 0 || arg==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    wdata = ZALLOC(FrameData);
    if (wdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }

    SET_PDATA(this, wdata);
    targetPtr = unhand((struct Hjava_awt_Dialog *)unhand(this)->target);
    unhand(this)->insets = (struct Hjava_awt_Insets *)arg;
    insetsPtr = unhand(unhand(this)->insets);
    wdata->top = insetsPtr->top;
    wdata->left = insetsPtr->left;
    wdata->bottom = insetsPtr->bottom;
    wdata->right = insetsPtr->right;
    wdata->isModal = targetPtr->modal;
    wdata->mappedOnce = 0;

    argc = 0;
    parentData = PDATA(FrameData,parent);
    XtSetArg(args[argc], XmNtransientFor, parentData->winData.shell);argc++;
    XtSetArg(args[argc], XmNsaveUnder, False); argc++;
    XtSetArg(args[argc], XmNx, (XtArgVal)targetPtr->x); argc++;    
    XtSetArg(args[argc], XmNy, (XtArgVal)targetPtr->y); argc++;    
    XtSetArg(args[argc], XmNwidth,
	     (XtArgVal)targetPtr->width - (wdata->left + wdata->right)); argc++;
    XtSetArg(args[argc], XmNheight,
	     (XtArgVal)targetPtr->height - (wdata->top + wdata->bottom)); argc++;
    if (targetPtr->resizable) {
	XtSetArg(args[argc], XmNallowShellResize, True); argc++;
    } else {
	XtSetArg(args[argc], XmNallowShellResize, False); argc++;
    }
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNvisual, awt_visual); argc++;
    XtSetArg(args[argc], XmNcolormap, awt_cmap); argc++;
    XtSetArg(args[argc], XmNdepth, awt_depth); argc++;
    XtSetArg(args[argc], XmNnoResize, targetPtr->resizable ? False : True); argc++;
    wdata->winData.shell = XtCreatePopupShell("dialog",
					      xmDialogShellWidgetClass,
					      parentData->winData.shell,
					      args,
					      argc);
    setDeleteCallback(this, wdata);

    XtAddEventHandler(wdata->winData.shell, StructureNotifyMask,
		      FALSE, Dialog_event_handler, this);
    argc = 0;
    if (wdata->isModal) {
	XtSetArg(args[argc], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	argc++;
    }
    XtSetArg(args[argc], XmNwidth,
	     (XtArgVal)targetPtr->width - (wdata->left + wdata->right)); argc++;
    XtSetArg(args[argc], XmNheight,
	     (XtArgVal)targetPtr->height - (wdata->top + wdata->bottom)); argc++;
    XtSetArg(args[argc], XmNmainWindowMarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmainWindowMarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNspacing, 0); argc++;
    wdata->mainWindow = XmCreateForm(wdata->winData.shell, "main",
				     args, argc);
    wdata->winData.comp.widget = awt_canvas_create((XtPointer)this,
						   wdata->mainWindow,
						   targetPtr->width,
						   targetPtr->height,
						   wdata);

    if (targetPtr->warningString) {
	Dimension hoffset;
	wdata->warningWindow = awt_util_createWarningWindow(wdata->mainWindow,
							    makeCString(targetPtr->warningString));
	XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
	wdata->top += hoffset;
	XtVaSetValues(wdata->warningWindow,
		      XmNtopAttachment, XmATTACH_FORM,
		      XmNleftAttachment, XmATTACH_FORM,
		      XmNrightAttachment, XmATTACH_FORM,
		      NULL);
	XtVaSetValues(XtParent(wdata->winData.comp.widget),
		      XmNtopAttachment, XmATTACH_WIDGET,
		      XmNtopWidget, wdata->warningWindow,
		      XmNbottomAttachment, XmATTACH_FORM,
		      XmNleftAttachment, XmATTACH_FORM,
		      XmNrightAttachment, XmATTACH_FORM,
		      NULL);
	changeInsets(this, wdata);
    } else {
	wdata->warningWindow = NULL;
	XtVaSetValues(XtParent(wdata->winData.comp.widget),
		      XmNbottomAttachment, XmATTACH_FORM,
		      XmNtopAttachment, XmATTACH_FORM,
		      XmNleftAttachment, XmATTACH_FORM,
		      XmNrightAttachment, XmATTACH_FORM,
		      NULL);
    }

    XtAddCallback(wdata->winData.comp.widget,
		  XmNresizeCallback,
		  Window_resize,
		  this);

    wdata->menuBar = NULL;

    awt_util_show(wdata->winData.comp.widget);
    XtSetMappedWhenManaged(wdata->winData.shell, False);
    XtManageChild(wdata->mainWindow);
    AWT_UNLOCK();
}

void
sun_awt_motif_MDialogPeer_pShow(struct Hsun_awt_motif_MDialogPeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 ||
	wdata->winData.comp.widget==0 ||
	wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(wdata->winData.comp.widget,
		  XmNx, -(wdata->left),
		  XmNy, -(wdata->top), NULL);

    XtManageChild(wdata->winData.comp.widget);
    XtSetMappedWhenManaged(wdata->winData.shell, True);
    if (wdata->isModal) {
        XtPopup(wdata->winData.shell, XtGrabNonexclusive);
        AWT_FLUSH_UNLOCK();
        awt_MToolkit_modalWait(WaitForUnmap, wdata->winData.comp.widget);
    } else {
        XtPopup(wdata->winData.shell, XtGrabNonexclusive);
        XRaiseWindow(awt_display, XtWindow(wdata->winData.shell));
        AWT_FLUSH_UNLOCK();
    }
}


void
sun_awt_motif_MDialogPeer_pHide(struct Hsun_awt_motif_MDialogPeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 ||
	wdata->winData.comp.widget==0 ||
	wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(wdata->winData.comp.widget);
    XtPopdown(wdata->winData.shell);
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MDialogPeer_pSetTitle(struct Hsun_awt_motif_MDialogPeer *this,
				   struct Hjava_lang_String *title)
{
    char                *ctitle;
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->winData.shell == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    ctitle = makeLocalizedCString( title );
    if( ctitle == NULL ) {
        AWT_UNLOCK();
        return;
    }

    XtVaSetValues(wdata->winData.shell,
		  XmNtitle, ctitle,
		  XmNiconName, ctitle,
		  XmNname, ctitle,
		  NULL);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MDialogPeer_pDispose(struct Hsun_awt_motif_MDialogPeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->mainWindow==0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(wdata->winData.shell);
    XtDestroyWidget(wdata->mainWindow);
    XtDestroyWidget(wdata->winData.shell);
    sysFree((void *)wdata);
    SET_PDATA(this,0);
    AWT_UNLOCK();
}


void
sun_awt_motif_MDialogPeer_pReshape(struct Hsun_awt_motif_MDialogPeer *this,
				  long x, long y,
				  long w, long h)
{
    struct FrameData		*wdata;
    Dimension			hoffset;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 ||
	wdata->winData.comp.widget==0 ||
	wdata->winData.shell==0 ||
	unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    inreshape = 1;

    if (wdata->warningWindow != 0) {
	XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
    } else {
	hoffset = 0;
    }

    w -= (wdata->left + wdata->right);
    h += (hoffset - (wdata->top + wdata->bottom));

    if (w < 0) {
	w = 0;
    }
    if (h < 0) {
	h = 0;
    }
    XtUnmanageChild(wdata->mainWindow);
    XtVaSetValues(wdata->mainWindow,
		  XmNx, (XtArgVal) x,
		  XmNy, (XtArgVal) y,
		  XmNwidth, (XtArgVal)w,
 		  XmNheight, (XtArgVal)h,
		  NULL);
    XtManageChild(wdata->mainWindow);
    inreshape = 0;

    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MDialogPeer_setResizable(struct Hsun_awt_motif_MDialogPeer *this,
				      long resizable)
{
    struct FrameData		*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 ||
	wdata->winData.comp.widget==0 ||
	wdata->winData.shell==0 ||
	unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(wdata->winData.shell,
		  XmNnoResize, resizable ? False : True,
		  NULL);

    AWT_UNLOCK();
}

void
sun_awt_motif_MDialogPeer_toBack(struct Hsun_awt_motif_MDialogPeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (XtWindow(wdata->winData.shell) != 0) {
	XLowerWindow(awt_display, XtWindow(wdata->winData.shell));
    }
    AWT_FLUSH_UNLOCK();
}
