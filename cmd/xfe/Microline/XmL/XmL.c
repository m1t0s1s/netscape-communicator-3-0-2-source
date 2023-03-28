/*
(c) Copyright 1994, 1995, Microline Software, Inc.  ALL RIGHTS RESERVED
  
THIS PROGRAM BELONGS TO MICROLINE SOFTWARE.  IT IS CONSIDERED A TRADE
SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT
RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.

THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE COPIED AND USED 
ONLY IN ACCORDANCE WITH THE TERMS OF THAT LICENSE AND WITH THE INCLUSION
OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE AND DOCUMENTATION, AND ITS 
COPYRIGHTS ARE OWNED BY MICROLINE SOFTWARE AND ARE PROTECTED BY UNITED
STATES COPYRIGHT LAWS AND INTERNATIONAL TREATY PROVISIONS.
 
THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE
AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY MICROLINE SOFTWARE.

THIS SOFTWARE AND REFERENCE MATERIALS ARE PROVIDED "AS IS" WITHOUT
WARRANTY AS TO THEIR PERFORMANCE, MERCHANTABILITY, FITNESS FOR ANY 
PARTICULAR PURPOSE, OR AGAINST INFRINGEMENT.  MICROLINE SOFTWARE
ASSUMES NO RESPONSIBILITY FOR THE USE OR INABILITY TO USE THIS 
SOFTWARE.

MICROLINE SOFTWARE SHALL NOT BE LIABLE FOR INDIRECT, SPECIAL OR
CONSEQUENTIAL DAMAGES RESULTING FROM THE USE OF THIS PRODUCT. SOME 
STATES DO NOT ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR
CONSEQUENTIAL DAMAGES, SO THE ABOVE LIMITATIONS MIGHT NOT APPLY TO
YOU.

MICROLINE SOFTWARE SHALL HAVE NO LIABILITY OR RESPONSIBILITY FOR SOFTWARE
ALTERED, MODIFIED, OR CONVERTED BY YOU OR A THIRD PARTY, DAMAGES
RESULTING FROM ACCIDENT, ABUSE OR MISAPPLICATION, OR FOR PROBLEMS DUE
TO THE MALFUNCTION OF YOUR EQUIPMENT OR SOFTWARE NOT SUPPLIED BY
MICROLINE SOFTWARE.
  
U.S. GOVERNMENT RESTRICTED RIGHTS
This Software and documentation are provided with RESTRICTED RIGHTS.
Use, duplication or disclosure by the Government is subject to
restrictions as set forth in subparagraph (c)(1) of the Rights in
Technical Data and Computer Software Clause at DFARS 252.227-7013 or
subparagraphs (c)(1)(ii) and (2) of Commercial Computer Software -
Restricted Rights at 48 CFR 52.227-19, as applicable, supplier is
Microline Software, 41 Sutter St Suite 1374, San Francisco, CA 94104.
*/

#include <XmL/XmL.h>
#include <Xm/XmP.h>
#include <Xm/LabelP.h>
#include <Xm/DrawnBP.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#ifdef MOTIF11
#include <Xm/VendorE.h>
#else
#include <Xm/VendorS.h>
#endif
#include <Xm/BulletinB.h>
#include <Xm/MenuShell.h>

#include <stdlib.h>
#include <stdio.h>

#if defined(_STDC_) || defined(__cplusplus) || defined(c_plusplus)

static void XmLDrawnBDestroyCB(Widget w, XtPointer clientData, XtPointer);
static void XmLDrawnBDrawCB(Widget, XtPointer, XtPointer);
static void XmLDrawnBDrawStringCB(Widget, XtPointer, XtPointer);
static void XmLFontGetAverageWidth(XFontStruct *fs, short *width);
static void XmLMessageBoxResponse(Widget, XtPointer, XtPointer);
static void XmLMessageBoxWMDelete(Widget, XtPointer, XtPointer);
static void XmLSortFunc(char *lvec, char *rvec);

#else

static void XmLDrawnBDestroyCB();
static void XmLDrawnBDrawCB();
static void XmLDrawnBDrawStringCB();
static void XmLFontGetAverageWidth();
static void XmLMessageBoxResponse();
static void XmLMessageBoxWMDelete();
static void XmLSortFunc();

#endif

