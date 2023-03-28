/*
 * @(#)TextFieldTest.java	1.4 95/09/06 Arthur van Hoff
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
public class TextFieldTest extends Frame {
    TextField t1;
    TextField t2;
    TextField t3;

    public TextFieldTest() {
	super("TextFieldTest");
	
	Panel p = new Panel();
	p.setLayout(new GridLayout(0, 2));
	p.add(new Label("name"));
	p.add(t1 = new TextField("Beatrix Butterfly"));
	t1.setForeground(Color.blue);

	p.add(new Label("address"));
	p.add(t2 = new TextField("3 Green Fields Lane"));

	p.add(new Label("city"));
	p.add(t3 = new TextField("SunnyHills"));
	t3.setEditable(false);

	add("Center", p);

	p = new Panel();
	Button b;
	p.add(b = new Button("clear"));
	b.setForeground(Color.red);
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
		t2.setText("2235 Wyandotte St");
		return true;
	    }
	    if ("select".equals(evt.arg)) {
		t1.selectAll();
		return true;
	    }
	    if ("print".equals(evt.arg)) {
		System.out.println("-- values --");
		System.out.println("name=" + t1.getText());
		System.out.println("address=" + t2.getText());
		System.out.println("city=" + t3.getText());
		return true;
	    }
	}
	System.out.println(evt.toString());
	return true;
    }

    public static void main(String args[]) {
	new TextFieldTest();
    }
}
