/*
 * @(#)TinyPanelPeer.java	1.3 95/12/02 Arthur van Hoff
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
import java.awt.peer.*;

class TinyPanelPeer extends TinyCanvasPeer implements PanelPeer {
    TinyPanelPeer() {
    }

    TinyPanelPeer(Component target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
    }

    public Insets insets() {
	return new Insets(0, 0, 0, 0);
    }

    public void print(Graphics g) {
	super.print(g);
	((Container)target).printComponents(g);
    }

    public boolean handleWindowEvent(Event evt) {
	evt.target = target;
	targetEvent(evt);
	return true;
    }

    public void scroll(int dx, int dy) {
	// REMIND: noop for now.
    }
}

