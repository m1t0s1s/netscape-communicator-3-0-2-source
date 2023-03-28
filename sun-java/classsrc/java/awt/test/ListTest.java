/*
 * @(#)ListTest.java	1.4 95/09/01 Arthur van Hoff
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
 * A test of a Container with BorderLayout.
 */
public class ListTest extends Frame {
    List l1;
    List l2;
    Label lbl;

    public ListTest() {
	super("ListTest");
	l1 = new List();
	l1.addItem("one");
	l1.addItem("two");
	l1.addItem("three");
	l1.addItem("four");
	l1.addItem("five");
	l1.addItem("six");
	l1.addItem("seven");
	l1.addItem("eight");
	l1.addItem("nine");
	l1.addItem("ten");
	l1.setMultipleSelections(true);
	add("West", l1);

	l2 = new List(4, false);
	l2.addItem("een");
	l2.addItem("twee");
	l2.addItem("drie");
	l2.addItem("vier");
	l2.addItem("vijf");
	l2.addItem("zes");
	l2.addItem("zeven");
	l2.addItem("acht");
	l2.addItem("negen");
	l2.addItem("tien");
	add("Center", l2);

	Panel p = new Panel();
	p.add(new Button("print"));
	p.add(lbl = new Label("--selection--"));
	add("South", p);

	move(200, 100);
	pack();
	show();
    }

    public boolean handleEvent(Event evt) {
	switch (evt.id) {
	  case Event.LIST_SELECT:
	    System.out.println("List select: " + ((Integer)evt.arg).intValue());
	    return true;
	  case Event.LIST_DESELECT:
	    System.out.println("List deselect: " + ((Integer)evt.arg).intValue());
	    return true;
	  case Event.WINDOW_DESTROY:
	    System.exit(0);
	    return true;
	  default:
	    return super.handleEvent(evt);
	}
    }

    public boolean action(Event evt, Object arg) {
	if (evt.id == Event.ACTION_EVENT) {
	    if ("print".equals(evt.arg)) {
		System.out.println("-- list 1 --");
		String sel[] = l1.getSelectedItems();
		for (int i = 0 ; i < sel.length ; i++) {
		    System.out.println(sel[i]);
		}
		System.out.println("-- list 2 --");
		sel = l2.getSelectedItems();
		for (int i = 0 ; i < sel.length ; i++) {
		    System.out.println(sel[i]);
		}
	    }
	}

	lbl.setText((String)arg);
	return true;
    }

    public static void main(String args[]) {
	new ListTest();
    }
}
