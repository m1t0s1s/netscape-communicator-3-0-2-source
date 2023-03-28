/*
 * @(#)MScrollbarPeer.java	1.10 95/11/14 Sami Shaio
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
package sun.awt.motif;

import java.awt.*;
import java.awt.peer.*;

class MScrollbarPeer extends MComponentPeer implements ScrollbarPeer {
    native void create(MComponentPeer parent);
    
    MScrollbarPeer(Scrollbar target) {
	super(target);
    }

    public native void setValue(int value);    
    public native void setValues(int value, int visible, int minimum, int maximum);    
    public native void setLineIncrement(int l);
    public native void setPageIncrement(int l);

    public Dimension minimumSize() {
	if (((Scrollbar)target).getOrientation() == Scrollbar.VERTICAL) {
	    return new Dimension(18, 50);
	} else {
	    return new Dimension(50, 18);
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
}
