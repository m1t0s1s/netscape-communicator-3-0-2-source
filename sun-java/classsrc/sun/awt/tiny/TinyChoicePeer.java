/*
 * @(#)TinyChoicePeer.java	1.2 95/10/28 Arthur van Hoff
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

class TinyChoiceMenu extends TinyWindow {
    TinyChoicePeer peer;
    String items[];
    int selected;
    int h;

    /**
     * Create a choice popup menu
     */
    TinyChoiceMenu(TinyChoicePeer peer, int x, int y, int w, int h, int selected, String items[]) {
	this.h = h;
	this.peer = peer;
	this.items = items;
	this.selected = selected;
	winCreate(null);
	winBackground(peer.target.getBackground());
	winReshape(x - 2, (y - 2) - selected * h, w + 4, items.length * h + 4);
    }

    /**
     * Paint the popup menu
     */
    public void paint(Graphics g) {
	g.setColor(peer.target.getBackground());
	g.draw3DRect(0, 0, width - 1, height - 1, true);
	g.draw3DRect(1, 1, width - 3, height - 3, true);
	g.draw3DRect(2, 2 + selected * h, width - 5, h - 1, false);

	g.setFont(peer.target.getFont());
	FontMetrics fm = g.getFontMetrics();
	g.setColor(peer.target.getForeground());
	int y = 2 + (h + fm.getAscent()) / 2;
	for (int i = 0 ; i < items.length ; i++, y += h) {
	    g.drawString(items[i], 7, y);
	}
    }

    /**
     * Handle events
     */
    boolean handleWindowEvent(Event evt) {
	int i = selected;
	if ((evt.x < 0) || (evt.x > width) || (evt.y < 0) || (evt.y > height)) {
	    i = ((Choice)peer.target).getSelectedIndex();
	} else {
	    i = Math.max(0, Math.min(items.length - 1, (evt.y - 2) / h));
	}
	if (selected != i) {
	    Graphics g = new TinyGraphics(this);
	    try {
		g.setColor(peer.target.getBackground());
		g.drawRect(2, 2 + selected * h, width - 5, h - 1);
		selected = i;
		g.draw3DRect(2, 2 + selected * h, width - 5, h - 1, false);
	    } finally {
		g.dispose();
	    }
	}
	return true;
    }
}

class TinyChoicePeer extends TinyComponentPeer implements ChoicePeer {
    TinyChoiceMenu menu;

    /**
     * Create the choice
     */
    public TinyChoicePeer(Choice target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
    }

    /**
     * Minimum size
     */
    public Dimension minimumSize() {
	FontMetrics fm = getFontMetrics(target.getFont());
	Choice c = (Choice)target;
	int w = 0;
	for (int i = c.countItems() ; i-- > 0 ;) {
	    w = Math.max(fm.stringWidth(c.getItem(i)), w);
	}
	return new Dimension(w + 10 + 18, fm.getHeight() + 10);
    }

    /**
     * Paint the choice
     */
    public void paintComponent(Graphics g) {
	Choice ch = (Choice)target;

	g.setColor(Color.lightGray);
	g.fillRect(1, 1, width-2, height-2);
	g.draw3DRect(0, 0, width-1, height-1, true);
	g.draw3DRect(width - 18, (height / 2) - 3, 12, 6, true);

	g.setColor(Color.black);
	g.setFont(ch.getFont());
	FontMetrics fm = g.getFontMetrics();
	String lbl = ch.getSelectedItem();
	g.drawString(lbl, 5, (height + fm.getAscent()) / 2);
	target.paint(g);
    }	

    /**
     * Handle events
     */
    public boolean handleWindowEvent(Event evt) {
	if (!target.isEnabled()) {
	    return false;
	}

	switch (evt.id) {
	  case Event.MOUSE_DOWN: {
	    Choice ch = (Choice)target;
	    String items[] = new String[ch.countItems()];
	    for (int i = 0 ; i < items.length ; i++) {
		items[i] = ch.getItem(i);
	    }
	    Point pt = toGlobal(0, 0);
	    menu = new TinyChoiceMenu(this, pt.x, pt.y, width - 20, height, 
				      ch.getSelectedIndex(), items);
	    menu.winShow();
	    return true;
	  }

	  case Event.MOUSE_DRAG:
	    Point pt = menu.toLocal(toGlobal(evt.x, evt.y));
	    evt.x = pt.x;
	    evt.y = pt.y;
	    return menu.handleWindowEvent(evt);

	  case Event.MOUSE_UP: {
	    Choice ch = (Choice)target;
	    ch.select(menu.selected);
	    menu.winHide();
	    menu.winDispose();
	    menu = null;
	    targetEvent(Event.ACTION_EVENT, ch.getSelectedItem());
	    return true;
	  }
	}
	return false;
    }

    public void select(int index) {
	repaint();
    }
    public void addItem(String item, int index) {
	repaint();
    }
}

