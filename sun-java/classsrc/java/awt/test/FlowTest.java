/*
 * @(#)FlowTest.java	1.8 95/07/30 Arthur van Hoff
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

class FlowPanel extends Panel {
    FlowPanel(int align) {
	setLayout(new FlowLayout(align));
	add(new Button("een"));
	add(new Button("twee"));
	add(new Button("drie"));
	add(new Button("vier"));
	add(new Button("vijf"));
	add(new Button("zes"));
	add(new Button("zeven")).hide();
	add(new Button("acht"));
	add(new Button("negen"));
	add(new Button("tien"));
	add(new Button("elf"));
    }
}

class FlowTest extends Frame {
    FlowTest() {
	setFont(new Font("Dialog", Font.PLAIN, 18));
	setLayout(new BorderLayout());
	add("North", new FlowPanel(FlowLayout.LEFT)).setBackground(new Color(255, 200, 150));
	add("Center", new FlowPanel(FlowLayout.CENTER)).setBackground(new Color(200, 255, 150));
	add("South", new FlowPanel(FlowLayout.RIGHT)).setBackground(new Color(200, 100, 255));
	pack();
	show();
    }

    public static void main(String argv[]) {
	new FlowTest();
    }
}
