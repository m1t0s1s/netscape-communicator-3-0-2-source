/*
 * @(#)canvas.h	1.7 95/12/19 Sami Shaio
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
#ifndef _CANVAS_H_
#define _CANVAS_H_

void awt_canvas_pointerMotionEvents(Widget w, long on, XtPointer this);
void awt_canvas_reconfigure(struct FrameData *wdata);
Widget awt_canvas_create(XtPointer this,
			 Widget parent,
			 long width,
			 long height,
			 struct FrameData *wdata);
void awt_canvas_scroll(XtPointer this, struct CanvasData *wdata, long dx, long dy);
void awt_canvas_event_handler(Widget w, XtPointer client_data,
			      XEvent *event, Boolean *cont);
void awt_canvas_handleEvent(Widget w, XtPointer client_data,
			    XEvent *event, Boolean *cont, Boolean passEvent);

#endif           /* _CANVAS_H_ */
