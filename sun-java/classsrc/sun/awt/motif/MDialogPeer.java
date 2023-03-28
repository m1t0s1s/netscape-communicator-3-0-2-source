/*
 * @(#)MDialogPeer.java	1.10 95/11/02 Sami Shaio
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

class MDialogPeer extends MPanelPeer implements DialogPeer {
    Insets	insets;

    static Vector allDialogs = new Vector();

    native void create(MComponentPeer parent, Object arg);
    native void pSetTitle(String title);
    native void pShow();
    native void pHide();
    native void pReshape(int x, int y, int width, int height);
    native void pDispose();
    public native void setResizable(boolean resizable);

    MDialogPeer(Dialog target) {
	super(target,new Insets(Integer.getInteger("awt.frame.topInset", 25).intValue(),
				Integer.getInteger("awt.frame.leftInset", 5).intValue(),
				Integer.getInteger("awt.frame.bottomInset",5).intValue(),
				Integer.getInteger("awt.frame.rightInset", 5).intValue()));

	allDialogs.addElement(this);
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
	setResizable(target.isResizable());
    }

    public void setTitle(String title) {
	pSetTitle(title);
    }

    public void dispose() {
	allDialogs.removeElement(this);
	super.dispose();
    }

    /**
     * Called to inform the Dialog that it has moved.
     */
    public synchronized void handleMoved(int x, int y) {
	target.postEvent(new Event(target, 0, Event.WINDOW_MOVED, x, y, 0, 0));
    }


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

    public Insets insets() {
	return insets;
    }

    /**
     * Called to inform the Dialog that its size has changed and it
     * should layout its children.
     */
    public synchronized void handleResize(int width, int height) {
	//target.resize(width, height);
	target.invalidate();
	target.validate();
	target.repaint();
    }
}

