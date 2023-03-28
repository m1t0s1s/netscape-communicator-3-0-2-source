/*
 * @(#)PaintTest.java	1.20 95/09/16 Sami Shaio
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

public class PaintTest extends Frame {
    public PaintTest() {
	setTitle("PaintTest");
	setBackground(Color.lightGray);
	setLayout(new BorderLayout());
	add("Center", new PaintPanel());
	reshape(0, 0, 300, 300);
	show();
    }
	    
    public static void main(String args[]) {
	new PrintTest(new PaintTest());
    }
}

class PaintPanel extends Panel {
    public PaintPanel() {
	setLayout(new BorderLayout());
	Panel grid = new Panel();
	grid.setLayout(new GridLayout(0, 1));
	add("Center", grid);
	grid.add(new PaintCanvas());
	grid.add(new PaintCanvas());
	grid.add(new PaintCanvas());
	reshape(0, 0, 20, 20);
    }
}

class PaintCanvas extends Canvas {    
    int 	hadd = 0;
    int		xadd = 0;
    int		yadd = 0;
    boolean	focus = false;

    public PaintCanvas() {
	reshape(0, 0, 100, 100);
    }

    public boolean gotFocus(Event e, Object arg) {
	focus = true;
	repaint();
	return true;
    }

    public boolean lostFocus(Event e, Object arg) {
	focus = false;
	repaint();
	return true;
    }

    public void paint(Graphics g) {
	Rectangle r = bounds();
	int hlines = r.height / 10;
	int vlines = r.width / 10;

	g.setColor(Color.pink);
	for (int i = 1; i <= hlines; i++) {
	    g.drawLine(0, i * 10, r.width, i * 10);
	}
	for (int i = 1; i <= vlines; i++) {
	    g.drawLine(i * 10, 0, i * 10, r.height);
	}
	if (focus) {
	    g.setColor(Color.red);
	} else {
	    g.setColor(Color.darkGray);
	}
	g.drawRect(0, 0, r.width-1, r.height-1);
	g.drawLine(0, 0, r.width, r.height);
	g.drawLine(r.width, 0, 0, r.height);
	g.drawLine(0, r.height / 2, r.width, r.height / 2);
	g.drawLine(r.width / 2, 0, r.width / 2, r.height);

	for (int i = 0; i < 360; i += 30) {
	    float percent = (((float)i /(float)360) * (float)255);
	    g.setColor(new Color((int)percent, (int)(percent / 2),
				      (int)percent));
	    g.fillArc(xadd, yadd, 100 + hadd, 100 + hadd, i, 30);
	}
    }

    public boolean handleEvent(Event e) {
	switch (e.id) {
	  case Event.KEY_ACTION:
	  case Event.KEY_PRESS:
	    switch (e.key) {
	      case Event.UP:
		yadd -= 5;
		repaint();
		return true;
	      case Event.DOWN:
		yadd += 5;
		repaint();
		return true;
	      case Event.RIGHT:
	      case 'r':
		xadd += 5;
		repaint();
		return true;
	      case Event.LEFT:
		xadd -= 5;
		repaint();
		return true;
	      case Event.PGUP:
		hadd += 10;
		repaint();
		return true;
	      case Event.PGDN:
		hadd -= 10;
		repaint();
		return true;
	      default:
		return false;
	    }

	  default:
	    return super.handleEvent(e);
	}
    }

}

class PrintTest extends Frame {
    Component comp;

    public PrintTest(Component comp) {
	this.comp = comp;
	reshape(400, 0, 400, 400);
	show();
    }

    public void paint(Graphics g) {
	Dimension d = size();
	g.setColor(getBackground());
	g.fillRect(0, 0, d.width, d.height);
	comp.printAll(g);
    }
}
