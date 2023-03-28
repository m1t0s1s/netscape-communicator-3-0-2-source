/*
 * @(#)MMenuItemPeer.java	1.8 95/11/23 Sami Shaio
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

class MMenuItemPeer implements MenuItemPeer {
    int		pData;
    boolean	isCheckbox = false;
    MenuItem	target;

    native void create(Menu parent);

    protected MMenuItemPeer() {
    }

    MMenuItemPeer(MenuItem target) {
       this.target = target;

       Font f = target.getFont();
       if (f == null) {
          f = new Font("Dialog", Font.PLAIN, 12);
          target.setFont(f);
       }
        
       create((Menu)target.getParent());
    }
    public native void enable();
    public native void disable();
    public native void dispose();
    public native void setLabel(String label);

    public void action(long when, int modifiers) {
	target.postEvent(new Event(target, when, Event.ACTION_EVENT, 0, 0, 0, modifiers, target.getLabel()));
    }
}
