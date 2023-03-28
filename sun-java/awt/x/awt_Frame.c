/*
 * @(#)awt_Frame.c	1.64 95/12/08 Sami Shaio
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
#include <X11/cursorfont.h>
#include <Xm/VendorS.h>
#include <Xm/DialogS.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>
#include <X11/IntrinsicP.h>
#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "java_awt_Frame.h"
#include "sun_awt_motif_MFramePeer.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include "java_awt_Insets.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "java_awt_Image.h"
#include "java_awt_Font.h"
#include "canvas.h"
#include "image.h"
#include "awt_util.h"
#include "ustring.h"

#ifdef NETSCAPE
#undef Bool
#include "netscape_applet_MozillaAppletContext.h"
#include "netscape_applet_EmbeddedAppletFrame.h"
#include <java.h> /* for LJAppletData */
#include <structs.h> /* XXX remove later */
#include <shist.h> /* XXX remove later */
#endif /* NETSCAPE */

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
Frame_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont)
{
    struct Hsun_awt_motif_MFramePeer *this = (struct Hsun_awt_motif_MFramePeer *)data;
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

static void
Frame_quit(Widget w,
	   XtPointer client_data,
	   XtPointer call_data)
{
    JAVA_UPCALL((EE(), (void *)client_data,"handleQuit","()V"));
}
	   
static void
setDeleteCallback(struct Hsun_awt_motif_MFramePeer *this, struct FrameData *wdata)
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
			  Frame_quit, (XtPointer)this);
}


