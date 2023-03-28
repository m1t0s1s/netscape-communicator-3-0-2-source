/*
 * @(#)MFramePeer.java	1.40 95/12/01 Sami Shaio
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
package sun.awt.motif;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;
import java.awt.image.ImageObserver;
import sun.awt.image.ImageRepresentation;

class MFramePeer extends MPanelPeer implements FramePeer {
    Insets	insets;

    static Vector allFrames = new Vector();

    native void create(MComponentPeer parent, Object arg);
    native void pSetTitle(String title);
    native void pShow();
    native void pHide();
    native void pReshape(int x, int y, int width, int height);
    native void pDispose();
    native void pSetIconImage(ImageRepresentation ir);
    public native void setResizable(boolean resizable);

    MFramePeer(Frame target) {
	super(target,new Insets(Integer.getInteger("awt.frame.topInset", 25).intValue(),
				Integer.getInteger("awt.frame.leftInset", 5).intValue(),
				Integer.getInteger("awt.frame.bottomInset",5).intValue(),
				Integer.getInteger("awt.frame.rightInset", 5).intValue()));
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
	ImageRepresentation ir = ((X11Image)im).getImageRep(-1, -1);
	ir.reconstruct(ImageObserver.ALLBITS);
	pSetIconImage(ir);
    }

    public void setMenuBar(MenuBar mb) {
	pSetMenuBar(mb);
	if (target.isVisible()) {
	    Rectangle r = target.bounds();

	    pReshape(r.x, r.y, r.width, r.height);
	    target.invalidate();
	    target.validate();
	}
    }
    native void pSetMenuBar(MenuBar mb);

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
	pShow();
    }
    public native void toBack();

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

