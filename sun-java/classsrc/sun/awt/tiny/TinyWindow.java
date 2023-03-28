/*
 * @(#)TinyWindow.java	1.4 95/12/01 Arthur van Hoff
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
package sun.awt.tiny;

import java.awt.*;

/**
 * A simple window class. This class provides access to a simple
 * clipped X window that can recieve events. You must call create
 * to activate the window, it is initially unmapped.
 *
 * @author Arthur van Hoff
 */
class TinyWindow {
    private int xid;

    TinyWindow parent;
    int x;
    int y;
    int width;
    int height;

    native void winCreate(TinyWindow parent);
    native void winReshape(int x, int y, int width, int height);
    native void winDispose();
    native void winShow();
    native void winHide();
    native void winFront();
    native void winBack();
    native void winBackground(Color c);
    native void winFocus();
    native void winSetCursor(int cursorType);

    /**
     * Convert to global coordinates.
     */
    Point toGlobal(Point pt) {
	for (TinyWindow win = this ; win != null ; win = win.parent) {
	    pt.translate(win.x, win.y);
	}
	return pt;
    }
    Point toGlobal(int x, int y) {
	return toGlobal(new Point(x, y));
    }

    /**
     * Convert to local coordinates.
     */
    Point toLocal(Point pt) {
	for (TinyWindow win = this ; win != null ; win = win.parent) {
	    pt.translate(-win.x, -win.y);
	}
	return pt;
    }
    Point toLocal(int x, int y) {
	return toLocal(new Point(x, y));
    }

    /**
     * Paint the window.
     */
    void paint(Graphics g) {
	g.setColor(Color.black);
	g.drawRect(0, 0, width-1, height-1);
	g.setColor(Color.lightGray);
	g.fillRect(1, 1, width-2, height-2);
    }

    /**
     * Expose call back.
     */
    void handleExpose(int x, int y, int w, int h) {
	Graphics g = new TinyGraphics(this);
	try {
	    g.clipRect(x, y, w, h);
	    paint(g);
	} finally {
	    g.dispose();
	}
    }

    /**
     * Post Events
     */
    void postEvent(long when, int id, int x, int y, int key, int flags, Object arg) {
	handleWindowEvent(new Event(this, when, id, x, y, key, flags, arg));
    }

    /**
     * Event handler
     */
    boolean handleWindowEvent(Event evt) {
	return false;
    }

    /*
     * Low level call backs
     */
    void handleKeyPress(long when, int x, int y, int key, int flags) {
	x += this.x;
	y += this.y;
	if (parent != null) {
	    parent.handleKeyPress(when, x, y, key, flags);
	} else {
	    postEvent(when, Event.KEY_PRESS, x, y, key, flags, null);
	}
    }
    void handleKeyRelease(long when, int x, int y, int key, int flags) {
	x += this.x;
	y += this.y;
	if (parent != null) {
	    parent.handleKeyRelease(when, x, y, key, flags);
	} else {
	    postEvent(when, Event.KEY_RELEASE, x, y, key, flags, null);
	}
    }
    void handleActionKey(long when, int x, int y, int key, int flags) {
	x += this.x;
	y += this.y;
	if (parent != null) {
	    parent.handleActionKey(when, x, y, key, flags);
	} else {
	    postEvent(when, Event.KEY_ACTION, x, y, key, flags, null);
	}
    }
    void handleActionKeyRelease(long when, int x, int y, int key, int flags) {
	x += this.x;
	y += this.y;
	if (parent != null) {
	    parent.handleActionKeyRelease(when, x, y, key, flags);
	} else {
	    postEvent(when, Event.KEY_ACTION_RELEASE, x, y, key, flags, null);
	}
    }
    void handleMouseEnter(long when, int x, int y) {
	postEvent(when, Event.MOUSE_ENTER, x, y, 0, 0, null);
    }
    void handleMouseExit(long when, int x, int y) {
	postEvent(when, Event.MOUSE_EXIT, x, y, 0, 0, null);
    }
    void handleMouseDown(long when, int x, int y, int flags) {
	postEvent(when, Event.MOUSE_DOWN, x, y, 0, flags, null);
    }
    void handleMouseUp(long when, int x, int y, int flags) {
	postEvent(when, Event.MOUSE_UP, x, y, 0, flags, null);
    }
    void handleMouseMove(long when, int x, int y, int flags) {
	postEvent(when, Event.MOUSE_MOVE, x, y, 0, flags, null);
    }
    void handleMouseDrag(long when, int x, int y, int flags) {
	postEvent(when, Event.MOUSE_DRAG, x, y, 0, flags, null);
    }
    void handleFocusIn() {
    }
    void handleFocusOut() {
    }

    /**
     * Request the character focus.
     */
    void requestFocus() {
	requestFocus(this);
    }
    void requestFocus(TinyWindow focus) {
	if (parent != null) {
	    parent.requestFocus(focus);
	}
    }

    boolean hasFocus() {
	return hasFocus(this);
    }
    boolean hasFocus(TinyWindow win) {
	return (parent != null) && parent.hasFocus(win);
    }

    protected void finalize() {
	winDispose();
    }
}



