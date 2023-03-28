/*
 * @(#)BorderTest.java	1.14 95/08/07 Sami Shaio
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
public class BorderTest extends Frame {
    public BorderTest() {
	super("BorderTest");
	setLayout(new BorderLayout(5, 5));
	add("North", new Button("North"));
	add("Center", new Button("Center"));
	add("South", new Button("South"));
	add("West", new Button("West"));
	add("East", new Button("East"));
	move(200, 100);
	pack();
	show();
    }

    public boolean handleEvent(Event evt) {
	System.out.println(evt.toString());
	return true;
    }

    public static void main(String args[]) {
	new BorderTest();
    }
}
