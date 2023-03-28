/*
 * @(#)TinyButtonPeer.java	1.2 95/10/28 Arthur van Hoff
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

class TinyButtonPeer extends TinyComponentPeer implements ButtonPeer {
    boolean down;

    /**
     * Create a button.
     */
    TinyButtonPeer(Button target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
    }

    /**
     * Minimum size.
     */
    public Dimension minimumSize() {
	Button but = (Button)target;
	FontMetrics fm = getFontMetrics(but.getFont());
	return new Dimension(fm.stringWidth(but.getLabel()) + 10, fm.getHeight() + 10);
    }

    /**
     * Set the label.
     */
    public void setLabel(String lbl) {
	repaint();
    }

    /**
     * Paint the button.
     */
    public void paintComponent(Graphics g) {
	Button but = (Button)target;

	g.setColor(down ? target.getBackground().darker() : target.getBackground());
	g.fillRect(1, 1, width-2, height-2);
	g.setColor(target.getBackground());
	g.draw3DRect(0, 0, width-1, height-1, !down);

	g.setColor(but.isEnabled() ? target.getForeground() : target.getForeground().brighter());
	g.setFont(but.getFont());
	FontMetrics fm = g.getFontMetrics();
	String lbl = but.getLabel();
	g.drawString(lbl, (width - fm.stringWidth(lbl)) / 2, 
		          (height + fm.getAscent()) / 2);
    }	

    /**
     * Handle events.
     */
    public boolean handleWindowEvent(Event evt) {
	if (!target.isEnabled()) {
	    return false;
	}

	switch (evt.id) {
	  case Event.MOUSE_DOWN:
	    down = true;
	    repaint();
	    return true;

	  case Event.MOUSE_DRAG:
	    if ((evt.x < 0) || (evt.x > width) || (evt.y < 0) || (evt.y > height)) {
		if (down) {
		    down = false;
		    repaint();
		}
	    } else if (!down) {
		down = true;
		repaint();
	    }
	    return true;

	case Event.MOUSE_UP:
	    if (down) {
		down = false;
		repaint();
		targetEvent(Event.ACTION_EVENT, ((Button)target).getLabel());
	    }
	    return true;
	}
	return false;
    }
}
