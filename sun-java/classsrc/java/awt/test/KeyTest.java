/*
 * @(#)KeyTest.java	1.5 95/10/03 Sami Shaio
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
import java.awt.*;

public class KeyTest extends Frame {
    String	value = "";
    Color	keyColor = Color.green;
    Font	font = new Font("Helvetica", Font.BOLD, 24);

    public KeyTest() {
	resize(300, 300);
	show();
    }

    public static void main(String args[]) {
	new KeyTest();
    }

    public void paint(Graphics g) {
	g.setColor(keyColor);
	g.setFont(font);
	Rectangle bounds = bounds();
	FontMetrics fm = g.getFontMetrics(font);
	
	g.drawString(value,
		     ((bounds.width - fm.stringWidth(value))/ 2),
		     ((bounds.height - fm.getHeight()) / 2));
    }

    void displayKey(int key, int modifiers, boolean pressed) {
	String keyString;

	char s[] = new char[1];

	if (pressed) {
	    keyColor = Color.red;
	} else {
	    keyColor = Color.black;
	}
	if ((modifiers&Event.CTRL_MASK) != 0) {
	    s[0] = (char)(key + 'a' - 1);
	} else {
	    s[0] = (char)key;
	}
	keyString = new String(s);

	value = modifierString(modifiers) + keyString + "[" + key + "]";
	repaint();
    }

    String modifierString(int modifiers) {

	String mstr = "";
	if ((modifiers&Event.CTRL_MASK) != 0) {
	    mstr = mstr+"Ctrl-";
	}
	if ((modifiers & Event.SHIFT_MASK) != 0) {
	    mstr = mstr+"Shift-";
	}
	if ((modifiers & Event.META_MASK) != 0) {
	    mstr = mstr+"Meta-";
	}
	if ((modifiers & Event.ALT_MASK) != 0) {
	    mstr = mstr+"Alt-";
	}

	return mstr;
    }

    void displayKeyAction(Event e, boolean pressed) {
	String keyString;

	if (pressed) {
	    keyColor = Color.red;
	} else {
	    keyColor = Color.black;
	}
	switch (e.key) {
	  case Event.HOME:
	    keyString = "Home";
	    break;
	  case Event.END:
	    keyString = "End";
	    break;
	  case Event.PGUP:
	    keyString = "PageUp";
	    break;
	  case Event.PGDN:
	    keyString = "PageDown";
	    break;
	  case Event.UP:
	    keyString = "Up";
	    break;
	  case Event.DOWN:
	    keyString = "Down";
	    break;
	  case Event.LEFT:
	    keyString = "Left";
	    break;
	  case Event.RIGHT:
	    keyString = "Right";
	    break;
	  case Event.ESC:
	    keyString = "ESC";
	    break;
	  case Event.F1:
	    keyString = "F1";
	    break;
	  case Event.F2:
	    keyString = "F2";
	    break;
	  case Event.F3:
	    keyString = "F3";
	    break;
	  case Event.F4:
	    keyString = "F4";
	    break;
	  case Event.F5:
	    keyString = "F5";
	    break;
	  case Event.F6:
	    keyString = "F6";
	    break;
	  case Event.F7:
	    keyString = "F7";
	    break;
	  case Event.F8:
	    keyString = "F8";
	    break;
	  case Event.F9:
	    keyString = "F9";
	    break;
	  case Event.F10:
	    keyString = "F10";
	    break;
	  case Event.F11:
	    keyString = "F11";
	    break;
	  case Event.F12:
	    keyString = "F12";
	    break;
	  default:
	    keyString = "???";
	    break;
	}
	value = modifierString(e.modifiers) + keyString;
	repaint();
    }

    void displayMouse(Event e) {
	value = modifierString(e.modifiers) + "Button (" + e.x + ", "
	    + e.y + ")";
	switch (e.id) {
	  case Event.MOUSE_DRAG:
	    value = value + "...";
	  case Event.MOUSE_DOWN:
	    keyColor = Color.red;
	    break;
	  case Event.MOUSE_MOVE:
	    value = value + "'''";
	  case Event.MOUSE_UP:
	    keyColor = Color.black;
	    break;
	  default:
	    break;
	}
	repaint();
    }

    public boolean handleEvent(Event e) {
	switch (e.id) {
	  case Event.WINDOW_DESTROY:
	    System.exit(0);
	    return true;
	  case Event.MOUSE_DOWN:
	  case Event.MOUSE_UP:
	  case Event.MOUSE_MOVE:
	  case Event.MOUSE_DRAG:
	    displayMouse(e);
	    return true;
	  case Event.KEY_PRESS:
	  case Event.KEY_RELEASE:
	    displayKey(e.key, e.modifiers, e.id==Event.KEY_PRESS);
	    return true;
	  case Event.KEY_ACTION:
	  case Event.KEY_ACTION_RELEASE:
	    displayKeyAction(e, e.id==Event.KEY_ACTION);
	    return true;
	  default:
	    return false;
	}
    }
}

