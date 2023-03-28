/*
 * @(#)CheckboxTest.java	1.2 95/08/06 Arthur van Hoff
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
 * A test of a Container with Checkboxes.
 */
public class CheckboxTest extends Frame {
    Panel p;

    public CheckboxTest() {
	super("Checkbox Test");
	p = new Panel();
	CheckboxGroup g1 = new CheckboxGroup();
	CheckboxGroup g2 = new CheckboxGroup();
	p.setLayout(new GridLayout(0, 3));
	p.add(new Checkbox("one", g1, true));
	p.add(new Checkbox("een", g2, true));
	p.add(new Checkbox("eins", null, true));
	p.add(new Checkbox("two", g1, false));
	p.add(new Checkbox("twee", g2, false));
	p.add(new Checkbox("zwei", null, false));
	p.add(new Checkbox("three", g1, false));
	p.add(new Checkbox("drie", g2, false));
	p.add(new Checkbox("drei", null, false));
	p.add(new Checkbox("four", g1, false));
	p.add(new Checkbox("vier", g2, false));
	p.add(new Checkbox("vier", null, false));
	p.add(new Checkbox("five", g1, false));
	p.add(new Checkbox("vijf", g2, false));
	p.add(new Checkbox("fumf", null, false));

	add("Center", p);
	add("South", new Button("list"));
	move(200, 100);
	pack();
	show();
    }
    
    public void list() {
	System.out.println("-- list of all check boxes that are turned on --");
	for (int i = 0 ; i < p.countComponents() ; i++) {
	    Checkbox comp = (Checkbox)p.getComponent(i);
	    if (comp.getState()) {
		System.out.println(comp.getLabel());
	    }
	}
    }

    public boolean handleEvent(Event evt) {
	if (evt.id == Event.ACTION_EVENT) {
	    if ("list".equals(evt.arg)) {
		list();
	    } else {
		System.out.println(evt.toString());
	    }
	    return true;
	}
	return false;
    }

    public static void main(String args[]) {
	new CheckboxTest();
    }
}
