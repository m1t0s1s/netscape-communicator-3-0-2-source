/*
 * @(#)MChoicePeer.java	1.17 95/12/05 Sami Shaio
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

class MChoicePeer extends MComponentPeer implements ChoicePeer {
    native void create(MComponentPeer parent);
    native void pReshape(int x, int y, int width, int height);
    public native void select(int index);

    void initialize() {
	Choice opt = (Choice)target;
	int itemCount = opt.countItems();
	for (int i=0; i < itemCount; i++) {
	    addItem(opt.getItem(i), i);
	}
	select(opt.getSelectedIndex());
	super.initialize();
    }

    public MChoicePeer(Choice target) {
	super(target);
    }

    public Dimension minimumSize() {
       
	X11FontMetrics fm = (X11FontMetrics)getFontMetrics( target.getFont() );
	Choice c = (Choice)target;
	int width = 0;
        int height = 0;
        Dimension size = new Dimension();
        
	for (int i = c.countItems() ; i-- > 0 ;) {
            size = fm.stringExtent( c.getItem(i) );
	    width  = Math.max( size.width, width );
            height = Math.max( size.height, height );
	}

        size.width = width + 32;
        size.height = Math.max(height + 8, 15) + 5;
        return size;
    }

    public native void setFont(Font f);
    public native void addItem(String item, int index);
    public native void remove(int index);

    public native void setBackground(Color c);

    void action(int index) {
	Choice c = (Choice)target;
	c.select(index);
	target.postEvent(new Event(target, Event.ACTION_EVENT, c.getItem(index)));
    }
}