int XmLDateDaysInMonth(m, y)
int m, y;
{
	static int d[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

	if (m < 1 || m > 12 || y < 1753 || y > 9999)
		return -1;
	if (m == 2 && (!(y % 4) && (y % 100) || !(y % 400)))
		return 29;
	return d[m - 1];
}

/* Calculation from Communications Of The ACM, Vol 6, No 8, p 444 */
/* sun is 0, sat is 6 */
int XmLDateWeekDay(m, d, y)
int m, d, y;
{
	long jd, j1, j2;

	if (m < 1 || m > 12 || d < 1 || d > XmLDateDaysInMonth(m, y) ||
		y < 1753 || y > 9999)
		return -1;
	if (m > 2)
		m -= 3;
	else
		{
		m += 9;
		y--;
		}
	j1 = y / 100;
	j2 = y - 100 * j1;
	jd = (146097 * j1) / 4 + (1461 * j2) / 4 + (153 * m + 2) / 5 +
		1721119 + d;
        return (jd + 1) % 7;
}

typedef struct
	{
	GC gc;
	int type;
	int dir;
	XFontStruct *fontStruct;
	} XmLDrawnBData;

void XmLDrawnButtonSetType(w, drawnType, drawnDir)
Widget w;
int drawnType, drawnDir;
	{
	Display *dpy;
	XmLDrawnBData *dd;
	XmDrawnButtonWidget b;
	XmString str;
	XmFontList fontlist;
	XGCValues values;
	XtGCMask mask;
	Dimension width, height, dim;
	Dimension highlightThickness, shadowThickness;
	Dimension marginWidth, marginHeight;
	Dimension marginTop, marginBottom, marginLeft, marginRight;

	if (!XtIsSubclass(w, xmDrawnButtonWidgetClass))
		{
		XmLWarning(w, "DrawnButtonSetType() - not an XmDrawnButton");
		return;
		}
	XtVaSetValues(w,
		XmNpushButtonEnabled, True,
		NULL);
	XtRemoveAllCallbacks(w, XmNexposeCallback);
	XtRemoveAllCallbacks(w, XmNresizeCallback);
	if (drawnType == XmDRAWNB_STRING && drawnDir == XmDRAWNB_RIGHT)
		{
		XtVaSetValues(w,
			XmNlabelType, XmSTRING,
			NULL);
		return;
		}
	b = (XmDrawnButtonWidget)w;
	dd = (XmLDrawnBData *)malloc(sizeof(XmLDrawnBData));
	dd->type = drawnType;
	dd->dir = drawnDir;
	if (dd->type == XmDRAWNB_STRING)
		{
		XtVaGetValues(w,
			XmNlabelString, &str,
			XmNfontList, &fontlist,
			XmNhighlightThickness, &highlightThickness,
			XmNshadowThickness, &shadowThickness,
			XmNmarginHeight, &marginHeight,
			XmNmarginWidth, &marginWidth,
			XmNmarginTop, &marginTop,
			XmNmarginBottom, &marginBottom,
			XmNmarginLeft, &marginLeft,
			XmNmarginRight, &marginRight,
			NULL);
		if (!str && XtName(w))
			str = XmStringCreateSimple(XtName(w));
		if (!str)
			str = XmStringCreateSimple("");
		XmStringExtent(fontlist, str, &width, &height);
		XmStringFree(str);
		if (drawnDir == XmDRAWNB_UP || drawnDir == XmDRAWNB_DOWN)
			{
			dim = width;
			width = height;
			height = dim;
			}
		height += (highlightThickness + shadowThickness +
			marginHeight) * 2 + marginTop + marginBottom;
		width += (highlightThickness + shadowThickness +
			marginWidth) * 2 + marginLeft + marginRight;
		/* change to pixmap type so label string isnt drawn */
		XtVaSetValues(w,
			XmNlabelType, XmPIXMAP,
			NULL);
		XtVaSetValues(w,
			XmNwidth, width,
			XmNheight, height,
			NULL);
		dpy = XtDisplay(w);
		dd->gc = XCreateGC(dpy, DefaultRootWindow(dpy), 0, NULL);
		dd->fontStruct = XLoadQueryFont(dpy, "fixed");
		if (!dd->fontStruct)
			{
			XmLWarning(w, "DrawnButtonSetType() - FATAL can't load fixed font");
			return;
			}
		XSetFont(dpy, dd->gc, dd->fontStruct->fid);
		XtAddCallback(w, XmNexposeCallback, XmLDrawnBDrawStringCB,
			(XtPointer)dd);
		XtAddCallback(w, XmNresizeCallback, XmLDrawnBDrawStringCB,
			(XtPointer)dd);
		}
	else
		{
		mask = GCForeground;
		values.foreground = b->primitive.foreground;
		dd->gc = XtGetGC(w, mask, &values);
		XtAddCallback(w, XmNexposeCallback, XmLDrawnBDrawCB, (XtPointer)dd);
		XtAddCallback(w, XmNresizeCallback, XmLDrawnBDrawCB, (XtPointer)dd);
		}
	XtAddCallback(w, XmNdestroyCallback, XmLDrawnBDestroyCB, (XtPointer)dd);
	}

static void XmLDrawnBDestroyCB(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLDrawnBData *dd;

	dd = (XmLDrawnBData *)clientData;
	if (dd->type == XmDRAWNB_STRING)
		{
		XFreeGC(XtDisplay(w), dd->gc);
		XFreeFont(XtDisplay(w), dd->fontStruct);
		}
	else
		XtReleaseGC(w, dd->gc);
	free((char *)dd);
	}

static void XmLDrawnBDrawStringCB(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLDrawnBData *dd;
	XmFontList fontlist;
	XmString str;
	XmStringDirection stringDir;
	unsigned char drawDir, alignment;
	int width, height, xoff, yoff, drawWidth;
	Pixel fg;
	Dimension highlightThickness;
	Dimension shadowThickness, marginWidth, marginHeight;
	Dimension marginLeft, marginRight, marginTop, marginBottom;

	if (!XtIsRealized(w))
		return;
	dd = (XmLDrawnBData *)clientData;
	XtVaGetValues(w,
		XmNlabelString, &str,
		NULL);
	if (!str && XtName(w))
		str = XmStringCreateSimple(XtName(w));
	if (!str)
		return;
	XtVaGetValues(w,
		XmNforeground, &fg,
		XmNfontList, &fontlist,
		XmNalignment, &alignment,
		XmNhighlightThickness, &highlightThickness,
		XmNshadowThickness, &shadowThickness,
		XmNmarginWidth, &marginWidth,
		XmNmarginHeight, &marginHeight, 
		XmNmarginLeft, &marginLeft,
		XmNmarginRight, &marginRight,
		XmNmarginTop, &marginTop,
		XmNmarginBottom, &marginBottom,
		NULL);
	xoff = highlightThickness + shadowThickness + marginLeft + marginWidth;
	yoff = highlightThickness + shadowThickness + marginTop + marginHeight;
	width = XtWidth(w) - xoff - xoff + marginLeft - marginRight;
	height = XtHeight(w) - yoff - yoff + marginTop - marginBottom;
	if (XmIsManager(XtParent(w)))
		XtVaGetValues(XtParent(w),
			XmNstringDirection, &stringDir,
			NULL);
	else
		stringDir = XmSTRING_DIRECTION_L_TO_R;
	switch (dd->dir)
		{
		case XmDRAWNB_LEFT:
			drawDir = XmSTRING_LEFT;
			break;
		case XmDRAWNB_UP:
			drawDir = XmSTRING_UP;
			break;
		case XmDRAWNB_DOWN:
			drawDir = XmSTRING_DOWN;
			break;
		default:
			drawDir = XmSTRING_RIGHT;
			break;
		}
	if (drawDir == XmSTRING_LEFT || drawDir == XmSTRING_RIGHT)
		drawWidth = width;
	else
		drawWidth = height;
	XSetForeground(XtDisplay(w), dd->gc, fg);
	XmLStringDrawDirection(XtDisplay(w), XtWindow(w), fontlist,
		str, dd->gc, xoff, yoff, drawWidth, alignment, stringDir, drawDir);
	XmStringFree(str);
	}

static void XmLDrawnBDrawCB(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	XmLDrawnBData *dd;
	XmDrawnButtonWidget b;
	unsigned char drawDir;
	unsigned char alignment;
	Display *dpy;
	Window win;
	GC gc;
	XPoint p[2][5];
	XSegment seg;
	int np[2];
	int i, j, temp;
	int md, type, dir;
	int avgx, avgy, xoff, yoff, st;

	if (!XtIsRealized(w))
		return;
	dd = (XmLDrawnBData *)clientData;
	type = dd->type;
	dir = dd->dir;
	gc = dd->gc;
	b = (XmDrawnButtonWidget)w;
	win = XtWindow(w);
	dpy = XtDisplay(w);
	st = b->primitive.shadow_thickness;
	i = st * 2 + b->primitive.highlight_thickness * 2;
	/* calculate max dimension */
	md = XtWidth(w) - i;
	if (md > ((int)XtHeight(w) - i))
		md = XtHeight(w) - i;
	if (md < 4)
		return;
	xoff = ((int)XtWidth(w) - md) / 2;
	yoff = ((int)XtHeight(w) - md) / 2;
	np[0] = 0;
	np[1] = 0;
	switch (type)
		{
		case XmDRAWNB_ARROW:
			p[0][0].x = md / 6;
			p[0][0].y = md / 6;
			p[0][1].x = md / 6;
			p[0][1].y = md - md / 6;
			p[0][2].x = md - md / 6;
			p[0][2].y = md / 2;
			np[0] = 3;
			break;
		case XmDRAWNB_ARROWLINE:
			p[0][0].x = md / 5;
			p[0][0].y = md / 5;
			p[0][1].x = md / 5;
			p[0][1].y = md - md / 5;
			p[0][2].x = md - md / 5;
			p[0][2].y = md / 2;
			np[0] = 3;
			p[1][0].x = md - md / 5 + 1;
			p[1][0].y = md / 5;
			p[1][1].x = md - md / 5 + 1;
			p[1][1].y = md - md / 5;
			p[1][2].x = md - md / 10;
			p[1][2].y = md - md / 5;
			p[1][3].x = md - md / 10;
			p[1][3].y = md / 5;
			np[1] = 4;
			break;
		case XmDRAWNB_DOUBLEARROW:
			/* odd major dimensions can give jagged lines */
			if (md % 2)
				md += 1;
			p[0][0].x = md / 10;
			p[0][0].y = md / 10;
			p[0][1].x = md / 10;
			p[0][1].y = md - md / 10;
			p[0][2].x = md / 2;
			p[0][2].y = md / 2;
			np[0] = 3;
			p[1][0].x = md - md / 2;
			p[1][0].y = md / 10;
			p[1][1].x = md - md / 2;
			p[1][1].y = md - md / 10;
			p[1][2].x = md - md / 10;
			p[1][2].y = md / 2;
			np[1] = 3;
			break;
		case XmDRAWNB_SQUARE:
			p[0][0].x = md / 3;
			p[0][0].y = md / 3;
			p[0][1].x = md / 3;
			p[0][1].y = md - md / 3;
			p[0][2].x = md - md / 3;
			p[0][2].y = md - md / 3;
			p[0][3].x = md - md / 3;
			p[0][3].y = md / 3;
			np[0] = 4;
			break;
		case XmDRAWNB_DOUBLEBAR:
			p[0][0].x = md / 3;
			p[0][0].y = md / 4;
			p[0][1].x = md / 3;
			p[0][1].y = md - md / 4;
			p[0][2].x = md / 2 - md / 10;
			p[0][2].y = md - md / 4;
			p[0][3].x = md / 2 - md / 10;
			p[0][3].y = md / 4;
			np[0] = 4;
			p[1][0].x = md - md / 3;
			p[1][0].y = md / 4;
			p[1][1].x = md - md / 3;
			p[1][1].y = md - md / 4;
			p[1][2].x = md - md / 2 + md / 10;
			p[1][2].y = md - md / 4;
			p[1][3].x = md - md / 2 + md / 10;
			p[1][3].y = md / 4;
			np[1] = 4;
			break;
		}
	for (i = 0; i < 2; i++)
		{
		avgx = 0;
		avgy = 0;
		for (j = 0; j < np[i]; j++)
			{
			switch (dir)
				{
				case XmDRAWNB_RIGHT:
					/* points unchanged */
					break;
				case XmDRAWNB_LEFT:
					p[i][j].x = md - p[i][j].x;
					break;
				case XmDRAWNB_UP:
					temp = p[i][j].x;
					p[i][j].x = p[i][j].y;
					p[i][j].y = md - temp;
					break;
				case XmDRAWNB_DOWN:
					temp = p[i][j].x;
					p[i][j].x = p[i][j].y;
					p[i][j].y = temp;
					break;
			}
			p[i][j].x += xoff;
			p[i][j].y += yoff;
			avgx += p[i][j].x;
			avgy += p[i][j].y;
			}
		if (!np[i])
			continue;
		avgx /= np[i];
		avgy /= np[i];
		XFillPolygon(dpy, win, gc, p[i], np[i], Nonconvex, CoordModeOrigin);
		p[i][np[i]].x = p[i][0].x;
		p[i][np[i]].y = p[i][0].y;
		for (j = 0; j < np[i]; j++)
			{
			seg.x1 = p[i][j].x;
			seg.y1 = p[i][j].y;
			seg.x2 = p[i][j + 1].x;
			seg.y2 = p[i][j + 1].y;
			if ((seg.x1 <= avgx && seg.x2 <= avgx) ||
				(seg.y1 <= avgy && seg.y2 <= avgy))
				XDrawSegments(dpy, win,
					b->primitive.bottom_shadow_GC, &seg, 1);
			else
				XDrawSegments(dpy, win,
					b->primitive.top_shadow_GC, &seg, 1);
			}
		}
	}

XmFontList XmLFontListCopyDefault(widget)
Widget widget;
	{
	Widget parent;
	XFontStruct *font;
	XmFontList fontList, fl;

	fontList = 0;
	parent = XtParent(widget);
	while (parent)
		{
		fl = 0;
		if (XmIsVendorShell(parent) || XmIsMenuShell(parent))
			XtVaGetValues(parent, XmNdefaultFontList, &fl, NULL);
		else if (XmIsBulletinBoard(parent))
			XtVaGetValues(parent, XmNbuttonFontList, &fl, NULL);
		if (fl)
			{
			fontList = XmFontListCopy(fl);
			parent = 0;
			}
		if (parent)
			parent = XtParent(parent);
		}
	if (!fontList)
		{
		font = XLoadQueryFont(XtDisplay(widget), "fixed");
		if (!font)
			XmLWarning(widget,
				"FontListCopyDefault() - FATAL ERROR - can't load fixed font");
		fontList = XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
		}
	return fontList;
	}

void XmLFontListGetDimensions(fontList, width, height, useAverageWidth)
XmFontList fontList;
short *width, *height;
Boolean useAverageWidth;
	{
	XmStringCharSet charset;
	XmFontContext context;
	XFontStruct *fs;
	short w, h;
	/* --- begin code to work around Motif internal bug */
	typedef struct {
		XmFontList nextFontList;
		Boolean unused;
	} XmFontListContextRec;
	typedef struct {
		XFontStruct *font;
		XmStringCharSet unused;
	} XmFontListRec;
	XmFontList nextFontList;
	/* --- end Motif workaround code */

	*width = 0;
	*height = 0;
	if (XmFontListInitFontContext(&context, fontList))
		{
		while (1)
			{
			/* --- begin code to work around Motif internal bug */
			/* --- this code can be removed for Motif 2.0    */
			nextFontList = ((XmFontListContextRec *)context)->nextFontList;
			if (!nextFontList)
				break;
			if (!((XmFontListRec *)nextFontList)->font)
				break;
			/* --- end Motif workaround code */
			if (XmFontListGetNextFont(context, &charset, &fs) == False)
				break;
			XtFree(charset);
			if (useAverageWidth == True)
				XmLFontGetAverageWidth(fs, &w);
			else
				w = fs->max_bounds.width;
			h = fs->max_bounds.ascent + fs->max_bounds.descent;
			if (*height < h)
				*height = h;
			if (*width < w)
				*width = w;
			}	
		XmFontListFreeFontContext(context);
		}
	}

static void XmLFontGetAverageWidth(fs, width)
XFontStruct *fs;
short *width;
	{
	long aw, n;
	int r, c, mm, i;
	XCharStruct *cs;

	n = 0;
	aw = 0;
	mm = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;
	for (r = fs->min_byte1; r <= fs->max_byte1; r++)
		for (c = fs->min_char_or_byte2; c <= fs->max_char_or_byte2; c++)
			{
			if (!fs->per_char)
				continue;
			i = ((r - fs->min_byte1) * mm) + (c - fs->min_char_or_byte2);
			cs = &fs->per_char[i];
			if (!cs->width)
				continue;
			aw += cs->width;
			n++;
			}
	if (n)
		aw = aw / n;
	else
		aw = fs->min_bounds.width;
	*width = (short)aw;
	}

int XmLMessageBox(w, string, okOnly)
Widget w;
char *string;
Boolean okOnly;
	{
	int status = 0;
	Widget dialog, shell;
	Arg args[3];
	XtAppContext context;
	XmString str, titleStr;
	String shellTitle;
	Atom WM_DELETE_WINDOW;

#ifdef MOTIF11
	str = XmStringCreateLtoR(string, XmSTRING_DEFAULT_CHARSET);
#else
	str = XmStringCreateLocalized(string);
#endif
	XtSetArg(args[0], XmNmessageString, str);
	XtSetArg(args[1], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
	shell = XmLShellOfWidget(w);
	if (shell)
		XtVaGetValues(shell, XmNtitle, &shellTitle, NULL);
	if (shell && shellTitle)
		titleStr = XmStringCreateLtoR(shellTitle,
			XmSTRING_DEFAULT_CHARSET);
	else
		titleStr = XmStringCreateLtoR("Notice",
			XmSTRING_DEFAULT_CHARSET);
	XtSetArg(args[2], XmNdialogTitle, titleStr);
	if (okOnly == True)
		dialog = XmCreateMessageDialog(XtParent(w), "popup", args, 3);
	else
		dialog = XmCreateQuestionDialog(XtParent(w), "popup", args, 3);
	WM_DELETE_WINDOW = XmInternAtom(XtDisplay(w), "WM_DELETE_WINDOW",
		False);
	XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, XmLMessageBoxWMDelete,
		(caddr_t)&status);
	XmStringFree(str);
	XmStringFree(titleStr);
	XtAddCallback(dialog, XmNokCallback, XmLMessageBoxResponse,
		(XtPointer)&status);
	if (okOnly == True)
		{
		XtUnmanageChild(XmMessageBoxGetChild(dialog,
			XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(dialog,
			XmDIALOG_HELP_BUTTON));
		}
	else
		{
		XtAddCallback(dialog, XmNcancelCallback, XmLMessageBoxResponse,
			(XtPointer)&status);
		XtAddCallback(dialog, XmNhelpCallback, XmLMessageBoxResponse,
			(XtPointer)&status);
		}
	XtManageChild(dialog);

	context = XtWidgetToApplicationContext(w);
	while (!status ||  XtAppPending(context))
		XtAppProcessEvent(context, XtIMAll);
	XtDestroyWidget(dialog);
	return status;
	}

static void XmLMessageBoxWMDelete(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	int *status = (int *)clientData;
	*status = 1;
	}

static void XmLMessageBoxResponse(w, clientData, callData)
Widget w;
XtPointer clientData, callData;
	{
	int *status = (int *)clientData;
	XmAnyCallbackStruct *reason;

	reason = (XmAnyCallbackStruct *)callData;
	switch (reason->reason)
		{
		case XmCR_OK:
			*status = 1;
			break;
		case XmCR_CANCEL:
			*status = 2;
			break;
		case XmCR_HELP:
			*status = 3;
			break;
		}
	}

void XmLPixmapDraw(w, pixmap, pixmapWidth, pixmapHeight, alignment,
gc, rect, clipRect)
Widget w;
Pixmap pixmap;
int pixmapWidth, pixmapHeight;
unsigned char alignment;
GC gc;
XRectangle *rect, *clipRect;
	{
	Display *dpy;
	Window win;
	int needsClip;
	int x, y, width, height;

	if (pixmap == XmUNSPECIFIED_PIXMAP)
		return;
	if (rect->width <= 4 || rect->height <= 4)
		return;
	if (clipRect->width < 3 || clipRect->height < 3)
		return;
	if (!XtIsRealized(w))
		return;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	width = pixmapWidth;
	height = pixmapHeight;
	if (!width || !height)
		{
		alignment = XmALIGNMENT_TOP_LEFT;
		width = clipRect->width - 4;
		height = clipRect->height - 4;
		}

	needsClip = 1;
	if (clipRect->x == rect->x &&
		clipRect->y == rect->y &&
		clipRect->width == rect->width &&
		clipRect->height == rect->height &&
		width + 4 <= (int)clipRect->width &&
		height + 4 <= (int)clipRect->height)
		needsClip = 0;

	if (alignment == XmALIGNMENT_TOP_LEFT ||
		alignment == XmALIGNMENT_LEFT ||
		alignment == XmALIGNMENT_BOTTOM_LEFT)
		x = rect->x + 2;
	else if (alignment == XmALIGNMENT_TOP ||
		alignment == XmALIGNMENT_CENTER ||
		alignment == XmALIGNMENT_BOTTOM)
		x = rect->x + ((int)rect->width - width) / 2;
	else
		x = rect->x + rect->width - width - 2;

	if (alignment == XmALIGNMENT_TOP ||
		alignment == XmALIGNMENT_TOP_LEFT ||
		alignment == XmALIGNMENT_TOP_RIGHT)
		y = rect->y + 2;
	else if (alignment == XmALIGNMENT_LEFT ||
		alignment == XmALIGNMENT_CENTER ||
		alignment == XmALIGNMENT_RIGHT)
		y = rect->y + ((int)rect->height - height) / 2;
	else
		y = rect->y + rect->height - height - 2;

	if (needsClip)
		XSetClipRectangles(dpy, gc, 0, 0, clipRect, 1, Unsorted);
	XSetGraphicsExposures(dpy, gc, False);
	XCopyArea(dpy, pixmap, win, gc, 0, 0, width, height, x, y);
	XSetGraphicsExposures(dpy, gc, True);
	if (needsClip)
		XSetClipMask(dpy, gc, None);
	}

int XmLRectIntersect(r1, r2)
XRectangle *r1;
XRectangle *r2;
	{
	if (!r1->width || !r1->height || !r2->width || !r2->height)
		return XmLRectOutside;
	if (r1->x + (int)r1->width - 1 < r2->x ||
		r1->x > r2->x + (int)r2->width - 1 ||
		r1->y + (int)r1->height - 1 < r2->y ||
		r1->y > r2->y + (int)r2->height - 1)
		return XmLRectOutside;
	if (r1->x >= r2->x &&
		r1->x + (int)r1->width <= r2->x + (int)r2->width &&
		r1->y >= r2->y &&
		r1->y + (int)r1->height <= r2->y + (int)r2->height)
		return XmLRectInside; /* r1 inside r2 */
	return XmLRectPartial;
	}

Widget XmLShellOfWidget(w)
Widget w;
	{
	while(1)
		{
		if (!w)
			return 0;
		if (XtIsSubclass(w, shellWidgetClass))
			return w;
		w = XtParent(w);
		}	
	}

static XmLSortCompareFunc XmLSortCompare;
static int XmLSortEleSize;
static void *XmLSortUserData;

void XmLSort(base, numItems, itemSize, compare, userData)
void *base;
int numItems;
unsigned int itemSize;
XmLSortCompareFunc compare;
void *userData;
	{
	XmLSortCompareFunc oldCompare;
	int oldEleSize;
	void *oldUserData;
	char *lvec, *rvec;

	if (numItems < 2)
		return;

	/* for sorts within a sort compare function, we must
	   save any global sort variables on the local stack
	   and restore them when finished */
	oldCompare = XmLSortCompare;
	oldEleSize = XmLSortEleSize;
	oldUserData = XmLSortUserData;
	XmLSortCompare = compare;
	XmLSortEleSize = itemSize;
	XmLSortUserData = userData;

	lvec = (char *)base;
	rvec = lvec + (numItems - 1) * itemSize;
	XmLSortFunc(lvec, rvec);

	XmLSortCompare = oldCompare;
	XmLSortEleSize = oldEleSize;
	XmLSortUserData = oldUserData;
	}

#define SWAP(p1, p2) \
		{ \
		if (p1 != p2) \
			{ \
			int zi; \
			char zc; \
			for (zi = 0; zi < XmLSortEleSize; zi++) \
				{ \
				zc = (p1)[zi]; \
				(p1)[zi] = (p2)[zi]; \
				(p2)[zi] = zc; \
				} \
			}\
		}

static void XmLSortFunc(lvec, rvec)
char *lvec, *rvec;
	{
	int i;
	char *nlvec, *nrvec, *pvec;

start:
	i = (*XmLSortCompare)(XmLSortUserData, lvec, rvec);

	/* two item sort */
	if (rvec == lvec + XmLSortEleSize)
		{
		if (i > 0)
			SWAP(lvec, rvec)
		return;
		}

	/* find mid of three items */
	pvec = lvec + ((rvec - lvec) / (XmLSortEleSize * 2)) * XmLSortEleSize;
	if (i < 0)
		{
		i = (*XmLSortCompare)(XmLSortUserData, lvec, pvec);
		if (i > 0)
			pvec = lvec;
		else if (i == 0)
			pvec = rvec;
		}
	else if (i > 0)
		{
		i = (*XmLSortCompare)(XmLSortUserData, rvec, pvec);
		if (i > 0)
			pvec = rvec;
		else if (i == 0)
			pvec = lvec;
		}
	else
		{
		pvec = lvec + XmLSortEleSize;
		while (1)
			{
			i = (*XmLSortCompare)(XmLSortUserData, lvec, pvec);
			if (i < 0)
				break;
			else if (i > 0)
				{
				pvec = lvec;
				break;
				}
			if (pvec == rvec)
				return;
			pvec += XmLSortEleSize;
			}
		}

	/* partition the set */
	nlvec = lvec;
	nrvec = rvec;
	while (1)
		{
		if (pvec == nrvec)
			pvec = nlvec;
		else if (pvec == nlvec)
			pvec = nrvec;
		SWAP(nrvec, nlvec)
		while ((*XmLSortCompare)(XmLSortUserData, nlvec, pvec) < 0)
			nlvec += XmLSortEleSize;
		while ((*XmLSortCompare)(XmLSortUserData, nrvec, pvec) >= 0)
			nrvec -= XmLSortEleSize;
		if (nlvec > nrvec)
			break;
		}

	/* sort partitioned sets */
	if (lvec < nlvec - XmLSortEleSize)
		XmLSortFunc(lvec, nlvec - XmLSortEleSize);
	if (nlvec < rvec)
		{
		lvec = nlvec;
		goto start;
		}
	}

void XmLStringDraw(w, string, stringDir, fontList, alignment,
gc, rect, clipRect)
Widget w;
XmString string;
XmStringDirection stringDir;
XmFontList fontList;
unsigned char alignment;
GC gc;
XRectangle *rect;
XRectangle *clipRect;
	{
	Display *dpy;
	Window win;
	Dimension width, height;
	int x, y, needsClip;
	unsigned char strAlignment;

	if (!string)
		return;
	if (rect->width <= 4 || rect->height <= 4)
		return;
	if (!XtIsRealized(w))
		return;
	dpy = XtDisplay(w);
	win = XtWindow(w);
	if (alignment == XmALIGNMENT_LEFT ||
		alignment == XmALIGNMENT_TOP_LEFT ||
		alignment == XmALIGNMENT_BOTTOM_LEFT)
		strAlignment = XmALIGNMENT_BEGINNING;
	else if (alignment == XmALIGNMENT_CENTER ||
		alignment == XmALIGNMENT_TOP ||
		alignment == XmALIGNMENT_BOTTOM)
		strAlignment = XmALIGNMENT_CENTER;
	else
		strAlignment = XmALIGNMENT_END;
	XmStringExtent(fontList, string, &width, &height);
	needsClip = 1;
	if (clipRect->x == rect->x &&
		clipRect->y == rect->y &&
		clipRect->width == rect->width &&
		clipRect->height == rect->height &&
		(int)width + 4 <= (int)clipRect->width &&
		(int)height + 4 <= (int)clipRect->height)
		needsClip = 0;
	x = rect->x + 2;
	if (alignment == XmALIGNMENT_TOP ||
		alignment == XmALIGNMENT_TOP_LEFT ||
		alignment == XmALIGNMENT_TOP_RIGHT)
		y = rect->y + 2;
	else if (alignment == XmALIGNMENT_LEFT ||
		alignment == XmALIGNMENT_CENTER ||
		alignment == XmALIGNMENT_RIGHT)
		y = rect->y + ((int)rect->height - height) / 2;
	else
		y = rect->y + rect->height - height - 2;
	/* XmStringDraw clipping doesnt work in all cases
	   so we use a clip region for clipping */
	if (needsClip)
		XSetClipRectangles(dpy, gc, 0, 0, clipRect, 1, Unsorted);
	XmStringDraw(dpy, win, fontList, string, gc,
		x, y, rect->width - 4, strAlignment, stringDir, clipRect);
	if (needsClip)
		XSetClipMask(dpy, gc, None);
	}

void XmLStringDrawDirection(dpy, win, fontlist, string, gc, x, y,
	width, alignment, layout_direction, drawing_direction)
Display *dpy;
Window win;
XmFontList fontlist;
XmString string;
GC gc;
int x, y;
Dimension width;
unsigned char alignment;
unsigned char layout_direction;
unsigned char drawing_direction;
{
	Screen *screen;
	XFontStruct *fontStruct;
	XImage *sourceImage, *destImage;
	Pixel black, white;
	Pixmap pixmap;
	GC pixmapGC;
	int sourceWidth, sourceHeight;
	int destWidth, destHeight;
	int stringWidth, stringHeight;
	int i, j, bytesPerLine;
	Dimension dW, dH;
	char *data;

	screen = DefaultScreenOfDisplay(dpy);
	black = BlackPixelOfScreen(screen);
	white = WhitePixelOfScreen(screen);
	XmStringExtent(fontlist, string, &dW, &dH);
	stringWidth = (int)dW;
	stringHeight = (int)dH;
	if (!stringWidth || !stringHeight)
		return;

	/* draw string into 1 bit deep pixmap */
	pixmap = XCreatePixmap(dpy, win, stringWidth, stringHeight, 1);
	pixmapGC = XCreateGC(dpy, pixmap, 0, NULL);
	fontStruct = XLoadQueryFont(dpy, "fixed");
	if (!fontStruct)
		{
		fprintf(stderr, "XmLStringDrawDirection: error - ");
		fprintf(stderr, "can't load fixed font\n");
		return;
		}
	XSetFont(dpy, pixmapGC, fontStruct->fid);
	XSetBackground(dpy, pixmapGC, black);
	XSetForeground(dpy, pixmapGC, black);
	XFillRectangle(dpy, pixmap, pixmapGC, 0, 0, stringWidth, stringHeight);
	XSetForeground(dpy, pixmapGC, white);
	XmStringDraw(dpy, pixmap, fontlist, string, pixmapGC, 0, 0, stringWidth,
		XmALIGNMENT_BEGINNING, layout_direction, 0);
	XFreeFont(dpy, fontStruct);

	/* copy 1 bit deep pixmap into source image */
	sourceImage = XGetImage(dpy, pixmap, 0, 0, stringWidth, stringHeight,
		1, XYPixmap);
	XFreePixmap(dpy, pixmap);

	/* draw rotated text into destination image */
	if (drawing_direction == XmSTRING_UP || drawing_direction == XmSTRING_DOWN)
		{
		destWidth = stringHeight;
		destHeight = stringWidth;
		}
	else
		{
		destWidth = stringWidth;
		destHeight = stringHeight;
		}
	bytesPerLine  = (destWidth - 1) / 8 + 1;
	data = (char *)malloc(bytesPerLine * destHeight);
	destImage = XCreateImage(dpy, DefaultVisualOfScreen(screen),
	    1, XYBitmap, 0, data, destWidth, destHeight, 8, 0);
	for (i = 0; i < stringWidth; i++)
		for (j = 0; j < stringHeight; j++)
			{
			if (drawing_direction == XmSTRING_UP)
				XPutPixel(destImage, j, i,
					XGetPixel(sourceImage, stringWidth - i - 1, j));
			else if (drawing_direction == XmSTRING_DOWN)
				XPutPixel(destImage, stringHeight - j - 1, stringWidth - i - 1,
					XGetPixel(sourceImage, stringWidth - i - 1, j));
			else if (drawing_direction == XmSTRING_LEFT)
				XPutPixel(destImage, i, stringHeight - j - 1,
					XGetPixel(sourceImage, stringWidth - i - 1, j));
			else
				XPutPixel(destImage, i, j,
					XGetPixel(sourceImage, i, j));
			}
	XDestroyImage(sourceImage);

	/* copy rotated image into 1 bit deep pixmap */
	pixmap = XCreatePixmap(dpy, win, destWidth, destHeight, 1);
	XPutImage(dpy, pixmap, pixmapGC, destImage, 0, 0, 0, 0,
	    destWidth, destHeight);
	XDestroyImage(destImage);
	XFreeGC(dpy, pixmapGC);

	/* adjust position for alignment */
	if (drawing_direction == XmSTRING_UP || drawing_direction == XmSTRING_DOWN)
		{
		if (alignment == XmALIGNMENT_BEGINNING)
			;
		else if (alignment == XmALIGNMENT_CENTER)
			y += width / 2 - stringWidth / 2;
		else if (alignment == XmALIGNMENT_END)
			y += (int)width - stringWidth;
		}
	else
		{
		if (alignment == XmALIGNMENT_BEGINNING)
			;
		else if (alignment == XmALIGNMENT_CENTER)
			x += width / 2 - stringWidth / 2;
		else if (alignment == XmALIGNMENT_END)
			x += (int)width - stringWidth;
		}

	/* draw the pixmap as a stipple in the window */
	XSetBackground(dpy, gc, black);
	XSetStipple(dpy, gc, pixmap);
	XSetFillStyle(dpy, gc, FillStippled);
	XSetTSOrigin(dpy, gc, x % destWidth, y % destHeight);
	XFillRectangle(dpy, win, gc, x, y, destWidth, destHeight);
	XFreePixmap(dpy, pixmap);
	XSetFillStyle(dpy, gc, FillSolid);
	}

void XmLWarning(w, msg)
Widget w;
char *msg;
	{
	XtAppContext app;
	char s[512], *cname, *name;
	WidgetClass c;

	app = XtWidgetToApplicationContext(w);
	name = XtName(w);
	if (!name)
		name = "[No Name]";
	c = XtClass(w);
	cname = c->core_class.class_name;
	if (!cname)
		cname = "[No Class]";
	sprintf(s, "%s: %s: %s\n", cname, name, msg);
	XtAppWarning(app, s);
	}
