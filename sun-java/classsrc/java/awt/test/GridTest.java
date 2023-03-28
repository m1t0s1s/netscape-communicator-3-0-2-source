/*
 * @(#)GridTest.java	1.1 95/07/30 Arthur van Hoff
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

class GridPanel extends Panel {
    GridPanel(int rows, int cols) {
	setLayout(new GridLayout(rows, cols, 5, 5));
	add(new Button("een"));
	add(new Button("twee"));
	add(new Button("drie"));
	add(new Button("vier"));
	add(new Button("vijf"));
	add(new Button("zes"));
	add(new Button("zeven"));
	add(new Button("acht"));
	add(new Button("negen"));
	add(new Button("tien"));
	add(new Button("elf"));
    }
}

class GridTest extends Frame {
    GridTest() {
	setFont(new Font("Dialog", Font.PLAIN, 18));
	setLayout(new BorderLayout());
	Panel p = new GridPanel(3, 4);
	add("Center", p);
	p.add(new GridPanel(4, 3));
	pack();
	show();
    }

    public static void main(String argv[]) {
	new GridTest();
    }
}
