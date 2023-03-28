/*
 * @(#)TinyMenuPeer.java	1.3 95/11/23 Arthur van Hoff
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

public class TinyMenuPeer extends TinyWindow implements MenuPeer {
    TinyMenuPeer menu;
    int selected;
    Menu target;
    int h;

    /**
     * Create a menu
     */
    public TinyMenuPeer(Menu target) {
	this.target = target;
    }

    /**
     * Compute the width and height of the menu
     */
    void computeSize(int x, int y) {
	FontMetrics fm = TinyFontMetrics.getFontMetrics(TinyMenuBarPeer.menuFont);
	int nitems = target.countItems();
	h = fm.getHeight() + 6;

	int w = 40;
	for (int i = 0 ; i < nitems ; i++) {
	    String str = target.getItem(i).getLabel();
	    w = Math.max(w, fm.stringWidth(str) + 40);
	}
	
	winReshape(x, y, w, 4 + h * nitems);
    }

    /**
     * Show at an x,y location.
     */
    void popup(int x, int y) {
	selected = -1;
	winCreate(null);
	winBackground(TinyWindowFrame.frameLight);
	computeSize(x, y);
	winFront();
	winShow();
    }

    /**
     * Hide the menu and execute the action if
     * action is true.
     */
    void popdown(Event evt, boolean action) {
	winHide();
	winDispose();
	if (menu != null) {
	    menu.popdown(evt, action);
	    menu = null;
	} else if (action && (selected >= 0)) {
	    MenuItem mi = target.getItem(selected);
	    if (mi instanceof CheckboxMenuItem) {
		CheckboxMenuItem cb = (CheckboxMenuItem)mi;
		cb.setState(!cb.getState());
	    }
	    if (!(mi instanceof Menu)) {
		try {
		    mi.postEvent(new Event(mi, evt.when, Event.ACTION_EVENT, 0, 0, 0, evt.modifiers, mi.getLabel()));
		} catch (ThreadDeath e) {
		    throw e;
		} catch (Throwable e) {
		    e.printStackTrace();
		}
	    }
	}
	selected = -1;
    }

    /**
     * Paint an item
     */
    void paintItem(Graphics g, FontMetrics fm, int i) {
	int y = 2 + (h*i) + (h + fm.getAscent()) / 2;
	MenuItem mi = target.getItem(i);
	String str = mi.getLabel();

	if (str.equals("-")) {
	    g.setColor(TinyWindowFrame.frameLight);
	    g.draw3DRect(1, y - fm.getAscent() / 2, width-3, 2, false);
	} else {
	    if (i == selected) {
		g.setColor(TinyWindowFrame.frameLight);
		g.draw3DRect(2, 2 + i * h, width-5, h-1, false);
		g.setColor(TinyWindowFrame.frameMedium);
		g.fillRect(3, 3 + i * h, width-7, h-3);
	    } else {
		g.setColor(TinyWindowFrame.frameLight);
		g.fillRect(2, 2 + i * h, width-4, h);
	    }
	    g.setColor(mi.isEnabled() ? 
		       TinyWindowFrame.frameBlack : 
		       TinyWindowFrame.frameDark);
	    g.drawString(str, 10, y);

	    if (mi.isEnabled() && (mi instanceof Menu)) {
		int ax = width - 20;
		g.setColor(TinyWindowFrame.frameLight.darker());
		g.drawLine(ax, y, ax + 10, y - 5);
		g.setColor(TinyWindowFrame.frameLight.brighter());
		g.drawLine(ax + 10, y - 5, ax, y - 10);
		g.drawLine(ax, y - 10, ax, y);
	    } else if (mi instanceof CheckboxMenuItem) {
		g.setColor(TinyWindowFrame.frameLight);
		boolean state = ((CheckboxMenuItem)mi).getState();
		g.draw3DRect(width - 20, y - 10, 10, 10, !state);
		g.setColor(state ? TinyWindowFrame.frameMedium : TinyWindowFrame.frameLight);
		g.fillRect(width - 19, y - 9, 9, 9);
	    }
	}
    }

    /**
     * Paint the entire menu.
     */
    void paint(Graphics g) {
	g.setColor(TinyWindowFrame.frameLight);
	g.draw3DRect(0, 0, width-1, height-1, true);
	g.draw3DRect(1, 1, width-3, height-3, true);
	g.setFont(TinyMenuBarPeer.menuFont);
	FontMetrics fm = g.getFontMetrics();

	int nitems = target.countItems();
	for (int i = 0 ; i < nitems ; i++) {
	    paintItem(g, fm, i);
	}
    }

    /**
     * Select an item.
     */
    void select(int i) {
	if (i == selected) {
	    return;
	}

	Graphics g = new TinyGraphics(this);
	try {
	    g.setColor(TinyWindowFrame.frameLight);
	    g.setFont(TinyMenuBarPeer.menuFont);
	    FontMetrics fm = g.getFontMetrics();
	    if (selected >= 0) {	
		int old = selected;
		selected = -1;
		paintItem(g, fm, old);
	    }
	    if (i < 0) {
		return;
	    }

	    MenuItem mi = target.getItem(i);
	    if (mi.isEnabled() && !mi.getLabel().equals("-")) {
		paintItem(g, fm, selected = i);
	    } else {
		selected = -1;
	    }
	} finally {
	    g.dispose();
	}
    }

    /**
     * Handle events.
     */
    boolean handleWindowEvent(Event evt) {
	switch (evt.id) {
	  case Event.MOUSE_DRAG:
	    if (menu != null) {
		if (evt.x < width - 20) {
		    menu.popdown(evt, false);
		    menu = null;
		} else {
		    Point pt = menu.toLocal(toGlobal(evt.x, evt.y));
		    evt.x = pt.x;
		    evt.y = pt.y;
		    return menu.handleWindowEvent(evt);
		}
	    }
	    if ((evt.x < 0) || (evt.y < 0)) {
		select(-1);
		return true;
	    }

	    int nitems = target.countItems();
	    int i = Math.max(0, Math.min(nitems-1, (evt.y - 2)/h));
	    select(i);

	    if ((evt.x > width - 20) && (selected >= 0)) {
		MenuItem mi = target.getItem(selected);

		if ((mi instanceof Menu) && (mi.isEnabled())) {
		    menu = (TinyMenuPeer)mi.getPeer();
		    Point pt = toGlobal(width - 20, 2 + selected*h);
		    menu.popup(pt.x, pt.y);
		}
	    }
	    return true;
	}
	return false;
    }

    public void addSeparator() {
    }
    public void addItem(MenuItem item) {
    }
    public void delItem(int index) {
    }
    public void setLabel(String label) {
    }
    public void dispose() {
    }
    public void enable() {
    }
    public void disable() {
    }
}
