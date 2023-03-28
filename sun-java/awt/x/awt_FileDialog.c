/*
 * @(#)awt_FileDialog.c	1.15 95/12/04 Sami Shaio
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
#include "interpreter.h"
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>
#include "awt_p.h"
#include "java_awt_FileDialog.h"
#include "sun_awt_motif_MFileDialogPeer.h"
#include "sun_awt_motif_MComponentPeer.h"

static void
FileDialog_OK(
    Widget w,
    void *client_data,
    XmFileSelectionBoxCallbackStruct *call_data
)
{
    struct Hsun_awt_motif_MFileDialogPeer *this = (struct Hsun_awt_motif_MFileDialogPeer *)client_data;
    struct FrameData	*fdata = PDATA(FrameData,this);
    char		*file;

    XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,  &file);
    JAVA_UPCALL((EE(), client_data,"handleSelected",
		 "(Ljava/lang/String;)V", makeJavaString(file, strlen(file))));
    XtUnmanageChild(fdata->winData.comp.widget);
}

static void
FileDialog_CANCEL(
    Widget w,
    void *client_data,
    XmFileSelectionBoxCallbackStruct *call_data
)
{
    struct Hsun_awt_motif_MFileDialogPeer *this = (struct Hsun_awt_motif_MFileDialogPeer *)client_data;
    struct FrameData	*fdata = PDATA(FrameData,this);

    JAVA_UPCALL((EE(), client_data,"handleCancel","()V"));
    XtUnmanageChild(fdata->winData.comp.widget);
}


static void
FileDialog_quit(Widget w,
	   XtPointer client_data,
	   XtPointer call_data)
{
    JAVA_UPCALL((EE(), (void *)client_data,"handleQuit","()V"));
}
	   
static void
setDeleteCallback(struct Hsun_awt_motif_MFileDialogPeer *this, struct FrameData *wdata)
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
			  FileDialog_quit, (XtPointer)this);
}


static void
changeBackground(Widget w, void *bg)
{
    XmChangeColor(w, (Pixel)bg);
}

void
sun_awt_motif_MFileDialogPeer_create(struct Hsun_awt_motif_MFileDialogPeer *this,
				     struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct FrameData		*fdata;
    struct CanvasData		*wdata;
    int				argc;
    Arg				args[10];
    Widget			child;
    XmString			xim;
    Pixel			bg;
    Classjava_awt_FileDialog	*targetPtr;

    if (parent == 0 || unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    AWT_LOCK();

    wdata = PDATA(CanvasData,parent);
    fdata = ZALLOC(FrameData);
    SET_PDATA(this, fdata);

    if (fdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }

    XtVaGetValues(wdata->comp.widget, XmNbackground, &bg, NULL);

    argc=0;
    XtSetArg(args[argc], XmNmustMatch, False); argc++;
    XtSetArg(args[argc], XmNautoUnmanage, False); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNvisual, awt_visual); argc++;
    XtSetArg(args[argc], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);argc++;
    fdata->winData.comp.widget = XmCreateFileSelectionDialog(wdata->shell,
							     "",
							     args,
							     argc);
    fdata->winData.shell = XtParent(fdata->winData.comp.widget);
    awt_util_mapChildren(fdata->winData.shell, changeBackground, 0, (void *)bg);
    child = XmFileSelectionBoxGetChild(fdata->winData.comp.widget,
				       XmDIALOG_HELP_BUTTON);
    if (child != 0) {
	XtUnmanageChild(child);
    }
    targetPtr = unhand((struct Hjava_awt_FileDialog *)unhand(this)->target);
    child = XmFileSelectionBoxGetChild(fdata->winData.comp.widget,XmDIALOG_DEFAULT_BUTTON);
    if (child != 0) {
	XmString	xim;

	switch (targetPtr->mode) {
	  case java_awt_FileDialog_LOAD:
	    xim = XmStringCreateLtoR("Open", "labelFont");
	    XtVaSetValues(child, XmNlabelString, xim, NULL);
	    XmStringFree(xim);
	    break;
	  case java_awt_FileDialog_SAVE:
	    xim = XmStringCreateLtoR("Save", "labelFont");
	    XtVaSetValues(child, XmNlabelString, xim, NULL);
	    XmStringFree(xim);
	    break;
	  default:
	    break;
	}
    }
    child = XmFileSelectionBoxGetChild(fdata->winData.comp.widget,
				       XmDIALOG_TEXT);
    if (child != 0 && targetPtr->file != 0) {
	XtVaSetValues(child, XmNvalue, makeCString(targetPtr->file), NULL);
    }
    XtAddCallback(fdata->winData.comp.widget,
		  XmNokCallback,
		  (XtCallbackProc)FileDialog_OK,
		  (XtPointer)this);
    XtAddCallback(fdata->winData.comp.widget,
		  XmNcancelCallback,
		  (XtCallbackProc)FileDialog_CANCEL,
		  (XtPointer)this);
    setDeleteCallback(this, fdata);
    xim=XmStringCreateLtoR("./*", XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues(fdata->winData.comp.widget, XmNdirMask, xim,  NULL);
    /*XmFileSelectionDoSearch(fdata->winData.comp.widget, xim);*/
    XmStringFree(xim);
    AWT_UNLOCK();
}

