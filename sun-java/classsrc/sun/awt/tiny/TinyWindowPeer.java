/*
 * @(#)TinyWindowPeer.java	1.2 95/10/28 Arthur van Hoff
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

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;

class TinyWindowPeer extends TinyPanelPeer implements WindowPeer {
    TinyWindowFrame	frame;

    TinyWindowPeer() {
    }

    TinyWindowPeer(Window target) {
	frame = new TinyWindowFrame(this, null, target.getWarningString(), false);
	init(target);
    }

    void init(Window target) {
	super.init(target);

	if (target.getFont() == null) {
	    target.setFont(new Font("Dialog", Font.PLAIN, 10));
	}
	Color c = target.getBackground();
	if (c == null) {
	    target.setBackground(c = Color.lightGray);
	}
	winBackground(c);
	c = target.getForeground();
	if (c == null) {
	    target.setForeground(c = Color.black);
	}
    }

    public void toFront() {
    	frame.winFront();
    }
    public void toBack() {
    	frame.winBack();
    }
    public Insets insets() {
	return frame.insets();
    }
    public void reshape(int x, int y, int w, int h) {
	frame.winReshape(x, y, w, h);
    }
    public void show() {
	frame.winShow();
    }
    public void hide() {
	frame.winHide();
    }
}

