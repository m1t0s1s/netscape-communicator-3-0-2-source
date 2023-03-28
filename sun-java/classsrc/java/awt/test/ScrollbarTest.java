/*
 * @(#)ScrollbarTest.java	1.2 95/08/16 Arthur van Hoff
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

class ScrollablePaintCanvas extends Canvas {
    int tx = 20;
    int ty = 100;
    static final int SIZE = 500;

    public void paint(Graphics g) {
	g.translate(-tx, -ty);
	for (int i = 0 ; i < 200 ; i += 20) {
	    g.drawOval(i, i, SIZE - 2*i, SIZE - 2*i);
	}
	g.setColor(Color.yellow);
	g.drawLine(0, SIZE/2, SIZE, SIZE/2);
	g.drawLine(SIZE/2, 0, SIZE/2, SIZE);

	g.setColor(Color.red);
	g.fillOval(SIZE/2 - 20, SIZE/2 - 20, 40, 40);
    }
}

/**
 * A test of a Container with BorderLayout.
 */
public class ScrollbarTest extends Frame {
    Scrollbar vert;
    Scrollbar horz;
    ScrollablePaintCanvas cv;

    public ScrollbarTest() {
	super("ScrollbarTest");
	add("Center", cv = new ScrollablePaintCanvas());
	add("East", vert = new Scrollbar(Scrollbar.VERTICAL, cv.ty, 100, 0, ScrollablePaintCanvas.SIZE));
	add("South", horz = new Scrollbar(Scrollbar.HORIZONTAL, cv.tx, 100, 0, ScrollablePaintCanvas.SIZE));
	reshape(100, 200, 300, 250);
	show();
    }

    public boolean handleEvent(Event evt) {
	if (evt.target == vert) {
	    cv.ty = ((Integer)evt.arg).intValue();
	    cv.repaint();
	    return true;
	}
	if (evt.target == horz) {
	    cv.tx = ((Integer)evt.arg).intValue();
	    cv.repaint();
	    return true;
	}
	return false;
    }

    public static void main(String args[]) {
	new ScrollbarTest();
    }
}
