/*
 * @(#)TinyScrollbarPeer.java	1.4 95/11/09 Arthur van Hoff
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
import java.awt.peer.*;

class TinyScrollbarPeer extends TinyComponentPeer implements ScrollbarPeer, TinyScrollbarClient {
    TinyVerticalScrollbar tsb;
    
    /**
     * Create a scrollbar.
     */
    TinyScrollbarPeer(Scrollbar target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);

	if (target.getOrientation() == Scrollbar.VERTICAL) {
	    tsb = new TinyVerticalScrollbar();
	} else {
	    tsb = new TinyHorizontalScrollbar();
	}
	tsb.sb = this;
	tsb.min = target.getMinimum();
	tsb.max = target.getMaximum();
	tsb.vis = target.getVisible();
	tsb.val = target.getValue();
	tsb.line = target.getLineIncrement();
	tsb.page = target.getPageIncrement();
    }

    /**
     * Compute the minimum size for the scrollbar.
     */
    public Dimension minimumSize() {
	Scrollbar sb = (Scrollbar)target;
	return (sb.getOrientation() == Scrollbar.VERTICAL) ? 
	    new Dimension(15, 50) : new Dimension(50, 15);
    }

    /**
     * Paint the scrollbar.
     */
    public void paintComponent(Graphics g) {
	Scrollbar sb = (Scrollbar)target;
	g.setColor(sb.getBackground());
	g.fillRect(1, 1, width-2, height-2);
	tsb.paint(g, sb.getBackground(), width, height);
    }

    /**
     * The value has changed.
     */
    public void notifyValue(Object obj, int id, int val) {
	Scrollbar sb = (Scrollbar)target;
	sb.setValue(val);
	targetEvent(id, new Integer(val));
    }

    /**
     * Handle an event.
     */
    public boolean handleWindowEvent(Event evt) {
	Scrollbar sb = (Scrollbar)target;
	return tsb.handleWindowEvent(evt, width, height);
    }

    public void setValue(int value) {
	tsb.val = value;
	repaint();
    }
    public void setValues(int value, int visible, int minimum, int maximum) {
	tsb.min = minimum;
	tsb.max = maximum;
	tsb.vis = visible;
	tsb.val = value;
	repaint();
    }
    public void setLineIncrement(int l) {
	tsb.line = l;
    }
    public void setPageIncrement(int p) {
	tsb.page = p;
    }
}
