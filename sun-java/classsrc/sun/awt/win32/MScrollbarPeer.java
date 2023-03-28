/*
 * @(#)MScrollbarPeer.java	1.7 95/09/22 Sami Shaio
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
package sun.awt.win32;

import java.awt.*;
import java.awt.peer.*;

class MScrollbarPeer extends MComponentPeer implements ScrollbarPeer {
    native void pCreate(MComponentPeer parent);
    public native void setValue(int value);    
    public native void setValues(int value, int visible, int minimum, int maximum);    
    
    MScrollbarPeer(Scrollbar target) {
	super(target);
    }

    void create(MComponentPeer parent) {
	Scrollbar sb = (Scrollbar)target;
        pCreate(parent);
        setValues(sb.getValue(), sb.getVisible(), 
                  sb.getMinimum(), sb.getMaximum());
    }

    public Dimension minimumSize() {
	if (((Scrollbar)target).getOrientation() == Scrollbar.VERTICAL) {
	    return new Dimension(15, 50);
	} else {
	    return new Dimension(50, 15);
	}
    }

    public void lineUp(int value) {
	Scrollbar sb = (Scrollbar)target;
	sb.setValue(value);
	target.postEvent(new Event(target, Event.SCROLL_LINE_UP, new Integer(value)));
    }

    public void lineDown(int value) {
	Scrollbar sb = (Scrollbar)target;
	sb.setValue(value);
	target.postEvent(new Event(target, Event.SCROLL_LINE_DOWN, new Integer(value)));
    }

    public void pageUp(int value) {
	Scrollbar sb = (Scrollbar)target;
	sb.setValue(value);
	target.postEvent(new Event(target, Event.SCROLL_PAGE_UP, new Integer(value)));
    }

    public void pageDown(int value) {
	Scrollbar sb = (Scrollbar)target;
	sb.setValue(value);
	target.postEvent(new Event(target, Event.SCROLL_PAGE_DOWN, new Integer(value)));
    }

    public void dragAbsolute(int value) {
	Scrollbar sb = (Scrollbar)target;
	sb.setValue(value);
	target.postEvent(new Event(target, Event.SCROLL_ABSOLUTE, new Integer(value)));
    }

    //XXX: NOT IMPLEMENTED
    public void setLineIncrement(int l) {
    }
    //XXX: NOT IMPLEMENTED
    public void setPageIncrement(int l) {
    }
}
