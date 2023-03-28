/*
 * @(#)awt_Scrollbar.c	1.18 95/11/27 Sami Shaio
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
#include "java_awt_Scrollbar.h"
#include "sun_awt_motif_MScrollbarPeer.h"
#include "sun_awt_motif_MComponentPeer.h"

static void
Scrollbar_lineUp(Widget	w,
		 XtPointer	client_data,
		 XtPointer	call_data)
{
    int new_value = ((XmScrollBarCallbackStruct *)call_data)->value;

    JAVA_UPCALL((EE(),
		 (void *)client_data,
		 "lineUp","(I)V", new_value));
}

static void
Scrollbar_lineDown(Widget	w,
		   XtPointer	client_data,
		   XtPointer	call_data)
{
    int new_value = ((XmScrollBarCallbackStruct *)call_data)->value;

    JAVA_UPCALL((EE(),
		 (void *)client_data,
		 "lineDown","(I)V", new_value));
}

static void
Scrollbar_pageUp(Widget	w,
		 XtPointer	client_data,
		 XtPointer	call_data)
{
    int new_value = ((XmScrollBarCallbackStruct *)call_data)->value;

    JAVA_UPCALL((EE(),
		 (void *)client_data,
		 "pageUp","(I)V", new_value));
}

static void
Scrollbar_pageDown(Widget	w,
		   XtPointer	client_data,
		   XtPointer	call_data)
{
    int new_value = ((XmScrollBarCallbackStruct *)call_data)->value;

    JAVA_UPCALL((EE(),
		 (void *)client_data,
		 "pageDown","(I)V", new_value));
}

static void
Scrollbar_dragAbsolute(Widget	w,
		       XtPointer	client_data,
		       XtPointer	call_data)
{
    int new_value = ((XmScrollBarCallbackStruct *)call_data)->value;

    JAVA_UPCALL((EE(), 
		 (void *)client_data,
		 "dragAbsolute","(I)V", new_value));
}


void
sun_awt_motif_MScrollbarPeer_create(struct Hsun_awt_motif_MScrollbarPeer *this,
				    struct Hsun_awt_motif_MComponentPeer *parent)
{
    int				argc;
    Arg				args[40];
    struct ComponentData	*wdata;
    struct ComponentData	*sdata;
    Dimension			d;
    Hjava_awt_Scrollbar		*target;
    Pixel			bg;

    if (parent == 0 || unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    AWT_LOCK();
    wdata = (struct ComponentData *)unhand(parent)->pData;
    target = (struct Hjava_awt_Scrollbar *)unhand(this)->target;
    sdata = ZALLOC(ComponentData);
    SET_PDATA(this, sdata);

    if ((sdata == 0) || (target == 0)) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaGetValues(wdata->widget, XmNbackground, &bg, NULL);
    switch (unhand(target)->orientation) {
      case java_awt_Scrollbar_HORIZONTAL:
	XtVaGetValues(wdata->widget, XmNwidth, &d, NULL);

	argc=0;
	XtSetArg(args[argc], XmNorientation, XmHORIZONTAL); argc++;
	break;
      case java_awt_Scrollbar_VERTICAL:
	XtVaGetValues(wdata->widget, XmNheight, &d, NULL);

	argc=0;
	XtSetArg(args[argc], XmNorientation, XmVERTICAL); argc++;
	break;
      default:
	sysFree(sdata);
	SignalError(0, JAVAPKG "IllegalArgumentException",
		    "bad scrollbar orientation");
	AWT_UNLOCK();
	return;
    }

    XtSetArg(args[argc], XmNrecomputeSize, False); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNx, 0); argc++;
    XtSetArg(args[argc], XmNy, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNmarginTop, 0); argc++;
    XtSetArg(args[argc], XmNmarginBottom, 0); argc++;
    XtSetArg(args[argc], XmNmarginLeft, 0); argc++;
    XtSetArg(args[argc], XmNmarginRight, 0); argc++;
    if ((int)unhand(target)->sVisible > 0) {
	XtSetArg(args[argc], XmNpageIncrement, (int)unhand(target)->sVisible); argc++;
	XtSetArg(args[argc], XmNsliderSize, (int)unhand(target)->sVisible); argc++;
	XtSetArg(args[argc], XmNvalue, (int)unhand(target)->value); argc++;
	XtSetArg(args[argc], XmNminimum, (int)unhand(target)->minimum); argc++;
	XtSetArg(args[argc], XmNmaximum, (int)unhand(target)->maximum + unhand(target)->sVisible); argc++;
    }
    sdata->widget = XmCreateScrollBar(wdata->widget, "scrollbar", args, argc);

    XtSetMappedWhenManaged(sdata->widget, False);
    XtManageChild(sdata->widget);
    XtAddCallback(sdata->widget,
		  XmNdecrementCallback,
		  Scrollbar_lineUp,
		  (XtPointer)this);
    XtAddCallback(sdata->widget,
		  XmNincrementCallback,
		  Scrollbar_lineDown,
		  (XtPointer)this);
    XtAddCallback(sdata->widget,
		  XmNpageDecrementCallback,
		  Scrollbar_pageUp,
		  (XtPointer)this);
    XtAddCallback(sdata->widget,
		  XmNpageIncrementCallback,
		  Scrollbar_pageDown,
		  (XtPointer)this);
    XtAddCallback(sdata->widget,
		  XmNdragCallback,
		  Scrollbar_dragAbsolute,
		  (XtPointer)this);
    AWT_UNLOCK();
}

void
sun_awt_motif_MScrollbarPeer_setValue(struct Hsun_awt_motif_MScrollbarPeer *this, long value)
{
    struct ComponentData *sdata;
    
    AWT_LOCK();
    if ((sdata = PDATA(ComponentData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    /* pass in visible for sliderSize since Motif will calculate the */
    /* slider's size for us. */
    XtVaSetValues(sdata->widget,
		  XmNvalue, value,
		  NULL);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MScrollbarPeer_setValues(struct Hsun_awt_motif_MScrollbarPeer *this,
				       long value,
				       long visible,
				       long minimum,
				       long maximum)
{
    struct ComponentData *sdata;
    
    AWT_LOCK();
    if ((sdata = PDATA(ComponentData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    /* pass in visible for sliderSize since Motif will calculate the */
    /* slider's size for us. */
    XtVaSetValues(sdata->widget,
		  XmNminimum, minimum,
		  XmNmaximum, maximum  + visible,
		  XmNvalue, value,
		  XmNsliderSize, visible,
		  NULL);
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MScrollbarPeer_setLineIncrement(struct Hsun_awt_motif_MScrollbarPeer *this, long value)
{
    struct ComponentData *sdata;
    
    AWT_LOCK();
    if ((sdata = PDATA(ComponentData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(sdata->widget,
		  XmNincrement, value,
		  NULL);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MScrollbarPeer_setPageIncrement(struct Hsun_awt_motif_MScrollbarPeer *this, long value)
{
    struct ComponentData *sdata;
    
    AWT_LOCK();
    if ((sdata = PDATA(ComponentData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(sdata->widget,
		  XmNpageIncrement, value,
		  NULL);
    AWT_FLUSH_UNLOCK();
}

