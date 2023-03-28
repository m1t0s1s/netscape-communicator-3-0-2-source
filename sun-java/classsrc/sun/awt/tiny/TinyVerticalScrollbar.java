/*
 * @(#)TinyVerticalScrollbar.java	1.3 95/11/09 Arthur van Hoff
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
 * A simple vertical scroll bar.
 */
class TinyVerticalScrollbar {
    TinyScrollbarClient sb;
    int val;
    int min;
    int max;
    int vis;
    int line;
    int page;
    int mode;
    int mv;

    void notifyValue(int v) {
	if (v < min) {
	    v = min;
	} else if (v > max) {
	    v = max;
	}
	if ((v != val) && (sb != null)) {
	    sb.notifyValue(this, mode, v);
	}
    }

    void drawLine(Graphics g, int x1, int y1, int x2, int y2) {
	g.drawLine(x1, y1, x2, y2);
    }

    void paint(Graphics g, Color back, int width, int height) {

	double f = (double)(height - 2*(width-1)) / ((max - min) + vis);
	int v1 = width + (int)(f * (val - min));
	int v2 = (int)(f * vis);
	int w2 = width-2;

	g.setColor(back.brighter());
	drawLine(g, width-1, 0, width-1, height);
	drawLine(g, 0, height-1, width, height-1);
	drawLine(g, width/2, 1, 1, w2);
	drawLine(g, 1, height-w2, w2, height-w2);
	drawLine(g, 1, height-w2, width/2, height-2);
	drawLine(g, 1, v1, width-2, v1);
	drawLine(g, 1, v1, 1, v1 + v2-1);

	g.setColor(back.darker());
	drawLine(g, 0, 0, width, 0);
	drawLine(g, 0, 0, 0, height);
	drawLine(g, 1, w2, w2, w2);
	drawLine(g, w2, w2, width/2, 1);
	drawLine(g, width/2, height-2, w2, height-w2);
	drawLine(g, width-2, v1, width-2, v1 + v2 - 1);
	drawLine(g, 1, v1 + v2 - 1, width-2, v1 + v2 - 1);
    }

    boolean handleWindowEvent(Event evt, int width, int height) {
	switch (evt.id) {
	case Event.MOUSE_DOWN:
	    if (evt.y < width-1) {
		mode = Event.SCROLL_LINE_UP;
		notifyValue(val - line);
		return true;
	    }
	    if (evt.y > height - (width-1)) {
		mode = Event.SCROLL_LINE_DOWN;
		notifyValue(val + line);
		return true;
	    }

	    double f = (double)(height - 2*(width-1)) / ((max - min) + vis);
	    int v1 = width + (int)(f * (val - min));
	    int v2 = (int)(f * vis);

	    if (evt.y < v1) {
		mode = Event.SCROLL_PAGE_UP;
		notifyValue(val - page);
		return true;
	    }
	    if (evt.y > v1 + v2) {
		mode = Event.SCROLL_PAGE_DOWN;
		notifyValue(val + page);
		return true;
	    }
	    mode = Event.SCROLL_ABSOLUTE;
	    mv = evt.y - v1;
	    return true;

	case Event.MOUSE_DRAG:
	    if (mode == Event.SCROLL_ABSOLUTE) {
		notifyValue((((evt.y - (mv + width)) * ((max - min) + vis) / (height - 2*width))) + min);
	    }
	    return true;

	case Event.MOUSE_UP:
	    return true;
	}

	return false;
    }
}
