/*
 * @(#)TinyHorizontalScrollbar.java	1.2 95/10/30 Arthur van Hoff
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
package sun.awt.tiny;

import java.awt.*;

/**
 * A simple horizontal scroll bar. The scrollbar is made horizontal
 * by taking a vertical scrollbar and swapping the x and y coordinates.
 */
class TinyHorizontalScrollbar extends TinyVerticalScrollbar {
    void drawLine(Graphics g, int x1, int y1, int x2, int y2) {
	g.drawLine(y1, x1, y2, x2);
    }

    void paint(Graphics g, Color back, int width, int height) {
	super.paint(g, back, height, width);
    }

    boolean handleWindowEvent(Event evt, int width, int height) {
	evt.x ^= evt.y;
	evt.y ^= evt.x;
	evt.x ^= evt.y;
	return super.handleWindowEvent(evt, height, width);
    }
}
