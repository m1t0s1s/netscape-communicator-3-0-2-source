/*
 * @(#)awt_util.c	1.18 95/11/27 Sami Shaio
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
#include "color.h"
#include <X11/IntrinsicP.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>

static Widget MakeAppletSecurityChrome(Widget parent, char *warning)
{
    Widget warningWindow;
    Widget label;
    int argc;
    Arg args[10];
    Pixel yellow = awtImage->ColorMatch(255, 225, 0);
    Pixel black = awtImage->ColorMatch(0, 0, 0);

    argc = 0;
    XtSetArg(args[argc], XmNspacing, 0); argc++;
    XtSetArg(args[argc], XmNbackground, yellow); argc++;
    XtSetArg(args[argc], XmNheight, 20); argc++;
    XtSetArg(args[argc], XmNwidth, 200); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    warningWindow =  XmCreateForm(parent, "main", args, argc);

    XtManageChild(warningWindow);
    label = XtVaCreateManagedWidget(warning,
				    xmLabelWidgetClass, warningWindow,
				    XmNhighlightThickness, 0,
				    XmNbackground, yellow,
				    XmNforeground, black,
				    XmNalignment, XmALIGNMENT_CENTER,
				    XmNrecomputeSize, False,
				    NULL);
    XtVaSetValues(label,
		  XmNbottomAttachment, XmATTACH_FORM,
		  XmNtopAttachment, XmATTACH_FORM,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  NULL);
    return warningWindow;
}

Widget (*awt_MakeAppletSecurityChrome)(Widget parent, char *message) = MakeAppletSecurityChrome;

Widget
awt_util_createWarningWindow(Widget parent, char *warning)
{
    return (*awt_MakeAppletSecurityChrome)(parent, warning);
}

void
awt_setWidgetGravity(Widget w, int gravity)
{
    XSetWindowAttributes    xattr;
    Window  win = XtWindow(w);

    if (win != 0) {
	xattr.bit_gravity = StaticGravity;
	xattr.win_gravity = StaticGravity;
	XChangeWindowAttributes(XtDisplay(w), win,
				CWBitGravity|CWWinGravity,
				&xattr);
    }
}

void
awt_util_reshape(Widget w, long x, long y, long wd, long ht) 
{
    if (w == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    XtUnmanageChild(w);

    /* AVH: This is a horrible hack. Motif doesn't actually
     * do anything when you move a component to 0, 0
     */
    if ((x == 0) && (y == 0)) {
	XtVaSetValues(w, XmNx, 1, XmNy, 1, NULL);
    }
    XtVaSetValues(w,
		  XmNx, x,
		  XmNy, y,
		  XmNwidth, (wd > 0) ? wd : 1,
		  XmNheight, (ht > 0) ? ht : 1,
		  NULL);
    XtManageChild(w);
}

void
awt_util_hide(Widget w) 
{
    if (w == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    XtSetMappedWhenManaged(w, False);
}

void
awt_util_show(Widget w) 
{
/*
    extern Boolean  scrollBugWorkAround;
*/
    if (w == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    XtSetMappedWhenManaged(w, True);
/*
  XXX: causes problems on 2.5
    if (!scrollBugWorkAround) {
	awt_setWidgetGravity(w, StaticGravity);
    }
*/
}

void
awt_util_enable(Widget w) 
{
    XtSetSensitive(w, True);
}

void
awt_util_disable(Widget w) 
{
    XtSetSensitive(w, False);
}

void
awt_util_mapChildren(Widget w, void (*func)(Widget,void *),
		     int applyToCurrent, void *data) {
    WidgetList			wlist;
    Cardinal			wlen = 0;
    int				i;

    if (applyToCurrent != 0) {
	(*func)(w, data);
    }
    if (!XtIsComposite(w)) {
	return;
    }

    XtVaGetValues(w,
		  XmNchildren, &wlist,
		  XmNnumChildren, &wlen,
		  NULL);
    if (wlen > 0) {
	for (i=0; i < wlen; i++) {
	    awt_util_mapChildren(wlist[i], func, 1, data);
	}
    }
}

void
awt_changeAttributes(Display *dpy, Widget w, unsigned long mask,
		     XSetWindowAttributes *xattr)
{
    int			i;
    WidgetList		wlist;
    Cardinal		wlen = 0;

    if (XtWindow(w) && XtIsRealized(w)) {
	XChangeWindowAttributes(dpy,
				XtWindow(w),
				mask,
				xattr);
    } else {
	return;
    }
    XtVaGetValues(w,
		  XmNchildren, &wlist,
		  XmNnumChildren, &wlen,
		  NULL);
    for (i = 0; i < wlen; i++) {
	if (XtWindow(wlist[i]) && XtIsRealized(wlist[i])) {
	    XChangeWindowAttributes(dpy,
				    XtWindow(wlist[i]),
				    mask,
				    xattr);
	}
    }
}
