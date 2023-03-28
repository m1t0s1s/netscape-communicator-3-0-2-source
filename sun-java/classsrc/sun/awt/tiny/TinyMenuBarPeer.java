/*
 * @(#)TinyMenuBarPeer.java	1.3 95/11/23 Arthur van Hoff
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

public class TinyMenuBarPeer extends TinyWindow implements MenuBarPeer {
    TinyFramePeer frame;
    TinyMenuPeer menu;
    int menuSelected = -1;
    MenuBar	target;

    final static int GAP = 10;

    final static Color menuBlack = TinyWindowFrame.frameBlack;
    final static Color menuDark = TinyWindowFrame.frameDark;
    final static Color menuMedium = TinyWindowFrame.frameMedium;
    final static Color menuLight = TinyWindowFrame.frameLight;
    final static Color menuWhite = TinyWindowFrame.frameWhite;

    final static Font menuFont = new Font("Dialog", Font.PLAIN, 12);

    /**
     * Create a menu bar peer
     */
    public TinyMenuBarPeer(MenuBar target) {
	this.target = target;
    }

    /**
     * Show the menu bar in a window, it will be reshaped
     * to the correct size by the window.
     */
    void init(TinyWindow parent) {
	winCreate(parent);
	winBackground(menuLight);
	winShow();
    }

    /**
     * Add a menu.
     */
    public void addMenu(Menu m) {
	paint();
    }

    /**
     * Delete a menu.
     */
    public void delMenu(int index) {
	paint();
    }

    /**
     * Add help menu (ignore this for now).
     */
    public void addHelpMenu(Menu m) {
	paint();
    }

    /**
     * Dispose the peer. The window will be disposed
     * when the frame is disposed.
     */
    public void dispose() {
    }

    /**
     * Select which menu we are in.
     */
    boolean selectMenu(Event evt, int x, int y) {
	if (y > height) {
	    return menu != null;

	}
	FontMetrics fm = TinyFontMetrics.getFontMetrics(menuFont);
	int nitems = target.countMenus();
	int mx = 0;
		
	for (int i = 0 ; i < nitems ; i++) {
	    Menu mn = target.getMenu(i);
	    String item = mn.getLabel();
	    int w = fm.stringWidth(item);
	    if (mn.isEnabled() && (x > mx) && (x <= mx + w + GAP*2)) {
		if (menu != mn.getPeer()) {
		    if (menu != null) {
			menu.popdown(evt, false);
		    }
		    Point pt = toGlobal(mx + 1, height - 3);
		    menu = (TinyMenuPeer)mn.getPeer();
		    menu.popup(pt.x, pt.y);
		    menuSelected = i;
		    paint();
		}
		return true;
	    }
	    mx += w + GAP*2;
	}
	return menu != null;
    }

    /**
     * Paint the menu
     */
    void paint(Graphics g) {
	g.setColor(menuWhite);
	g.drawLine(0, 0, width-1, 0);
	g.drawLine(0, 0, 0, height-1);
	g.setColor(menuDark);
	g.drawLine(width-1, 0, width-1, height-1);
	g.drawLine(0, height-1, width-1, height-1);

	g.setFont(menuFont);
	FontMetrics fm = g.getFontMetrics();

	int x = GAP;
	int y = (height + fm.getAscent()) / 2;
	int nitems = target.countMenus();

	for (int i = 0 ; i < nitems ; i++) {
	    Menu mn = target.getMenu(i);
	    String item = mn.getLabel();
	    int w = fm.stringWidth(item) + GAP*2;
	    if (i == menuSelected) {
		g.setColor(menuLight);
		g.draw3DRect((x - GAP) + 1, 3, w - 2, height - 7, false);
		g.setColor(menuMedium);
		g.fillRect((x - GAP) + 2, 4, w - 4, height - 9);
	    }
	    g.setColor(mn.isEnabled() ? menuBlack : menuDark);
	    g.drawString(item, x, y);
	    x += w;
	    if (x >= width) {
		break;
	    }
	}
    }

    /**
     * Paint the menu
     */
    void paint() {
	Graphics g = new TinyGraphics(this);
	try {
	    g.setColor(menuLight);
	    g.fillRect(1, 1, width-2, height-2);
	    paint(g);
	} finally {
	    g.dispose();
	}
    }

    /**
     * Event handler.
     */
    boolean handleWindowEvent(Event evt) {
	switch (evt.id) {
	  case Event.MOUSE_DOWN:
	    selectMenu(evt, evt.x, evt.y);
	    return true;

	  case Event.MOUSE_DRAG:
	    if (selectMenu(evt, evt.x, evt.y)) {
		Point pt = menu.toLocal(toGlobal(evt.x, evt.y));
		evt.x = pt.x;
		evt.y = pt.y;
		return menu.handleWindowEvent(evt);
	    }
	    return true;

	  case Event.MOUSE_UP:
	    if (menu != null) {
		menu.popdown(evt, true);
		menu = null;
		menuSelected = -1;
		paint();
	    }
	    return true;
	}
	return false;
    }
}
