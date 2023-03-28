/*
 * @(#)TinyCheckboxPeer.java	1.2 95/10/28 Arthur van Hoff
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

public class TinyCheckboxPeer extends TinyComponentPeer implements CheckboxPeer {
    static final int SIZE = 20;
    static final int BORDER = 4;
    static final int SIZ = SIZE - BORDER*2;

    boolean down;

    /**
     * Create the checkbox.
     */
    public TinyCheckboxPeer(Checkbox target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
    }

    /**
     * Minimum size.
     */
    public Dimension minimumSize() {
	String lbl = ((Checkbox)target).getLabel();
	if (lbl != null) {
	    FontMetrics fm = getFontMetrics(target.getFont());
	    return new Dimension(SIZE + fm.stringWidth(lbl) + 5, 
				 Math.max(fm.getHeight() + 8, SIZE));
	}
	return new Dimension(SIZE, SIZE);
    }

    /**
     * Draw a 3D oval.
     */
    void draw3DOval(Graphics g, int x, int y, int w, int h, boolean raised) {
	Color c = g.getColor();
	Color brighter = c.brighter();
	Color darker = c.darker();

	g.setColor(raised ? brighter : darker);
	g.drawArc(x, y, w, h, 45, 180);
	g.setColor(raised ? darker : brighter);
	g.drawArc(x, y, w, h, 225, 180);
    }

    /**
     * Paint the checkbox.
     */
    public void paintComponent(Graphics g) {
	int x = BORDER;
	int y = ((height - SIZE) / 2) + BORDER;

	Checkbox cb = (Checkbox)target;

	g.setColor(down ? Color.gray : cb.getBackground());
	if (cb.getCheckboxGroup() != null) {
	    g.fillOval(x, y, SIZ, SIZ);
	    g.setColor(Color.lightGray);
	    draw3DOval(g, x, y, SIZ, SIZ, !down);
	    g.setColor(cb.getForeground());
	    if (cb.getState()) {
		g.fillOval(x + 3, y + 3, SIZ - 6, SIZ - 6);
	    }
	} else {
	    g.fillRect(x, y, SIZ, SIZ);
	    g.setColor(Color.lightGray);
	    g.draw3DRect(x, y, SIZ, SIZ, !down);
	    g.setColor(cb.getForeground());
	    if (cb.getState()) {
		g.fillRect(x + 3, y + 3, SIZ - 5, SIZ - 5);
	    }
	}

	String lbl = cb.getLabel();
	if (lbl != null) {
	    // REMIND: align
	    g.setFont(cb.getFont());
	    FontMetrics fm = g.getFontMetrics();
	    g.drawString(lbl, SIZE, (height + fm.getAscent()) / 2);
	}
	target.paint(g);
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
		Checkbox cb = (Checkbox)target;
		boolean state = !cb.getState();
		down = false;
		cb.setState(state);
		targetEvent(Event.ACTION_EVENT, new Boolean(state));
	    }
	    return true;
	}
	return false;
    }
   
    public void setLabel(String label) {
	repaint();
    }
    public void setState(boolean state) {
	repaint();
    }
    public void setCheckboxGroup(CheckboxGroup g) {
	repaint();
    }
}
