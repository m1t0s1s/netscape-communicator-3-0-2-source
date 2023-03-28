/*
 * @(#)ChoiceTest.java	1.1 95/08/06 Arthur van Hoff
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
 * A test of Choice components.
 */
public class ChoiceTest extends Frame {
    Choice c1;
    Choice c2;

    public ChoiceTest() {
	super("ChoiceTest");
	c1 = new Choice();
	c1.addItem("one");
	c1.addItem("two");
	c1.addItem("three");
	c1.addItem("four");
	add("North", c1);

	c2 = new Choice();
	c2.addItem("een");
	c2.addItem("twee");
	c2.addItem("drie");
	c2.addItem("vier");
	add("Center", c2);

	add("South", new Button("print"));
	move(200, 100);
	pack();
	show();
    }

    public boolean action(Event evt, Object arg) {
	if ("print".equals(arg)) {
	    System.out.println("-- selected items --");
	    System.out.println(c1.getSelectedItem());
	    System.out.println(c2.getSelectedItem());
	    return true;
	}
	System.out.println(evt.toString());
	return false;
    }

    public static void main(String args[]) {
	new ChoiceTest();
    }
}
