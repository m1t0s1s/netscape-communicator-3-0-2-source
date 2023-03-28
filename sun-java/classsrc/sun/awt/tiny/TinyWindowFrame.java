/*
 * @(#)TinyWindowFrame.java	1.3 95/12/01 Arthur van Hoff
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
import java.util.Vector;

class TinyWindowFrameWarning extends TinyWindow {
    String warning;

    final static Color warnBack = Color.black;
    final static Color warnFore = Color.red;

    final static Font warnFont = new Font("Dialog", Font.BOLD, 10);

    TinyWindowFrameWarning(TinyWindow parent, String warning) {
	this.warning = warning;
	winCreate(parent);
	winShow();
	winBackground(warnBack);
    }

    void paint(Graphics g) {
	g.setFont(warnFont);
	g.setColor(warnFore);

	FontMetrics fm = g.getFontMetrics();
	g.drawString(warning, 5, (height + fm.getAscent()) / 2);
    }
}

class TinyWindowFrame extends TinyWindow {
    TinyWindow border;
    TinyWindowPeer content;
    TinyWindow menubar;
    TinyWindow focus;
    TinyWindow warning;

    boolean resizable;
    boolean selected;
    String title;

    int mx, my;
    int mode;

    final static Vector allWindows = new Vector();

    final static int INDRAG = 0;
    final static int INRESIZE = 1;

    final static int TITLE = 19;
    final static int MENU = 27;
    final static int WARN = 20;
    final static int BORDER = 6;
    final static int BORDER1 = BORDER - 1;
    final static int BORDER2 = BORDER*2 - 1;
    final static int BORDER3 = TITLE - 1;

    final static Color frameBlack = Color.black;
    final static Color frameDark = Color.gray;
    final static Color frameMedium = new Color(160, 160, 160);
    final static Color frameLight = Color.lightGray;
    final static Color frameWhite = Color.white;
    final static Color frameSelect = frameMedium;

    final static Font titleFont = new Font("Dialog", Font.PLAIN, 14);

    /**
     * Create a window frame.
     */
    TinyWindowFrame(TinyWindowPeer content, String title, String warning, boolean resizable) {
	allWindows.addElement(this);
	this.content = content;
	this.title = title;
	this.resizable = resizable;

	winCreate(null);
	winBackground(frameLight);

	border = new TinyWindow();
	border.winCreate(this);
	border.winShow();

	content.winCreate(border);
	content.winShow();

	if (warning != null) {
	    this.warning = new TinyWindowFrameWarning(this, warning);
	}
    }

    /**
     * Dispose
     */
    void dispose() {
	allWindows.removeElement(this);
	if (menubar != null) {
	    menubar.winDispose();
	}
	if (warning != null) {
	    warning.winDispose();
	}
	content.winDispose();
	super.winDispose();
    }

    /**
     * Compute the insets.
     */
    public Insets insets() {
	Insets in = new Insets(0, 0, 0, 0);

	if (title != null) {
	    in.top    += BORDER + TITLE;
	    in.bottom += BORDER;
	    in.left   += BORDER;
	    in.right  += BORDER;
	}

	if (menubar != null) {
	    in.top += MENU;
	}
	if (warning != null) {
	    in.top += WARN;
	}
	return in;
    }

    /**
     * Reshape the window frame and its contents.
     */
    public void winReshape(int x, int y, int width, int height) {
	Insets in = insets();
	super.winReshape(x, y, width, height);
	border.winReshape(in.left, in.top, 
			  width - (in.left + in.right), 
			  height - (in.top + in.bottom));
	content.winReshape(-in.left, -in.top, width, height);

	int top = (title != null) ? (BORDER + TITLE) : 0;

	if (menubar != null) {
	    menubar.winReshape(in.left, top, width - (in.left + in.right), MENU);
	    top += MENU;
	}

	if (warning != null) {
	    warning.winReshape(in.left, top, width - (in.left + in.right), WARN);
	}
    }

    /**
     * Set the cursor image.
     */
    public void setCursor(int cursorType) {
	winSetCursor(cursorType);
    }


    /**
     * Set the title.
     */
    void setTitle(String title) {
	this.title = title;

	Graphics g = new TinyGraphics(this);
	try {
	    paintTitle(g);
	} finally {
	    g.dispose();
	}
    }

    /**
     * Set resizable.
     */
    void setResizable(boolean resizable) {
	this.resizable = resizable;
	paint();
    }

    /**
     * Set the menubar.
     */
    void setMenuBar(TinyMenuBarPeer menubar) {
	if (this.menubar != null) {
	    this.menubar.winDispose();
	}
	this.menubar = menubar;
	if (menubar != null) {
	    menubar.init(this);
	}
	winReshape(x, y, width, height);
    }

    /**
     * Set the icon image.
     */
    public void setIconImage(Image im) {
	// REMIND
    }

    /**
     * Draw a 3D rect in the right colors.
     */
    void frameRect(Graphics g, int x, int y, int w, int h, boolean raised) {
	g.setColor(raised ? frameWhite : frameDark);
	g.drawLine(x, y, x + w, y);
	g.drawLine(x, y, x, y + h);
	g.setColor(raised ? frameDark : frameWhite);
	g.drawLine(x + w, y, x + w, y + h);
	g.drawLine(x, y + h, x + w, y + h);
    }

    /**
     * Paint the title
     */
    void paintTitle(Graphics g) {
	// title box
	g.setColor(selected ? frameSelect : frameLight);
	g.fillRect(BORDER + TITLE + 1, BORDER + 1, width - (BORDER*2 + TITLE*3 + 2), BORDER3 - 1);
	g.setColor(frameLight);
	frameRect(g, BORDER + TITLE, BORDER, width - (BORDER*2 + TITLE*3 + 1), BORDER3, true);

	// title
	int w = width - (BORDER2 + TITLE*3 + 2);
	g.clipRect(BORDER + TITLE, BORDER, w, TITLE);
	g.setColor(frameBlack);
	g.setFont(titleFont);
	FontMetrics fm = g.getFontMetrics();
	g.drawString(title, BORDER + TITLE + 5 + (w - fm.stringWidth(title)) / 2,
		     BORDER + (TITLE + fm.getAscent()) / 2);
    }

    /**
     * Paint the window borders.
     */
    void paint(Graphics g) {
	if (title == null) {
	    return;
	}

	// background
	g.setColor(selected ? frameSelect : frameLight);
	g.fillRect(0, 0, width, height);
	g.setColor(frameLight);

	// borders
	frameRect(g, 0, 0, width-1, height-1, true);
	frameRect(g, BORDER1, BORDER1, width - BORDER2, height - BORDER2, false);

	// menu box
	frameRect(g, BORDER, BORDER, BORDER3, BORDER3, true);
	frameRect(g, BORDER + 4, BORDER + 7, TITLE - 9, TITLE - 16, true);

	// grow box
	frameRect(g, width - (BORDER + TITLE), BORDER, BORDER3, BORDER3, true);
	frameRect(g, width - (BORDER + TITLE - 4), BORDER + 4, TITLE - 9, TITLE - 9, true);

	// icon box
	frameRect(g, width - (BORDER + TITLE*2), BORDER, BORDER3, BORDER3, true);
	frameRect(g, width - (BORDER + TITLE*2 - 7), BORDER + 7, TITLE - 15, TITLE - 15, true);

	if (resizable) {
	    frameRect(g, 0, BORDER + TITLE - 1, BORDER1, 1, false);
	    frameRect(g, width-BORDER, BORDER + TITLE - 1, BORDER1, 1, false);
	    frameRect(g, 0, height - (BORDER + TITLE - 1), BORDER1, 1, false);
	    frameRect(g, width-BORDER, height - (BORDER + TITLE - 1), BORDER1, 1, false);
	    frameRect(g, BORDER + TITLE - 1, 0, 1, BORDER1, false);
	    frameRect(g, width - (BORDER + TITLE + 1), 0, 1, BORDER1, false);
	    frameRect(g, BORDER + TITLE - 1, height-BORDER, 1, BORDER1, false);
	    frameRect(g, width - (BORDER + TITLE + 1), height-BORDER, 1, BORDER1, false);
	}

	// paint the title
	paintTitle(g);
    }

    /**
     * Paint the window borders.
     */
    void paint() {
	Graphics g = new TinyGraphics(this);
	try {
	    paint(g);
	} finally {
	    g.dispose();
	}
    }

    /**
     * Assign the focus to a new window.
     */
    void requestFocus(TinyWindow newFocus) {
	if (newFocus != focus) {
	    if (focus != null) {
		focus.handleWindowEvent(new Event(focus, Event.LOST_FOCUS, null));
	    }
	    focus = newFocus;
	    if (focus != null) {
		focus.handleWindowEvent(new Event(focus, Event.GOT_FOCUS, null));
	    }
	}
    }

    /**
     * Check if this window has the focus.
     */
    boolean hasFocus(TinyWindow focus) {
	return focus == this.focus;
    }

    /**
     * The window now has the keyboard focus.
     */
    void handleFocusIn() {
	selected = true;
	if (title != null) {
	    paint();
	    if (focus != null) {
		focus.handleWindowEvent(new Event(focus, Event.GOT_FOCUS, null));
	    }
	}
    }

    /**
     * The window no longer has the keyboard focus.
     */
    void handleFocusOut() {
	selected = false;
	if (title != null) {
	    paint();
	    if (focus != null) {
		focus.handleWindowEvent(new Event(focus, Event.LOST_FOCUS, null));
	    }
	}
    }

    /**
     * Event handler.
     */
    boolean handleWindowEvent(Event evt) {
	switch (evt.id) {
	  case Event.KEY_PRESS:
	  case Event.KEY_RELEASE:
	  case Event.KEY_ACTION:
	  case Event.KEY_ACTION_RELEASE:
	    if (focus != null) {
		return focus.handleWindowEvent(evt);
	    }
	    return true;

	  case Event.MOUSE_ENTER:
	    winFocus();
	    return true;

	  case Event.MOUSE_DOWN:
	      mx = evt.x;
	      my = evt.y;
	      if ((mx > BORDER) && (mx < width - BORDER) && (my < BORDER + TITLE)) {
		  mode = INDRAG;
		  winFront();
	      } else {
		  mode = INRESIZE;
	      }
	      return true;

	  case Event.MOUSE_DRAG:
	      switch (mode) {
	      case INDRAG:
		  content.target.move(x + (evt.x - mx), y + (evt.y - my));
		  break;	

	      case INRESIZE:
		  content.target.resize(evt.x, evt.y);
		  content.target.validate();
		  content.target.repaint();
		  break;
	      }
	      return true;
	}
	return false;
    }
}
