/*
 * @(#)MTinyChoicePeer.java	1.3 95/11/29 Sami Shaio
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

import java.awt.*;
import java.awt.peer.*;

import java.awt.*;
import java.awt.peer.*;

class MChoiceMenu extends Window {
    MTinyChoicePeer peer;
    String items[];
    int selected;
    int h;

    /**
     * Create a choice popup menu
     */
    MChoiceMenu(Frame f, MTinyChoicePeer peer, int x, int y, int w, int h, int selected, String items[]) {
	super(f);
	this.h = h;
	this.peer = peer;
	this.items = items;
	this.selected = selected;
	setBackground(f.getBackground());
	reshape(x - 2, (y - 2) - selected * h, w + 4, items.length * h + 4);
    }

    /**
     * Paint the popup menu
     */
    public void paint(Graphics g) {
	Dimension size = size();

	g.setColor(getBackground());
	g.draw3DRect(0, 0, size.width - 1, size.height - 1, true);
	g.draw3DRect(1, 1, size.width - 3, size.height - 3, true);
	g.draw3DRect(2, 2 + selected * h, size.width - 5, size.height - 1, false);

	g.setFont(getFont());
	FontMetrics fm = g.getFontMetrics();
	g.setColor(getForeground());
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
	Dimension size = size();
	if ((evt.x < 0) || (evt.x > size.width) || (evt.y < 0) || (evt.y > size.height)) {
	    i = ((Choice)peer.target).getSelectedIndex();
	} else {
	    i = Math.max(0, Math.min(items.length - 1, (evt.y - 2) / h));
	}
	if (selected != i) {
	    Graphics g = getGraphics();
	    try {
		g.setColor(getBackground());
		g.drawRect(2, 2 + selected * h, size.width - 5, h - 1);
		selected = i;
		g.draw3DRect(2, 2 + selected * h, size.width - 5, h - 1, true);
	    } finally {
		g.dispose();
	    }
	}
	return false;
    }
}

class MTinyChoicePeer extends MCanvasPeer implements ChoicePeer {
    MChoiceMenu menu;

    /**
     * Create the choice
     */
    public MTinyChoicePeer(Choice target) {
	super(target);
    }

    /**
     * Minimum size
     */
    public Dimension minimumSize() {

        X11FontMetrics fm = (X11FontMetrics)getFontMetrics( target.getFont() );
	Choice c = (Choice)target;
	int width = 0;
        int height = 0;
        Dimension size = new Dimension();
        
	for (int i = c.countItems() ; i-- > 0 ;) {
            size = fm.stringExtent( c.getItem(i) );
	    width  = Math.max( size.width, width );
            height = Math.max( size.height, height );
	}

        size.width = width + 10 + 18;
        size.height = height + 10;
        return size;
    }

    /**
     * Paint the choice
     */
    public void paint(Graphics g) {
	Choice ch = (Choice)target;
	Dimension size = target.size();

	g.setColor(Color.lightGray);
	g.fillRect(1, 1, size.width-2, size.height-2);
	g.draw3DRect(0, 0, size.width-1, size.height-1, true);
	g.draw3DRect(size.width - 18, (size.height / 2) - 3, 12, 6, true);

	g.setColor(Color.black);
	g.setFont(ch.getFont());
	FontMetrics fm = g.getFontMetrics();
	String lbl = ch.getSelectedItem();
	g.drawString(lbl, 5, (size.height + fm.getAscent()) / 2);
	target.paint(g);
    }	

    protected void handleMouseDown(long when, int data,  int x, int y,
				   int xroot, int yroot,
				   int clickCount, int flags) {
	if (!target.isEnabled()) {
	    return;
	}
	Dimension size = target.size();
	Choice ch = (Choice)target;
	String items[] = new String[ch.countItems()];
	for (int i = 0 ; i < items.length ; i++) {
	    items[i] = ch.getItem(i);
	}

	Component f;

	for (f=target; f != null && !(f instanceof Frame);
	     f=f.getParent()) {
	}
	menu = new MChoiceMenu((Frame)f, this, xroot, yroot,
			       size.width - 20, size.height, 
			       ch.getSelectedIndex(), items);
	menu.show();
    }

    protected void handleMouseUp(long when, int data, int x, int y,
				 int xroot, int yroot, int flags) {
	if (!target.isEnabled()) {
	    return;
	}
	Choice ch = (Choice)target;
	ch.select(menu.selected);
	menu.hide();
	menu.dispose();
	menu = null;
	target.postEvent(new Event(target,Event.ACTION_EVENT, ch.getSelectedItem()));
    }

    protected void handleMouseDrag(long when, int data, int x, int y,
				   int xroot, int yroot, int flags) {
	if (!target.isEnabled()) {
	    return;
	}
	Rectangle b = menu.bounds();

	Point pt = new Point(xroot - b.x, yroot - b.y);

	menu.handleWindowEvent(new Event(target, when, Event.MOUSE_DRAG,
					 pt.x, pt.y, 0, flags, null));
    }

    public void select(int index) {
	target.repaint();
    }
    public void addItem(String item, int index) {
	target.repaint();
    }
}
