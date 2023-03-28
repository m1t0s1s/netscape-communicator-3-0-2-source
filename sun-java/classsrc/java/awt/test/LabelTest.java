/*
 * %W% %E% Arthur van Hoff
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
public class LabelTest extends Frame {
    public LabelTest() {
	super("LabelTest");
	Font f1 = new Font("Helvetica", Font.ITALIC, 18);
	Font f2 = new Font("Helvetica", Font.BOLD, 24);
	Color c1 = new Color(200, 50, 100);
	Color c2 = new Color(50, 100, 200);
	Label lbl;

	setLayout(new GridLayout(0, 3));
	add(lbl = new Label("Left", Label.LEFT));
	add(lbl = new Label("Left", Label.LEFT));
	lbl.setFont(f1);
	lbl.setForeground(c1);
	add(lbl = new Label("Left", Label.LEFT));
	lbl.setFont(f2);
	lbl.setForeground(c2);
	add(lbl = new Label("Center", Label.CENTER));
	add(lbl = new Label("Center", Label.CENTER));
	lbl.setFont(f1);
	lbl.setForeground(c1);
	add(lbl = new Label("Center", Label.CENTER));
	lbl.setFont(f2);
	lbl.setForeground(c2);
	add(lbl = new Label("Right", Label.RIGHT));
	add(lbl = new Label("Right", Label.RIGHT));
	lbl.setFont(f1);
	lbl.setForeground(c1);
	add(lbl = new Label("Right", Label.RIGHT));
	lbl.setFont(f2);
	lbl.setForeground(c2);
	move(200, 100);
	pack();
	show();
    }

    public static void main(String args[]) {
	new LabelTest();
    }
}
