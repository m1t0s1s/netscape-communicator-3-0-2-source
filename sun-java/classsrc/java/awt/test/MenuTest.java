/*
 * @(#)MenuTest.java	1.3 95/08/06 Arthur van Hoff
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

/**
 * A test of a window with menus.
 */
public class MenuTest extends Frame {
    Label lbl;

    public MenuTest() {
	super("MenuTest");
	MenuBar mb = new MenuBar();
	Menu m = new Menu("English");
	m.add(new MenuItem("one"));
	m.add(new MenuItem("two"));
	m.add(new MenuItem("three"));
	m.add(new MenuItem("four"));

	Menu submenu = new Menu("more");
	submenu.add(new MenuItem("five"));
	submenu.add(new MenuItem("six"));
	m.add(submenu);

	Menu submenu2 = new Menu("more");
	submenu2.add(new MenuItem("seven"));
	submenu2.add(new MenuItem("eight"));
	submenu.add(submenu2);
	mb.add(m);

	m = new Menu("Nederlands");
	m.add(new MenuItem("een"));
	m.add(new MenuItem("twee"));
	m.add(new MenuItem("drie"));
	m.add(new MenuItem("vier"));
	mb.add(m);

	m = new Menu("Deutsch");
	m.add(new CheckboxMenuItem("eins"));
	m.add(new CheckboxMenuItem("twei"));
	m.add(new CheckboxMenuItem("drie"));
	m.add(new CheckboxMenuItem("vier"));
	mb.add(m);

	setMenuBar(mb);

	setLayout(new BorderLayout());
	add("Center", lbl = new Label("The last menu selection will be displayed here.", Label.CENTER));
	move(200, 100);
	pack();
	show();
    }

    public boolean handleEvent(Event evt) {
	if ((evt.id == Event.ACTION_EVENT) && (evt.target instanceof MenuItem)) {
	    lbl.setText(evt.target.toString());
	    return true;
	}
	return false;
    }

    public static void main(String args[]) {
	new MenuTest();
    }
}
