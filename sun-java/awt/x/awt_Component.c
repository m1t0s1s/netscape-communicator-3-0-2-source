/*
 * @(#)awt_Component.c	1.42 95/12/19 Sami Shaio
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
#include "canvas.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_Event.h"
#include "sun_awt_motif_MComponentPeer.h"

/*
 * Looking at the state variables in target, initialize the component properly.
 */
void
sun_awt_motif_MComponentPeer_pInitialize(struct Hsun_awt_motif_MComponentPeer *this)
{
    Classjava_awt_Component		*targetPtr;
    Classsun_awt_motif_MComponentPeer	*thisPtr = unhand(this);
    struct ComponentData	 	*cdata;

    AWT_LOCK();
    if (thisPtr->target == 0 || (cdata = PDATA(ComponentData, this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    targetPtr = unhand(thisPtr->target);

    XtVaSetValues(cdata->widget,
		  XmNx, targetPtr->x,
		  XmNy, targetPtr->y,
		  XmNvisual, awt_visual,
		  NULL);

    if (!XtIsSubclass(cdata->widget,xmDrawingAreaWidgetClass)) {
	XtAddEventHandler(cdata->widget, ExposureMask,
			  True, awt_canvas_event_handler, this);
    }
#ifdef NEW_EVENT_MODEL
    awt_addWidget(cdata->widget, this);
#endif
    AWT_UNLOCK();
}


struct Hjava_awt_Event *
sun_awt_motif_MComponentPeer_setData(struct Hsun_awt_motif_MComponentPeer *this,
				     long data,
				     struct Hjava_awt_Event *event)
{
    Classjava_awt_Event	*eventPtr = unhand(event);

    eventPtr->data = data;

    return event;
}


void
sun_awt_motif_MComponentPeer_disposeEvent(struct Hsun_awt_motif_MComponentPeer *this,
					 struct Hjava_awt_Event *event)
{
    Classjava_awt_Event	*eventPtr;

    if (event == NULL) {
	return;
    }

    eventPtr = unhand(event);
    AWT_LOCK();
    if (eventPtr->data != 0) {
	sysFree((void *)eventPtr->data);
	eventPtr->data = 0;
    }
    AWT_UNLOCK();
}


long 
sun_awt_motif_MComponentPeer_handleEvent(struct Hsun_awt_motif_MComponentPeer *this,
					 struct Hjava_awt_Event *event)
{
    Classjava_awt_Event	*eventPtr;
    extern void awt_modify_Event(XEvent *xev, struct Hjava_awt_Event *jev);

    if (event == NULL) {
	return 0;
    }

#ifdef NEW_EVENT_MODEL
    AWT_LOCK();
    eventPtr = unhand(event);
    if (eventPtr->data != 0) {
	awt_modify_Event((XEvent *)eventPtr->data, event);
	XtDispatchEvent((XEvent *)eventPtr->data);
	sysFree((void *)eventPtr->data);
        eventPtr->data = 0;
	AWT_UNLOCK();
	return 1;
    } else {
	AWT_UNLOCK();
	return 0;
    }
#else
    return 0;
#endif				/* NEW_EVENT_MODEL */
}

void 
sun_awt_motif_MComponentPeer_pEnable(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    awt_util_enable(cdata->widget);
    AWT_UNLOCK();
}


void 
sun_awt_motif_MComponentPeer_pDisable(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    awt_util_disable(cdata->widget);
    AWT_FLUSH_UNLOCK();
}

void 
sun_awt_motif_MComponentPeer_pShow(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    awt_util_show(cdata->widget);
    AWT_FLUSH_UNLOCK();
}

void 
sun_awt_motif_MComponentPeer_pHide(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    awt_util_hide(cdata->widget);
    AWT_FLUSH_UNLOCK();
}


void 
sun_awt_motif_MComponentPeer_pReshape(struct Hsun_awt_motif_MComponentPeer *this,
				      long x,
				      long y,
				      long w,
				      long h)
{
    struct ComponentData	*cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0 || unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    awt_util_reshape(cdata->widget, x, y, w, h);

    AWT_FLUSH_UNLOCK();
}


void 
sun_awt_motif_MComponentPeer_pTriggerRepaint(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData	*cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	AWT_UNLOCK();
	return;
    }
    if (XtWindow(cdata->widget) == 0) {
	AWT_UNLOCK();
	return;
    }
    if (cdata->repaintPending) {
	/* generate a GraphicsExpose event */
	XEvent evt;
	evt.xgraphicsexpose.type = GraphicsExpose;
	evt.xgraphicsexpose.send_event = False;
	evt.xgraphicsexpose.display = awt_display;
	evt.xgraphicsexpose.drawable = XtWindow(cdata->widget);
	evt.xgraphicsexpose.x = cdata->x1;
	evt.xgraphicsexpose.y = cdata->y1;
	evt.xgraphicsexpose.width = cdata->x2 - cdata->x1;
	evt.xgraphicsexpose.height = cdata->y2 - cdata->y1;
	evt.xgraphicsexpose.count = 0;
	XSendEvent(awt_display, XtWindow(cdata->widget), False, ExposureMask, &evt);
	XFlush(awt_display);
    }
    AWT_UNLOCK();
}

void 
sun_awt_motif_MComponentPeer_pAddRepaint(struct Hsun_awt_motif_MComponentPeer *this,
				      long x,
				      long y,
				      long w,
				      long h)
{
    struct ComponentData	*cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (XtWindow(cdata->widget) == 0) {
	AWT_UNLOCK();
	return;
    }

    /*
    printf("repaint %d,%d,%dx%d\n", x, y, w, h);
    */
    if (!cdata->repaintPending) {
	/* remember the repaint area */
	cdata->repaintPending = 1;
	cdata->x1 = x;
	cdata->y1 = y;
	cdata->x2 = x + w;
	cdata->y2 = y + h;
    } else {
	/* expand the repaint area */
	if (x < cdata->x1) {
	    cdata->x1 = x;
	}
	if (y < cdata->y1) {
	    cdata->y1 = y;
	}
	if (x + w > cdata->x2) {
	    cdata->x2 = x + w;
	}
	if (y + h > cdata->y2) {
	    cdata->y2 = y + h;
	}
    }

    AWT_UNLOCK();
}

void 
sun_awt_motif_MComponentPeer_pDispose(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *cdata;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(cdata->widget);
    XtDestroyWidget(cdata->widget);
#ifdef NEW_EVENT_MODEL
    awt_delWidget(cdata->widget);
#endif
    sysFree((void *)cdata);
    SET_PDATA(this, 0);
    AWT_UNLOCK();
}

static void
changeFont(Widget w, void *fontList)
{
    XtVaSetValues(w, XmNfontList, fontList, NULL);
}


void 
sun_awt_motif_MComponentPeer_setFont(struct Hsun_awt_motif_MComponentPeer *this,
				     struct Hjava_awt_Font *f)
{
    struct ComponentData *cdata;
    struct FontData *fdata;
    XmFontList	fontlist;
    char	*err;

    if (f == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(f, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return;
    }
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    fontlist = fdata->xmfontlist;
    if (fontlist != NULL) {
	awt_util_mapChildren(cdata->widget, changeFont, 1, (void *)fontlist);
    } else {
	SignalError(0, JAVAPKG "NullPointerException", 0);
    }
    AWT_FLUSH_UNLOCK();
}

static void
changeForeground(Widget w, void *fg)
{
    XtVaSetValues(w, XmNforeground, fg, NULL);
}

void 
sun_awt_motif_MComponentPeer_setForeground(struct Hsun_awt_motif_MComponentPeer *this,
				     struct Hjava_awt_Color *c)
{
    struct ComponentData *bdata;
    Pixel		color;

    if (c==0) {
	SignalError(0,JAVAPKG "NullPointerException","null color");
	return;
    }
    AWT_LOCK();
    if ((bdata = PDATA(ComponentData,this)) == 0 || bdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    color = awt_getColor(c);
    awt_util_mapChildren(bdata->widget, changeForeground, 1, (void *)color);
    AWT_FLUSH_UNLOCK();
}

static void
changeBackground(Widget w, void *bg)
{
    Pixel		fg;

    XtVaGetValues(w, XmNforeground, &fg, NULL);
    XmChangeColor(w, (Pixel)bg);
    XtVaSetValues(w, XmNforeground, fg, NULL);
}

void 
sun_awt_motif_MComponentPeer_setBackground(struct Hsun_awt_motif_MComponentPeer *this,
				     struct Hjava_awt_Color *c)
{
    struct ComponentData *bdata;
    Pixel		color;

    if (c==0) {
	SignalError(0,JAVAPKG "NullPointerException","null color");
	return;
    }
    AWT_LOCK();
    if ((bdata = PDATA(ComponentData,this)) == 0 || bdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    color = awt_getColor(c);
    awt_util_mapChildren(bdata->widget, changeBackground, 1, (void *)color);
    AWT_FLUSH_UNLOCK();
}

void 
sun_awt_motif_MComponentPeer_requestFocus(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *bdata;

    AWT_LOCK();
    if ((bdata = PDATA(ComponentData,this)) == 0 || bdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XmProcessTraversal(bdata->widget, XmTRAVERSE_CURRENT);
    AWT_FLUSH_UNLOCK();
}

void 
sun_awt_motif_MComponentPeer_nextFocus(struct Hsun_awt_motif_MComponentPeer *this)
{
    struct ComponentData *bdata;

    AWT_LOCK();
    if ((bdata = PDATA(ComponentData,this)) == 0 || bdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XmProcessTraversal(bdata->widget, XmTRAVERSE_NEXT);
    AWT_FLUSH_UNLOCK();
}


