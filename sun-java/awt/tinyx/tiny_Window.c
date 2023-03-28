/*
 * @(#)tiny_Window.c	1.5 95/12/01 Arthur van Hoff
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

#include "tiny.h"
#include "sun_awt_tiny_TinyWindow.h"
#include "java_awt_Frame.h"
#include <X11/cursorfont.h>

/**
 * Create a window. The window is initially unmapped.
 * The window is a toplevel window when the parent is null.
 */
void 
sun_awt_tiny_TinyWindow_winCreate(TinyWindow *this, TinyWindow *parent)
{
    XSetWindowAttributes atts;
    Window win, pwin;

    AWT_LOCK();
    /* make sure it wasn't already created */
    if (unhand(this)->xid != 0) {
	SignalError(0, JAVAPKG "IllegalArgumentException", "win");
	AWT_UNLOCK();
	return;
    }
    tiny_flush();

    /* get the parent X window id */
    if (parent) {
	pwin = (Window)unhand(parent)->xid;
    } else {
	pwin = awt_root;
    }

    /* initialize window attributes */
    atts.event_mask = KeyPressMask | KeyReleaseMask | FocusChangeMask |
		      ButtonPressMask | ButtonReleaseMask |
		      EnterWindowMask | LeaveWindowMask |
		      PointerMotionMask | ButtonMotionMask |
		      ExposureMask;
    atts.override_redirect = 1;
    atts.border_pixel = 0;
    atts.colormap = awt_cmap;

    /* create the window */
    win = XCreateWindow(awt_display, pwin, 0, 0, 1, 1, 0,
			awt_depth, InputOutput, awt_visual,
			CWOverrideRedirect | CWEventMask | CWBorderPixel | CWColormap, 
			&atts);

    /* make sure it worked */
    if (win == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", "win");
	AWT_UNLOCK();
	return;
    }

    /* set the window variables */
    unhand(this)->xid = (int)win;
    unhand(this)->parent = parent;

    /* register the window for events */
    tiny_register(this);
    AWT_UNLOCK();
}

/**
 * Reshape a window. Note: X windows can't be smaller than 1x1 pixel.
 */
void 
sun_awt_tiny_TinyWindow_winReshape(TinyWindow *this, long x, long y, long w, long h)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
    
	if (w <= 0) {
	    w = 1;
	}
	if (h <= 0) {
	    h = 1;
	}
	XMoveResizeWindow(awt_display, win, x, y, w, h);
	unhand(this)->x = x;
	unhand(this)->y = y;
	unhand(this)->width = w;
	unhand(this)->height = h;
    }
    AWT_UNLOCK();
}

/**
 * Dispose of the window. Further use of it will result in
 * an exception.
 */
void 
sun_awt_tiny_TinyWindow_winDispose(TinyWindow *this)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
	tiny_unregister(this);
	XDestroyWindow(awt_display, win);
	unhand(this)->xid = 0;
    }
    AWT_UNLOCK();
}

/**
 * Show a window.
 */
void 
sun_awt_tiny_TinyWindow_winShow(TinyWindow *this)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
	XMapWindow(awt_display, win);
    }
    AWT_UNLOCK();
}

/**
 * Hide a window.
 */
void 
sun_awt_tiny_TinyWindow_winHide(TinyWindow *this)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
	XUnmapWindow(awt_display, win);
    }
    AWT_UNLOCK();
}

/**
 * Bring a window to the front.
 */
void 
sun_awt_tiny_TinyWindow_winFront(TinyWindow *this)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
	XRaiseWindow(awt_display, win);
    }
    AWT_UNLOCK();
}

/**
 * Send a window to the back.
 */
void 
sun_awt_tiny_TinyWindow_winBack(TinyWindow *this)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
	XLowerWindow(awt_display, win);
    }
    AWT_UNLOCK();
}

/**
 * Set the background color of the window.
 */
void 
sun_awt_tiny_TinyWindow_winBackground(TinyWindow *this, Color *col)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	XSetWindowAttributes atts;

	tiny_flush();
	atts.background_pixel = awt_getColor(col);
	XChangeWindowAttributes(awt_display, win, CWBackPixel, &atts);
    }
    AWT_UNLOCK();
}

extern int tm;
/**
 * Request the input focus
 */
void 
sun_awt_tiny_TinyWindow_winFocus(TinyWindow *this)
{
    Window win;

    AWT_LOCK();
    if ((win = (Window)unhand(this)->xid) != 0) {
	tiny_flush();
	XSetInputFocus(awt_display, win, RevertToPointerRoot, CurrentTime);
    }
    AWT_UNLOCK();
}

void
sun_awt_tiny_TinyWindow_winSetCursor(TinyWindow *this, long cursorType)
{
    unsigned long 		valuemask = 0;
    XSetWindowAttributes	attributes;
    Cursor			cursor;

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
    valuemask = CWCursor;
    attributes.cursor = cursor;
    XChangeWindowAttributes(awt_display,
			    (Window)unhand(this)->xid,
			    valuemask,
			    &attributes);
}
