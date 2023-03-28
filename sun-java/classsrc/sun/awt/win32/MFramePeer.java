/*
 * @(#)MFramePeer.java	1.19 95/12/02 Sami Shaio
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
package sun.awt.win32;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;
import java.awt.image.ImageObserver;
import sun.awt.image.ImageRepresentation;

class MFramePeer extends MPanelPeer implements FramePeer {
    Insets	insets = new Insets(0, 0, 0, 0);

    static Vector allFrames = new Vector();

    native void create(MComponentPeer parent);
    native void pSetTitle(String title);
    native void pShow();
    native void pHide();
    native void pReshape(int x, int y, int width, int height);
    native void pDispose();
    native void pSetIconImage(ImageRepresentation ir);
    // XXX: need separate call to set insets because the insets data member
    // isn't set until after the super constructor is called. This was solved
    // differently on the Motif side but this solution seems cleaner.
    native void setInsets(Insets i);
    public native void setResizable(boolean resizable);

    MFramePeer(Frame target) {
	super(target);
	// see comment above for setInsets
	setInsets(insets);
	allFrames.addElement(this);
	if (target.getTitle() != null) {
	    pSetTitle(target.getTitle());
	}
	Font f = target.getFont();
	if (f == null) {
	    f = new Font("Dialog", Font.PLAIN, 12);
	    target.setFont(f);
	    setFont(f);
	}
	Color c = target.getBackground();
	if (c == null) {
	    target.setBackground(Color.lightGray);
	    setBackground(Color.lightGray);
	}
	c = target.getForeground();
	if (c == null) {
	    target.setForeground(Color.black);
	    setForeground(Color.black);
	}
	Image icon = target.getIconImage();
	if (icon != null) {
	    setIconImage(icon);
	}
	if (target.getCursorType() != Frame.DEFAULT_CURSOR) {
	    setCursor(target.getCursorType());
	}
	setResizable(target.isResizable());
    }

    public void setTitle(String title) {
	pSetTitle(title);
    }

    public void dispose() {
	allFrames.removeElement(this);
	super.dispose();
    }

    public void setIconImage(Image im) {
	ImageRepresentation ir = ((Win32Image)im).getImageRep(-1, -1);
	ir.reconstruct(ImageObserver.ALLBITS);
	pSetIconImage(ir);
    }

    public native void setMenuBar(MenuBar mb);

    public void handleQuit() {
	target.postEvent(new Event(target, Event.WINDOW_DESTROY, null));
    }

    public void handleIconify() {
	target.postEvent(new Event(target, Event.WINDOW_ICONIFY, null));
    }

    public void handleDeiconify() {
	target.postEvent(new Event(target, Event.WINDOW_DEICONIFY, null));
    }

    public void toFront() {
	show();
    }
    public void toBack() {
    }

    /**
     * Called to inform the Frame that it has moved.
     */
    public synchronized void handleMoved(int x, int y) {
	target.postEvent(new Event(target, 0, Event.WINDOW_MOVED, x, y, 0, 0));
    }

    /**
     * Called to inform the Frame that its size has changed and it
     * should layout its children.
     */
    public synchronized void handleResize(int width, int height) {
	//target.resize(width, height);
	target.invalidate();
	target.validate();
	target.repaint();
    }

    public Insets insets() {
	return insets;
    }

    public native void setCursor(int cursorType);
}
