/*
 * @(#)ListTest2.java	1.2 95/09/01 Sami Shaio
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
public class ListTest2 extends Frame {
    List list;
    TextField text;

    public ListTest2() {
	super("ListTest2");

	list = new List();
	list.addItem("one");
	list.addItem("two");
	list.addItem("three");
	list.addItem("four");
	list.addItem("five");
	list.addItem("six");
	list.addItem("seven");
	list.addItem("eight");
	list.addItem("nine");
	list.addItem("ten");
	add("Center", list);

	Panel p = new Panel();
	p.add(text=new TextField(20));
	p.add(new Checkbox("multiple"));
	p.add(new Button("add"));
	p.add(new Button("del"));
	p.add(new Button("clear"));
	p.add(new Button("select"));
	p.add(new Button("deselect"));
	p.add(new Button("print"));
	add("South", p);

	move(200, 100);
	resize(300, 300);
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

    int argToIndex(String arg) {
	int n = list.countItems();

	for (int i = 0; i < n; i++) {
	    if (arg.equals(list.getItem(i))) {
		return i;
	    }
	}
	return -1;
    }

    public boolean action(Event evt, Object arg) {
	if (evt.id == Event.ACTION_EVENT) {
	    if (evt.target instanceof Checkbox) {
		list.setMultipleSelections(((Boolean)arg).booleanValue());
	    } else if ("print".equals(evt.arg)) {
		System.out.println("-- list contents --");
		int n = list.countItems();
		for (int i=0; i < n; i++) {
		    System.out.println(list.getItem(i));
		}
		System.out.println("-- list selected --");		
		String sel[] = list.getSelectedItems();
		for (int i = 0 ; i < sel.length ; i++) {
		    System.out.println(sel[i]);
		}
	    } else if ("add".equals(evt.arg)) {
		list.addItem(text.getText());
	    } else if ("del".equals(evt.arg)) {
		int index = argToIndex(text.getText());

		if (index == -1) {
		    System.out.println("Invalid index");
		} else {
		    list.delItem(index);
		}
	    } else if ("select".equals(evt.arg)) {
		int index = argToIndex(text.getText());

		if (index == -1) {
		    System.out.println("Invalid index");
		} else {
		    list.select(index);
		}
	    } else if ("deselect".equals(evt.arg)) {
		int index = argToIndex(text.getText());

		if (index == -1) {
		    System.out.println("Invalid index");
		} else {
		    list.deselect(index);
		}
	    } else if ("clear".equals(evt.arg)) {
		list.clear();
	    }
	}

	return true;
    }

    public static void main(String args[]) {
	new ListTest2();
    }
}
