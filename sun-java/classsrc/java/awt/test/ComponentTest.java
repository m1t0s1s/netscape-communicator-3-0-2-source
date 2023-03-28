/*
 * @(#)ComponentTest.java	1.31 95/09/01 Sami Shaio
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
import java.util.Date;

public class ComponentTest extends Frame {
    Panel 	center;
    QuitDialog	quitDialog;
    AboutBox	aboutBox;
    FileDialog	openDialog;
    FileDialog	saveDialog;

    public ComponentTest() {
	aboutBox = new AboutBox(this);
	quitDialog = new QuitDialog(this);
	openDialog = new FileDialog(this, "Open File...");
	saveDialog = new FileDialog(this, "Save File...", FileDialog.SAVE);
	setTitle("ComponentTest");
	setResizable(false);
	setBackground(Color.lightGray);
	setFont(new Font("Helvetica", Font.PLAIN, 12));
	setLayout(new BorderLayout());

	MenuBar	mb = new MenuBar();
	Menu   m = new Menu("File");

	m.add(new MenuItem("About ComponentTest"));
	m.add(new MenuItem("Open..."));
	m.add(new MenuItem("Save..."));
	m.add(new MenuItem("Quit"));
	mb.add(m);
	setMenuBar(mb);

	Label l  = new Label("North");
	l.setAlignment(Label.CENTER);

	add("South", new TextField("South"));
	add("North", l);
	center = new ScrollPanel();
	center.setLayout(new BorderLayout());
	add("Center", center);

	Panel innerPanel = new ScrollPanel();
	center.add("Center", innerPanel);
	innerPanel.setLayout(new FlowLayout(FlowLayout.CENTER, 5, 5));
	innerPanel.add(new Label("List of Widgets"));
	innerPanel.add(new Button("Arthur"));
	innerPanel.add(new Button("Sami"));
	innerPanel.add(new SillyWidget());
	innerPanel.add(new TextField("quite random"));
	innerPanel.add(new Scrollbar(Scrollbar.VERTICAL));
	innerPanel.add(new Scrollbar(Scrollbar.HORIZONTAL));

	List li = new List(4, false);

	li.reshape(0, 0, 100, 100);
	li.addItem("Arthur");
	li.addItem("Sami");
	li.addItem("James");
	li.addItem("Chris");
	li.addItem("Mary");
	li.addItem("Lisa");
	li.addItem("Kathy");
	li.select(1);

	innerPanel.add(li);
	TextArea tx = new TextArea(10, 5);
	tx.setText("this is some random text");
	tx.setForeground(Color.red);

	innerPanel.add(tx);

	Choice	opt = new Choice();

	opt.addItem("Arthur");
	opt.addItem("Sami");
	opt.addItem("Chris");
	opt.addItem("Jim");

	innerPanel.add(opt);

	CheckboxGroup g = new CheckboxGroup();
	innerPanel.add(new Checkbox("one", g, false));
	innerPanel.add(new Checkbox("two", g, true));
	innerPanel.add(new Checkbox("three", g, false));
	center.add("East", new Scrollbar(Scrollbar.VERTICAL));
	center.add("South", new Scrollbar(Scrollbar.HORIZONTAL));

	add("East", new Button("East"));
	add("West", new Button("West"));
	resize(300, 500);
	show();
    }
	    
    public synchronized boolean handleEvent(Event e) {
	if (e.id == Event.WINDOW_DESTROY) {
	    quitDialog.show();
	    return true;
	} else {
	    return super.handleEvent(e);
	}
    }

    public synchronized void wakeUp() {
	notify();
    }

    public boolean action(Event evt, Object obj) {
	if (evt.target instanceof MenuItem) {
	    String label = (String)obj;

	    if (label.equals("Quit")) {
		quitDialog.show();
	    } else if (label.equals("About ComponentTest")) {
		Rectangle bounds = bounds();
		Rectangle abounds = aboutBox.bounds();
		aboutBox.move(bounds.x + (bounds.width - abounds.width)/ 2,
			      bounds.y + (bounds.height - abounds.height)/2);
		aboutBox.show();
	    } else if (label.equals("Open...")) {
		openDialog.setDirectory("/tmp");
		openDialog.show();
		if (openDialog.getFile() != null) {
		    System.out.println("OPEN: " + openDialog.getFile());
		}
	    } else if (label.equals("Save...")) {
		saveDialog.show();
		if (saveDialog.getFile() != null) {
		    System.out.println("SAVE: " + saveDialog.getFile());
		}
	    }
	}
	return true;
    }

    public static void main(String args[]) {
	new ComponentPrintTest(new ComponentTest());
    }
}

class ComponentPrintTest extends Frame {
    Component comp;

    public ComponentPrintTest(Component comp) {
	this.comp = comp;
	reshape(400, 0, 400, 400);
	show();
    }

    public void paint(Graphics g) {
	Dimension d = size();
	g.setColor(Color.green);
	g.fillRect(0, 0, d.width, d.height);
	g.setColor(Color.black);
	comp.printAll(g);
    }
}

class ScrollPanel extends Panel {
    public ScrollPanel() {
	reshape(0, 0, 100, 100);
    }

    public void paint(Graphics g) {
	Rectangle r = bounds();

	g.drawRect(0, 0, r.width, r.height);
    }
}

class SillyWidget extends Canvas {
    Color	c1 = Color.pink;
    Color	c2 = Color.lightGray;
    long	entered = 0;
    long	inComponent = 0;
    Font	font = new Font("Courier", Font.BOLD, 12);
    FontMetrics fm;

    public SillyWidget() {
	reshape(0, 0, 100, 100);
    }

    public boolean mouseEnter(Event e, int x, int y) {
	Color c = c1;
	entered = e.when;
	c1 = c2;
	c2 = c;
	repaint();
	return true;
    }

    public boolean mouseExit(Event e, int x, int y) {
	Color c = c1;

	inComponent = e.when - entered;
	c1 = c2;
	c2 = c;
	repaint();
	return true;
    }

    public void paint(Graphics g) {
	Rectangle	r = bounds();

	g.setColor(c1);
	g.fillRoundRect(0, 0, r.width, r.height, 20, 20);
	g.setColor(c2);
	g.fillRoundRect(5, 5, r.width - 10, r.height - 10, 20, 20);
	g.setColor(c1);
	g.fillRoundRect(15, 15, r.width - 30, r.height - 30, 20, 20);
	g.setColor(Color.darkGray);
	g.setFont(font);
	fm = g.getFontMetrics(font);
	String s = ""+inComponent+" ms.";
	int width = fm.stringWidth(s);
	System.out.println("width = " + width);
	g.drawString(s,
		     (r.width - width) / 2,
		     (r.height - fm.getHeight()) / 2);
    }
}
	
class AboutBox extends Window {
    public AboutBox(Frame parent) {
	super(parent);
	setBackground(new Color(100,200, 200));
	add("Center", new Label("ComponentTest",Label.CENTER));
	Panel p = new Panel();
	add("South", p);
	p.add(new Button("ok"));
	reshape(0, 0, 200, 100);
    }
    public void paint(Graphics g) {
	Rectangle bounds = bounds();

	g.setColor(getBackground());
	g.draw3DRect(0, 0, bounds.width - 1, bounds.height - 1, true);
    }
    public Insets insets() {
	return new Insets(5, 5, 5, 5);
    }
    public boolean action(Event e, Object arg) {
	hide();
	return true;
    }
}

class QuitDialog extends Dialog {
    public QuitDialog(Frame parent) {
	super(parent, "Quit Application?", true);
	setLayout(new FlowLayout());
	add(new Button("Yes"));
	add(new Button("No"));
	reshape(30, 30, 200, 50);
	setResizable(false);
    }

    public synchronized void show() {
	Rectangle bounds = getParent().bounds();
	Rectangle abounds = bounds();
	move(bounds.x + (bounds.width - abounds.width)/ 2,
	     bounds.y + (bounds.height - abounds.height)/2);
	super.show();
    }

    public synchronized void wakeUp() {
	notify();
    }

    public boolean handleEvent(Event e) {
	if (e.id == Event.WINDOW_DESTROY) {
	    hide();
	    return true;
	} else {
	    return super.handleEvent(e);
	}
    }

    public boolean action(Event e, Object arg) {
	String label = (String)arg;

	if (label.equals("Yes")) {
	    System.exit(0);
	} else if (label.equals("No")) {
	    hide();
	}
	return true;
    }
}
