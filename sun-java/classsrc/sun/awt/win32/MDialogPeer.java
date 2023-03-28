/*
 * @(#)MDialogPeer.java	1.8 95/11/29 Sami Shaio
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

class MDialogPeer extends MPanelPeer implements DialogPeer {
    Insets insets = new Insets(0, 0, 0, 0);

    static Vector allDialogs = new Vector();

    native void create(MComponentPeer parent);
    native void pSetTitle(String title);
    native void pShow();
    native void pHide();
    native void pReshape(int x, int y, int width, int height);
    native void pDispose();
    // XXX: need separate call to set insets because the insets data member
    // isn't set until after the super constructor is called. This was solved
    // differently on the Motif side but this solution seems cleaner.
    native void setInsets(Insets i);
    public native void setResizable(boolean resizable);

    MDialogPeer(Dialog target) {
	super(target);
	// see comment above for setInsets
	setInsets(insets);
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
	show();
    }
    public void toBack() {
    }

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