static void
changeInsets(struct Hsun_awt_motif_MFramePeer *this,
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
sun_awt_motif_MFramePeer_create(struct Hsun_awt_motif_MFramePeer *this,
				struct Hsun_awt_motif_MComponentPeer *parent,
				struct Hjava_lang_Object *arg)
{
    Arg			args[40];
    int			argc;
    struct FrameData	*wdata;
    Classjava_awt_Frame	*targetPtr;
    Classjava_awt_Insets *insetsPtr;
#ifdef NETSCAPE
    ClassClass		*cb;
    int			isEmbedded;
#endif
    
    AWT_LOCK();
    if (unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    targetPtr = unhand((struct Hjava_awt_Frame *)unhand(this)->target);
    unhand(this)->insets = (struct Hjava_awt_Insets *)arg;
    insetsPtr = unhand(unhand(this)->insets);
    wdata = ZALLOC(FrameData);
    SET_PDATA(this, wdata);
    if (wdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    wdata->cursorSet = 1;
    wdata->cursor = 0;

#ifdef NETSCAPE
    cb = FindClass(0, "netscape/applet/EmbeddedAppletFrame", FALSE);
    isEmbedded = cb && is_instance_of((JHandle*)unhand(this)->target, cb, EE());
    if (isEmbedded) {
	wdata->winData.flags |= W_IS_EMBEDDED;
	wdata->top = insetsPtr->top = 0;
	wdata->left = insetsPtr->left = 0;
	wdata->bottom = insetsPtr->bottom = 0;
	wdata->right = insetsPtr->right = 0;
    } else {
	wdata->top = insetsPtr->top;
	wdata->left = insetsPtr->left;
	wdata->bottom = insetsPtr->bottom;
	wdata->right = insetsPtr->right;
    }
#else
    wdata->top = insetsPtr->top;
    wdata->left = insetsPtr->left;
    wdata->bottom = insetsPtr->bottom;
    wdata->right = insetsPtr->right; 
#endif				/* NETSCAPE */

    wdata->isModal = 0;
    wdata->mappedOnce = 0;

    /* create a top-level shell */
    argc = 0;
    XtSetArg(args[argc], XmNsaveUnder, False); argc++;
    XtSetArg(args[argc], XmNx, (XtArgVal)targetPtr->x); argc++;    
    XtSetArg(args[argc], XmNy, (XtArgVal)targetPtr->y); argc++;    
    XtSetArg(args[argc], XmNwidth, (XtArgVal)targetPtr->width); argc++;
    XtSetArg(args[argc], XmNheight, (XtArgVal)targetPtr->height); argc++;
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
    XtSetArg(args[argc], XmNmappedWhenManaged, False); argc++;

    wdata->winData.shell = XtAppCreateShell("AWTapp","XApplication",
					    vendorShellWidgetClass,
					    awt_display,
					    args,
					    argc);
    setDeleteCallback(this, wdata);

    XtAddEventHandler(wdata->winData.shell, StructureNotifyMask,
		      FALSE, Frame_event_handler, this);
    argc = 0;
    XtSetArg(args[argc], XmNwidth,
	     (XtArgVal)targetPtr->width - (wdata->left + wdata->right)); argc++;
    XtSetArg(args[argc], XmNheight,
	     (XtArgVal)targetPtr->height - (wdata->top + wdata->bottom)); argc++;
    XtSetArg(args[argc], XmNmainWindowMarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmainWindowMarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNspacing, 0); argc++;
#ifdef NETSCAPE
    XtSetArg(args[argc], XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE); argc++;
#endif /* NETSCAPE */
    wdata->mainWindow = XmCreateMainWindow(wdata->winData.shell, "main", args, argc);

    wdata->winData.comp.widget = awt_canvas_create((XtPointer)this,
						   wdata->mainWindow,
						   targetPtr->width,
						   targetPtr->height,
						   wdata);
#ifdef NETSCAPE
    if (targetPtr->warningString && !isEmbedded)
#else
    if (targetPtr->warningString)
#endif
    {
	Dimension hoffset;

	wdata->warningWindow = awt_util_createWarningWindow(wdata->mainWindow,
							    makeCString(targetPtr->warningString));
	XtVaGetValues(wdata->warningWindow, XmNheight, &hoffset, NULL);
	wdata->top += hoffset;
	changeInsets(this, wdata);
    } else {
	wdata->warningWindow = NULL;
    }

    XtVaSetValues(wdata->winData.comp.widget, XmNy, 0, NULL);
    XmMainWindowSetAreas(wdata->mainWindow,
			 NULL,
			 wdata->warningWindow,
			 NULL,
			 NULL,
			 XtParent(wdata->winData.comp.widget));

    XtAddCallback(wdata->winData.comp.widget,
		  XmNresizeCallback,
		  Window_resize,
		  this);

    wdata->menuBar = NULL;
    awt_util_show(wdata->winData.comp.widget);

    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MFramePeer_pSetIconImage(struct Hsun_awt_motif_MFramePeer *this,
				       struct Hsun_awt_image_ImageRepresentation *ir)
{
    struct FrameData	*wdata;
    IRData		*ird;
    unsigned int width, height, border_width, depth;
    Window		win;
    Window		root;
    int x,y;
    unsigned long mask;
    XSetWindowAttributes attrs;

    if (ir == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    ird = (IRData *)unhand(ir)->pData;
    if (wdata == 0 || ird == 0 || ird->pixmap == 0 ||
	wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    XtVaGetValues(wdata->winData.shell,
		  XmNiconWindow, &win,
		  NULL);
    if (!win) {
	mask = CWBorderPixel | CWColormap;
	attrs.border_pixel = XFOREGROUND;
	attrs.colormap = XCOLORMAP;
	if (!XGetGeometry(awt_display,
			  ird->pixmap,
			  &root,
			  &x,
			  &y,
			  &width,
			  &height,
			  &border_width,
			  &depth) ||
	    ! (win = XCreateWindow(awt_display,
				   root,
				   0, 0, width, height,
				   (unsigned)0, depth,
				   InputOutput,
				   awt_visual,
				   mask, &attrs))) {
	    XtVaSetValues(wdata->winData.shell,
			  XmNiconPixmap, ird->pixmap,
			  NULL);
	    AWT_FLUSH_UNLOCK();
	    return;
	}
    }
    XtVaSetValues(wdata->winData.shell,
		  XmNiconWindow, win,
		  NULL);
    XSetWindowBackgroundPixmap(awt_display, win, ird->pixmap);
    XClearWindow(awt_display, win);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFramePeer_pSetMenuBar(struct Hsun_awt_motif_MFramePeer *this,
				     struct Hjava_awt_MenuBar *mb)
{
    struct FrameData		*wdata;
    struct ComponentData	*mdata;
    Classjava_awt_Frame		*thisPtr;
    Dimension			hoffset;

    if (mb == 0) {
	return;
    }

    AWT_LOCK();
    if (unhand(this)->target == 0 ||
	(wdata = PDATA(FrameData, this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    mdata = PEER_PDATA(ComponentData,Hsun_awt_motif_MMenuBarPeer, mb);
    if (mdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    /* if the frame has a menubar, unmanage it and install the current */
    /* one instead. */
    if (wdata->menuBar != 0) {
	thisPtr = unhand((Hjava_awt_Frame *)(unhand(this)->target));    
	if (thisPtr != 0) {
	    if (wdata->menuBar == mdata->widget) {
		AWT_UNLOCK();
		return;
	    }
	    XtSetMappedWhenManaged(wdata->menuBar, False);
	}
    }
    XtVaSetValues(mdata->widget, XmNresizeHeight, True, NULL);
    XmMainWindowSetAreas(wdata->mainWindow,
			 mdata->widget,
			 wdata->warningWindow,
			 NULL,
			 NULL,
			 XtParent(wdata->winData.comp.widget));
    XtSetMappedWhenManaged(mdata->widget, True);
    if (wdata->menuBar == 0) {
	XtVaGetValues(mdata->widget, XmNheight, &hoffset, NULL);
	hoffset += 15;
	wdata->top += hoffset;
	changeInsets(this, wdata);
	awt_canvas_reconfigure(wdata);
    }
    wdata->menuBar = mdata->widget;		      
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFramePeer_pShow(struct Hsun_awt_motif_MFramePeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->winData.comp.widget==0 || wdata->winData.shell==0 ||
	wdata->mainWindow == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    XtVaSetValues(wdata->winData.comp.widget,
		  XmNx, -(wdata->left),
		  XmNy, -(wdata->top), NULL);

    if (wdata->menuBar != 0) {
	awt_util_show(wdata->menuBar);
    }
    XtManageChild(wdata->mainWindow);
    if (XtWindow(wdata->winData.shell) == 0) {
	XtRealizeWidget(wdata->winData.shell);
    }
#ifdef NETSCAPE
    if (wdata->winData.flags & W_IS_EMBEDDED) {
	struct Hjava_awt_Frame *target =
	    (struct Hjava_awt_Frame *) unhand(this)->target;
	Hnetscape_applet_EmbeddedAppletFrame* appletFrame =
	    (Hnetscape_applet_EmbeddedAppletFrame*)target;
	LJAppletData* ad = (LJAppletData*)unhand(appletFrame)->pData;

	/*
	** Squirrel away the shell widget that was created by awt.  We'll
	** reparent this back to the root window when a grid cell is
	** destroyed, and reparent it back to the ad->window again when
	** the grid cell is recreated.
	*/
	if (ad) {
	    ad->awtWindow = wdata->winData.shell;

	    if (ad->window) {
		/*
		** Only reparent the shell widget on the context widget if
		** the context hasn't been destroyed. If it did get
		** destroyed, our ad->window is null.
		*/
		XReparentWindow(XtDisplay(wdata->winData.shell),
				XtWindow(wdata->winData.shell),
				XtWindow((Widget)ad->window), 0, 0);
	    }
	    else {
		/*
		** If we did get destroyed, don't reparent the shell on the
		** desktop.
		*/
		goto done;
	    }
	}
    }
#endif /* NETSCAPE */

    XtMapWidget(wdata->winData.shell);
    XRaiseWindow(awt_display, XtWindow(wdata->winData.shell));
  done:
    if (wdata->cursorSet == 0) {
	unsigned long 		valuemask = 0;
	XSetWindowAttributes	attributes;

	valuemask = CWCursor;
	attributes.cursor = wdata->cursor;
	XChangeWindowAttributes(awt_display,
				XtWindow(wdata->winData.shell),
				valuemask,
				&attributes);
    }
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFramePeer_pHide(struct Hsun_awt_motif_MFramePeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->winData.comp.widget==0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (XtWindow(wdata->winData.shell) != 0) {
	XtUnmapWidget(wdata->winData.shell);
    }
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFramePeer_toBack(struct Hsun_awt_motif_MFramePeer *this)
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


void
sun_awt_motif_MFramePeer_pSetTitle(struct Hsun_awt_motif_MFramePeer *this,
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
sun_awt_motif_MFramePeer_pDispose(struct Hsun_awt_motif_MFramePeer *this)
{
    struct FrameData	*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->mainWindow==0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
#ifdef NETSCAPE
    if (wdata->winData.flags & W_IS_EMBEDDED) {
	struct Hjava_awt_Frame *target;
	Hnetscape_applet_EmbeddedAppletFrame* appletFrame;
	LJAppletData* ad;

	target = (struct Hjava_awt_Frame *) unhand(this)->target;
	appletFrame = (Hnetscape_applet_EmbeddedAppletFrame*)target;
	if (appletFrame) {
	    ad = (LJAppletData*)unhand(appletFrame)->pData;
	    if (ad) {
		ad->awtWindow = NULL;
	    }
	}
    }
#endif /* NETSCAPE */
    if (wdata->cursor != 0 && wdata->cursor != None) {
	XFreeCursor(awt_display, wdata->cursor);
    }
    XtUnmanageChild(wdata->mainWindow);
    XtDestroyWidget(wdata->mainWindow);
    XtDestroyWidget(wdata->winData.shell);
    sysFree((void *)wdata);
    SET_PDATA(this,0);
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MFramePeer_pReshape(struct Hsun_awt_motif_MFramePeer *this,
				  long x, long y,
				  long w, long h)
{
    struct FrameData		*wdata;
    Dimension			woffset, hoffset;

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

    hoffset = 0;
    if (wdata->menuBar != 0) {
	XtVaGetValues(wdata->menuBar, XmNheight, &hoffset, NULL);
	hoffset += 15;
    }
    if (wdata->warningWindow != 0) {
	XtVaGetValues(wdata->warningWindow, XmNheight, &woffset, NULL);
	hoffset += woffset;
    }

    XtVaSetValues(wdata->winData.shell,
		  XtNx, (XtArgVal) x,
		  XtNy, (XtArgVal) y,
		  XtNwidth, (XtArgVal) w - (wdata->left + wdata->right),
 		  XtNheight, (XtArgVal) h + hoffset - (wdata->top + wdata->bottom),
		  NULL);
    XtVaSetValues(XtParent(wdata->winData.comp.widget),
		  XtNx, (XtArgVal) x,
		  XtNy, (XtArgVal) y,
		  XtNwidth, (XtArgVal) w - (wdata->left + wdata->right),
 		  XtNheight, (XtArgVal) h + hoffset - (wdata->top + wdata->bottom),
		  NULL);
    XtVaSetValues(wdata->winData.comp.widget,
		  XtNx, (XtArgVal) x - wdata->left,
		  XtNy, (XtArgVal) y - wdata->top,
		  XtNwidth, (XtArgVal) w,
		  XtNheight, (XtArgVal) h,
		  NULL);

    inreshape = 0;

    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFramePeer_setResizable(struct Hsun_awt_motif_MFramePeer *this,
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
		  XmNallowShellResize, resizable ? False : True,
		  XmNnoResize, resizable ? False : True,
		  NULL);

    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFramePeer_setCursor(struct Hsun_awt_motif_MFramePeer *this,
				   long cursorType)
{
    Cursor cursor;
    struct FrameData		*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    switch (cursorType) {
      case java_awt_Frame_DEFAULT_CURSOR:
	cursorType = -1;
	break;
      case java_awt_Frame_CROSSHAIR_CURSOR:
	cursorType = XC_crosshair;
	break;
      case java_awt_Frame_TEXT_CURSOR:
	cursorType = XC_xterm;
	break;
      case java_awt_Frame_WAIT_CURSOR:
	cursorType = XC_watch;
	break;
      case java_awt_Frame_SW_RESIZE_CURSOR:
	cursorType = XC_bottom_left_corner;
	break;
      case java_awt_Frame_NW_RESIZE_CURSOR:
	cursorType = XC_top_left_corner;
	break;
      case java_awt_Frame_SE_RESIZE_CURSOR:
	cursorType = XC_bottom_right_corner;
	break;
      case java_awt_Frame_NE_RESIZE_CURSOR:
	cursorType = XC_top_right_corner;
	break;
      case java_awt_Frame_S_RESIZE_CURSOR:
	cursorType = XC_bottom_side;
	break;
      case java_awt_Frame_N_RESIZE_CURSOR:
	cursorType = XC_top_side;
	break;
      case java_awt_Frame_W_RESIZE_CURSOR:
	cursorType = XC_left_side;
	break;
      case java_awt_Frame_E_RESIZE_CURSOR:
	cursorType = XC_right_side;
	break;
      case java_awt_Frame_HAND_CURSOR:
	cursorType = XC_hand2;
	break;
      case java_awt_Frame_MOVE_CURSOR:
	cursorType = XC_fleur;
	break;
      default:
	AWT_UNLOCK();
	return;
    }
    if (cursorType != -1) {
	cursor = XCreateFontCursor(awt_display, cursorType);
    } else {
	cursor = None;
    }
    if (wdata->cursor != 0 && wdata->cursor != None) {
	XFreeCursor(awt_display, wdata->cursor);
    }
    wdata->cursor = cursor;
    wdata->cursorSet = 0;
    if (XtWindow(wdata->winData.shell)) {
	unsigned long 		valuemask = 0;
	XSetWindowAttributes	attributes;

	valuemask = CWCursor;
	attributes.cursor = wdata->cursor;
	XChangeWindowAttributes(awt_display,
				XtWindow(wdata->winData.shell),
				valuemask,
				&attributes);
	wdata->cursorSet = 1;
    }

    AWT_FLUSH_UNLOCK();
}

    
