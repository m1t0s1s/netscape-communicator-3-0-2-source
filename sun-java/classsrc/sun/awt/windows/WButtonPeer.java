/*
 * @(#)WButtonPeer.java	1.5 95/08/07 Sami Shaio
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

import java.awt.*;
import java.awt.peer.*;

class WButtonPeer extends WComponentPeer implements ButtonPeer {
    native void create(WComponentPeer peer);
    public native void setLabel(String label);
    
    WButtonPeer(Button target) {
	    super(target);
    }

    void initialize() {
        super.initialize();
        setLabel( ((Button)target).getLabel() );
    }

    public Dimension minimumSize() {
	    FontMetrics fm = getFontMetrics(target.getFont());
	    return new Dimension(fm.stringWidth(((Button)target).getLabel()) + 14, 
			                 fm.getHeight() + 8);
    }

    protected void handleAction(long time, int msgData) {
	    target.postEvent(new Event(target, Event.ACTION_EVENT, ((Button)target).getLabel()));
    }
}
