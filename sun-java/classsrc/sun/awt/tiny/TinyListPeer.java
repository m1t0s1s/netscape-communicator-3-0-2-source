/*
 * @(#)TinyListPeer.java	1.6 95/11/22 Arthur van Hoff
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

class TinyListPeer extends TinyComponentPeer implements ListPeer, TinyScrollbarClient {
    TinyVerticalScrollbar sb;

    /**
     * Create a list
     */
    TinyListPeer(List target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
	FontMetrics fm = getFontMetrics(target.getFont());

	sb = new TinyVerticalScrollbar();
	sb.sb = this;
	sb.vis = (height - 2);
	sb.max = target.countItems() - 1;
	sb.page = sb.vis;
	sb.line = 1;
    }

    /**
     * Minimum size.
     */
    public Dimension minimumSize() {
	return minimumSize(4);
    }
    public Dimension preferredSize(int v) {
	return minimumSize(v);
    }
    public Dimension minimumSize(int v) {
	FontMetrics fm = getFontMetrics(target.getFont());
	return new Dimension(20 + fm.stringWidth("0123456789abcde"), 
			     (fm.getHeight() + 2) * v + 11);
    }

    /**
     * Paint the scrollbar.
     */
    void paintScrollbar(Graphics g) {
	Graphics ng = g.create();
	try {
	    ng.translate(width-15, 0);
	    sb.paint(ng, target.getBackground(), 15, height);
	} finally {
	    ng.dispose();
	}
    }

    /**
     * Paint the list
     */
    public void paintComponent(Graphics g) {
	List list = (List)target;
	g.setFont(list.getFont());
	FontMetrics fm = g.getFontMetrics();

	g.setColor(target.getBackground());
	g.draw3DRect(0, 0, width-15, height-1, false);
	g.fillRect(1, 1, width-16, height-2);
	g.fillRect(width-14, 1, 13, height-2);
	paintScrollbar(g);

	int y = 5 + fm.getAscent();
	int len = list.countItems();
	g.setColor(target.getForeground());
	g.clipRect(1, 1, width-15, height-2);
	for (int i = sb.val ; i < len ; i++) {
	    if (y + fm.getDescent() > height - 5) {
		break;
	    }
	    g.drawString(list.getItem(i), 5, y);
	    y += fm.getHeight() + 2;
	}
    }

    /**
     * Handle Events
     */
    public boolean handleWindowEvent(Event evt) {
	switch (evt.id) {
	case Event.MOUSE_DOWN:
	case Event.MOUSE_DRAG:
	case Event.MOUSE_MOVE:
	    if (evt.x >= width - 15) {
		evt.x -= width - 15;
		return sb.handleWindowEvent(evt, 15, height);
	    }
	    break;
	}
	return false;
    }

    public void notifyValue(Object obj, int id, int v) {
	sb.val = v;
	repaint();
    }
    public void setMultipleSelections(boolean v) {
    }
    public boolean isSelected(int index) {
	return false;
    }
    public void addItem(String item, int index) {
	repaint();
    }
    public void delItems(int start, int end) {
	repaint();
    }    
    public void select(int index) {
    }
    public void deselect(int index) {
    }
    public void makeVisible(int index) {
    }
    public void clear() {
    }
    public int[] getSelectedIndexes() {
	return new int[0];
    }
}
