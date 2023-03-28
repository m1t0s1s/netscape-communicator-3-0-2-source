/*
 * @(#)AddRemoveTest.java	1.2 95/09/19 Herb Jellinek
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
import java.applet.*;


/**
 * An interactive test of Container.add() and Container.remove().
 *
 * @version 1.2, 19 Sep 1995
 * @author Herb Jellinek
 */

public class AddRemoveTest extends Applet {
    /* set atTop to true to start with floatingButtons at top; false puts
     * 'em on bottom.
     */
    boolean atTop = false;

    Panel topHolder;
    Panel bottomHolder;

    Panel floatingButtons;

    Button flipper;
    Button dumper;
    
    public void init() {
	floatingButtons = new Panel();
	floatingButtons.setLayout(new FlowLayout());
	floatingButtons.add(new Button("one"));
	floatingButtons.add(new Button("two"));
	floatingButtons.add(new Button("three"));
	floatingButtons.add(new Button("four"));
	floatingButtons.add(new Button("five"));	
	
	setLayout(new BorderLayout());
	add("North", topHolder = new Panel());
	Panel center = new Panel();
	add("Center", center);
	center.setLayout(new GridLayout(0, 1));
	center.add(new Label("Center", Label.CENTER));
	center.add(flipper = new Button("Flip!"));
	center.add(dumper = new Button("Show!"));
	center.add(new Label("Panel", Label.CENTER));
	add("South", bottomHolder = new Panel());

	if (atTop) {
	    topHolder.add(floatingButtons);
	} else {
	    bottomHolder.add(floatingButtons);
	}
    }
    
    public void start() {
	flipper.enable();
    }

    public void stop() {
	flipper.disable();
    }

    public boolean handleEvent(Event e) {
	Object target = e.target;
	
	if (e.id == Event.WINDOW_DESTROY) {
	    System.exit(0);
	    return true;
	} else if (target == flipper) {
	    if (atTop) {
		topHolder.remove(floatingButtons);
		bottomHolder.add(floatingButtons);
		invalidate();
		validate();
	    } else {
		bottomHolder.remove(floatingButtons);
		topHolder.add(floatingButtons);
		invalidate();
		validate();
	    }
	    atTop = !atTop;
	    return true;
	} else if (target == dumper) {
	    dump(this+"");
	    return true;
	}
	return false;
    }

    void dump(String label) {
	System.out.println("---------- "+label);
	list();
	System.out.println();
	System.out.println();
    }
	

    public static void main(String args[]) {
	Frame f = new Frame("AddRemoveTest");
	AddRemoveTest addRemoveTest = new AddRemoveTest();

	addRemoveTest.init();
	addRemoveTest.start();

	f.add("Center", addRemoveTest);
	f.resize(300, 300);
	f.show();
    }
}

