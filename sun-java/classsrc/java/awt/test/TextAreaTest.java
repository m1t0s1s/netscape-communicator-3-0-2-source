/*
 * @(#)TextAreaTest.java	1.2 95/08/07 Arthur van Hoff
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
public class TextAreaTest extends Frame {
    TextArea t1;
    TextArea t2;

    public TextAreaTest() {
	super("TextAreaTest");
	add("North", t1 = new TextArea(4, 40));
	t1.setForeground(Color.blue);
	t1.setEditable(false);
	add("Center", t2 = new TextArea(10, 40));
	t2.setFont(new Font("Helvetica", Font.BOLD, 18));
	Panel p = new Panel();
	p.add(new Button("clear"));
	p.add(new Button("reset"));
	p.add(new Button("select"));
	p.add(new Button("print"));
	add("South", p);
	move(200, 100);
	pack();
	show();
    }

    public boolean handleEvent(Event evt) {
	if (evt.id == Event.ACTION_EVENT) {
	    if ("clear".equals(evt.arg)) {
		t1.setText("");
		t2.setText("");
		return true;
	    }
	    if ("reset".equals(evt.arg)) {
		t1.setText("Arthur van Hoff");
		t2.setText("Sami Shaio");
		return true;
	    }
	    if ("select".equals(evt.arg)) {
		t2.selectAll();
		return true;
	    }
	    if ("print".equals(evt.arg)) {
		System.out.println("-- values --");
		System.out.println("text=" + t1.getText());
		System.out.println("text=" + t2.getText());
		return true;
	    }
	}
	System.out.println(evt.toString());
	return true;
    }

    public static void main(String args[]) {
	new TextAreaTest();
    }
}