static int
WaitForUnmap(void *data)
{
    return (XtWindow((Widget)data) && !XtIsManaged((Widget)data));
}


void
sun_awt_motif_MFileDialogPeer_pShow(struct Hsun_awt_motif_MFileDialogPeer *this)
{
    struct FrameData	*wdata;
    XmString		dirMask = NULL;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 ||
	wdata->winData.comp.widget==0 ||
	wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaGetValues(wdata->winData.comp.widget, XmNdirMask, &dirMask, NULL);
    if (dirMask != NULL) {
	XmFileSelectionDoSearch(wdata->winData.comp.widget, dirMask);
    }
    XtManageChild(wdata->winData.comp.widget);
    AWT_FLUSH_UNLOCK();

    awt_MToolkit_modalWait(WaitForUnmap, wdata->winData.comp.widget);
}


void
sun_awt_motif_MFileDialogPeer_pHide(struct Hsun_awt_motif_MFileDialogPeer *this)
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
    if (XtIsManaged(wdata->winData.comp.widget)) {
	XtUnmanageChild(wdata->winData.comp.widget);
    }
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFileDialogPeer_setFileEntry(struct Hsun_awt_motif_MFileDialogPeer *this,
					   struct Hjava_lang_String *dir,
					   struct Hjava_lang_String *file)
{
    struct ComponentData *cdata;
    XmString	xim;
    char	*cdir;
    char	*separator;
    char	path[MAXPATHLEN];
    int		dirLen;

    AWT_LOCK();
    if ((cdata = PDATA(ComponentData,this)) == 0 || dir == 0) {
	AWT_UNLOCK();
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    cdir = makeCString(dir);
    dirLen = strlen(cdir);

    jio_snprintf(path, sizeof(path), "%s/*", cdir);
    xim=XmStringCreateLtoR(path,XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues(cdata->widget, XmNdirMask, xim,  NULL);
    XmFileSelectionDoSearch(cdata->widget, xim);
    XmStringFree(xim);

    if (cdir[dirLen] != '/') {
	separator = "/";
    } else {
	separator = "";
    }
    jio_snprintf(path, sizeof(path), "%s%s%s",
		 cdir, separator, makeCString(file));

    xim=XmStringCreateLtoR(path,XmSTRING_DEFAULT_CHARSET);
    XtVaSetValues(cdata->widget, XmNdirSpec, xim,  NULL);
    XmStringFree(xim);

    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MFileDialogPeer_pReshape(struct Hsun_awt_motif_MFileDialogPeer *this,
				       long x, long y,
				       long w, long h)
{
    struct FrameData		*wdata;

    AWT_LOCK();
    wdata = PDATA(FrameData,this);
    if (wdata == 0 || wdata->winData.shell==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    XtVaSetValues(wdata->winData.shell,
		  XtNx, (XtArgVal) x,
		  XtNy, (XtArgVal) y,
		  NULL);

    AWT_FLUSH_UNLOCK();
}
