/*
 * @(#)WWindowPeer.java	1.6 95/11/29 Sami Shaio
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
package sun.awt.windows;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;

class WWindowPeer extends WContainerPeer implements WindowPeer {
    static Vector allWindows = new Vector();

    // native create function
    native void create(WComponentPeer parent);

    // WindowPeer interface
    public void toFront() {
    	show();
    }
    public native void toBack();

    // constructor
    WWindowPeer(Window target) {
    	super(target);

    	allWindows.addElement(this);

    	Font f = target.getFont();
    	if (f == null) {
    	    f = new Font("Dialog", Font.PLAIN, 12);
    	    target.setFont(f);
    	    setFont(f);
    	}

    	Color c = target.getBackground();
    	if (c == null) {
    	    target.setBackground(Color.lightGray);
    	    setBackground(Color.lightGray);
    	}

    	c = target.getForeground();
    	if (c == null) {
    	    target.setForeground(Color.black);
    	    setForeground(Color.black);
    	}
    }

    public void dispose() {
    	allWindows.removeElement(this);
    	super.dispose();
    }

}

