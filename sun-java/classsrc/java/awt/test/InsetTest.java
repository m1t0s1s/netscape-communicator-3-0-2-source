/*
 * @(#)InsetTest.java	1.3 95/10/13 Sami Shaio
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
     
class QFrame extends Frame {
    public QFrame(String t) {
	super(t);
    }

    public boolean handleEvent(Event e) {
	if (e.id == Event.WINDOW_DESTROY) {
	    System.exit(0);
	}
	return true;
    }
}

public class InsetTest {
    public static void main(String args[]) {
	MenuBar m2;

	int d = 100;
	Frame f1 = new QFrame("f1");
	Frame f2 = new QFrame("f2");
	Frame f3 = new QFrame("f3");
	Frame f4 = new QFrame("f4");

	m2 = new MenuBar();
	m2.add(new Menu("File"));
	f2.setMenuBar(m2);
	f1.reshape(0, 0, d, d);
	f2.reshape(d, 0, d, d);
	f3.reshape(0, d, d, d);
	f4.reshape(d, d, d, d);

	Dialog d1 = new Dialog(f1, "d1", false);
	Dialog d2 = new Dialog(f2, "d2", false);
	Dialog d3 = new Dialog(f3, "d3", false);
	Dialog d4 = new Dialog(f4, "d4", false);
	
	d1.reshape(4*d, 0, d, d);
	d2.reshape(5*d, 0, d, d);
	d3.reshape(4*d, d, d, d);
	d4.reshape(5*d, d, d, d);

	Window w1 = new Window(f1);
	Window w2 = new Window(f2);
	Window w3 = new Window(f3);
	Window w4 = new Window(f4);

	w1.reshape(2*d, 0, d, d);
	w1.setBackground(Color.white);
	w2.reshape(3*d, 0, d, d);
	w2.setBackground(Color.red);
	w3.reshape(2*d, d, d, d);
	w3.setBackground(Color.green);
	w4.reshape(3*d, d, d, d);
	w4.setBackground(Color.blue);

	f1.show();
	f2.show();
	f3.show();
	f4.show();

	d1.show();
	d2.show();
	d3.show();
	d4.show();

	w1.show();
	w2.show();
	w3.show();
	w4.show();
    }
}

		   
	
