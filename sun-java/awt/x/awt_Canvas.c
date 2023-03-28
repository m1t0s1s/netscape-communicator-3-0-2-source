/*
 * @(#)awt_Canvas.c	1.14 95/12/08 Sami Shaio
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
#include "java_awt_Canvas.h"
#include "sun_awt_motif_MCanvasPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "color.h"
#include "canvas.h"

void
sun_awt_motif_MCanvasPeer_create(struct Hsun_awt_motif_MCanvasPeer *this,
				 struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct CanvasData	   *wdata;
    struct CanvasData	   *cdata;

    AWT_LOCK();
    if (parent == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    cdata = PDATA(CanvasData,parent);
    if (cdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    wdata = ZALLOC(CanvasData);
    if (wdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    SET_PDATA(this, wdata);
    wdata->comp.widget = awt_canvas_create((XtPointer)this,
					   cdata->comp.widget,
					   1, 1, NULL);
    wdata->flags = 0;
    wdata->shell = cdata->shell;
    AWT_UNLOCK();
}

#if 0
void
sun_awt_motif_MCanvasPeer_scroll(struct Hsun_awt_motif_MCanvasPeer *this,
				 long dx, long dy)
{
    struct CanvasData	    *wdata;

    AWT_LOCK();
    if ((wdata = PDATA(CanvasData,this)) == 0 || wdata->comp.widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    awt_canvas_scroll((XtPointer)this, wdata, dx, dy);
    AWT_UNLOCK();
}

#endif
