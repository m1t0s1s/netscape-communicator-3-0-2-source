/*
 * @(#)awt_Window.c	1.20 95/12/08 Sami Shaio
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
#include <Xm/RowColumn.h>
#include <Xm/AtomMgr.h>
#include <Xm/MenuShell.h>
#include <Xm/MwmUtil.h>
#include "java_awt_Window.h"
#include "java_awt_Insets.h"
#include "sun_awt_motif_MWindowPeer.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "canvas.h"
#include "color.h"

#if 0
static int
WaitForUnmap(void *data)
{
    return XtIsManaged((Widget)data) == False;
}
#endif

static void
Window_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont)
{
    struct Hsun_awt_motif_MWindowPeer *this = (struct Hsun_awt_motif_MWindowPeer *)data;
    struct FrameData	*wdata;

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
      default:
	break;
    }
}

static void
changeInsets(struct Hsun_awt_motif_MWindowPeer *this,
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
sun_awt_motif_MWindowPeer_create(struct Hsun_awt_motif_MWindowPeer *this,
				 struct Hsun_awt_motif_MComponentPeer *parent)
{
    Dimension		w,h;
    Position		x,y;
    Arg			args[20];
    int			argc;
    struct FrameData	*wdata;
    struct FrameData	*parentData;
    Classjava_awt_Window	*targetPtr;
    
    AWT_LOCK();
    if (unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    targetPtr = unhand((struct Hjava_awt_Window	*)unhand(this)->target);
    if (parent == 0) {
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
    parentData = PDATA(FrameData,parent);
    x = targetPtr->x;
    y = targetPtr->y;
    w = targetPtr->width;
    h = targetPtr->height;
    argc = 0;
    XtSetArg(args[argc], XmNtransientFor, parentData->winData.shell);argc++;
    XtSetArg(args[argc], XmNsaveUnder, False); argc++;
    XtSetArg(args[argc], XmNx, x); argc++;    
    XtSetArg(args[argc], XmNy, y); argc++;    
    XtSetArg(args[argc], XmNwidth, w); argc++;
    XtSetArg(args[argc], XmNheight, h); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNvisual, awt_visual); argc++;
    XtSetArg(args[argc], XmNcolormap, awt_cmap); argc++;
    XtSetArg(args[argc], XmNdepth, awt_depth); argc++;
    wdata->winData.shell = XtCreatePopupShell("Window",
					      xmMenuShellWidgetClass,
					      parentData->winData.shell,
					      args,
					      argc);

    XtAddEventHandler(wdata->winData.shell, StructureNotifyMask,
		      FALSE, Window_event_handler, this);

    argc = 0;
    XtSetArg(args[argc], XmNwidth, w); argc++;
    XtSetArg(args[argc], XmNheight, h); argc++;
    XtSetArg(args[argc], XmNmainWindowMarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmainWindowMarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNspacing, 0); argc++;
    wdata->mainWindow = XmCreateRowColumn(wdata->winData.shell, "main",
					  args, argc);
    wdata->top = wdata->left = wdata->bottom = wdata->right = 0;
    if (targetPtr->warningString) {
	Dimension hoffset;
	wdata->warningWindow = awt_util_createWarningWindow(wdata->mainWindow,
							    makeCString(targetPtr->warningString));
	XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
	wdata->top += hoffset;
	changeInsets(this, wdata);
    } else {
	wdata->warningWindow = NULL;
    }

    wdata->winData.comp.widget = awt_canvas_create((XtPointer)this,
						   wdata->mainWindow,
						   targetPtr->width,
						   targetPtr->height,
						   (wdata->warningWindow
						    ? wdata : NULL));
    awt_util_show(wdata->winData.comp.widget);
    XtManageChild(wdata->mainWindow);
    AWT_UNLOCK();
}

void
sun_awt_motif_MWindowPeer_pShow(struct Hsun_awt_motif_MWindowPeer *this)
{
    struct FrameData	*wdata;
    Dimension	width,height;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 ||
	wdata->winData.comp.widget==0 ||
	wdata->winData.shell==0 ||
	wdata->mainWindow == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(wdata->winData.comp.widget,
		  XmNx, -(wdata->left),
		  XmNy, -(wdata->top), NULL);

    XtVaGetValues(wdata->mainWindow,
		  XmNwidth, &width,
		  XmNheight, &height,
		  NULL);
    XtPopup(wdata->winData.shell, XtGrabNone);
    XRaiseWindow(awt_display, XtWindow(wdata->winData.shell));
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MWindowPeer_pHide(struct Hsun_awt_motif_MWindowPeer *this)
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
    XtPopdown(wdata->winData.shell);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MWindowPeer_pDispose(struct Hsun_awt_motif_MWindowPeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->mainWindow==0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(wdata->mainWindow);
    XtDestroyWidget(wdata->mainWindow);
    XtDestroyWidget(wdata->winData.shell);
    sysFree((void *)wdata);
    SET_PDATA(this,0);
    AWT_UNLOCK();
}


void
sun_awt_motif_MWindowPeer_pReshape(struct Hsun_awt_motif_MWindowPeer *this,
				  long x, long y,
				  long w, long h)
{
    struct FrameData		*wdata;
    Dimension			hoffset;
    long			aw, ah;

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

    if (wdata->warningWindow != 0) {
	XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
    } else {
	hoffset = 0;
    }
    aw = w - (wdata->left + wdata->right);

    if (aw < 0) {
	aw = 0;
    }

    ah = h + hoffset - (wdata->top + wdata->bottom);

    if (ah < 0) {
	ah = 0;
    }
    XtUnmanageChild(wdata->mainWindow);
    XtVaSetValues(wdata->winData.shell,
		  XtNx, (XtArgVal) x,
		  XtNy, (XtArgVal) y,
		  XtNwidth, (XtArgVal)aw,
 		  XtNheight, (XtArgVal)ah,
		  NULL);
    XtVaSetValues(wdata->winData.comp.widget,
		  XtNx, (XtArgVal) x - wdata->left,
		  XtNy, (XtArgVal) y - wdata->top,
		  XtNwidth, (XtArgVal) w + wdata->left + wdata->right,
		  XtNheight, (XtArgVal) h + wdata->top + wdata->bottom,
		  NULL);
    XtManageChild(wdata->mainWindow);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MWindowPeer_toBack(struct Hsun_awt_motif_MWindowPeer *this)
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
