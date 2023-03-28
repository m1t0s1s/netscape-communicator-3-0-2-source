/*
 * @(#)MWindowPeer.java	1.7 95/11/09 Sami Shaio
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

class MWindowPeer extends MPanelPeer implements WindowPeer {
    Insets insets = new Insets(0, 0, 0, 0);

    static Vector allWindows = new Vector();

    native void create(MComponentPeer parent);
    native void pShow();
    native void pHide();
    native void pReshape(int x, int y, int width, int height);
    native void pDispose();

    MWindowPeer(Window target) {
	super(target);
	allWindows.addElement(this);
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
    }

    public void dispose() {
	allWindows.removeElement(this);
	super.dispose();
    }

    public void toFront() {
	pShow();
    }
    public native void toBack();

    public Insets insets() {
	return insets;
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

