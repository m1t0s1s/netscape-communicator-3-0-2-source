/*
 * @(#)MChoicePeer.java	1.10 95/12/03 Sami Shaio
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

class MChoicePeer extends MComponentPeer implements ChoicePeer {
    native void create(MComponentPeer parent);
    public native void select(int index);

    void initialize() {
	Choice opt = (Choice)target;
	int itemCount = opt.countItems();
	for (int i=0; i < itemCount; i++) {
	    addItem(opt.getItem(i), i);
	}
	if (itemCount > 0 && opt.getSelectedIndex() >= 0) {
	    select(opt.getSelectedIndex());
	}
	super.initialize();
    }

    public MChoicePeer(Choice target) {
	super(target);
    }

    public Dimension minimumSize() {
	FontMetrics fm = getFontMetrics(target.getFont());
	Choice c = (Choice)target;
	int w = 0;
	for (int i = c.countItems() ; i-- > 0 ;) {
	    w = Math.max(fm.stringWidth(c.getItem(i)), w);
	}
	return new Dimension(28 + w, Math.max(fm.getHeight() + 8, 15) + 3);
    }

    public native void addItem(String item, int index);
    public native void remove(int index);

    void action(int index) {
	Choice c = (Choice)target;
	c.select(index);
	target.postEvent(new Event(target, Event.ACTION_EVENT, c.getItem(index)));
    }
}

