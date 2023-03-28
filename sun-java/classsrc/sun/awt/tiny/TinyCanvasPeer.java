/*
 * @(#)TinyCanvasPeer.java	1.2 95/10/28 Arthur van Hoff
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

class TinyCanvasPeer extends TinyComponentPeer implements CanvasPeer {
    TinyCanvasPeer() {
    }

    TinyCanvasPeer(Canvas target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
    }

    void paintComponent(Graphics g) {
	g.setColor(target.getBackground());
	g.fillRect(0, 0, width, height);
    }
    public void update(Graphics g) {
	target.update(g);
    }

    public boolean handleWindowEvent(Event evt) {
	if (evt.id == Event.MOUSE_DOWN) {
	    requestFocus();
	}

	evt.target = target;
	targetEvent(evt);
	return true;
    }
}




