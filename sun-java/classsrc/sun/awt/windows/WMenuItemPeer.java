/*
 * @(#)WMenuItemPeer.java	1.6 95/08/09 Sami Shaio
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

class WMenuItemPeer implements MenuItemPeer {
    int		    pNativeMenu;
    MenuItem	target;

    native void create(Menu parent, String label, boolean enable);

    //
    // assume a menuItem parent is defined so no check is performed.
    // This is not a very safe assumption but that's what we have now.
    //
    // Ideally we should create only if parent is valid.
    // We could, otherwise, let "create" happen on WMenuPeer.addItem
    // since at that point the parent is defined
    //
    WMenuItemPeer(MenuItem target) {
	    this.target = target;
	    create((Menu)target.getParent(), target.getLabel(), target.isEnabled());
    }

    // called from WCheckboxMenuItemPeer
    protected WMenuItemPeer() {
    }

    public void dispose() {
        pDispose((Menu)target.getParent());
    }
        

    // native implementation
    native void pEnable(Menu parent, boolean enable);
    native void pSetLabel(Menu parent, String label);
    public native void pDispose(Menu parent);

    //
    // MenuItemPeer interface
    public void enable() {
//        System.out.println("Enable on" + target.getLabel());
        pEnable((Menu)target.getParent(), true);
    }
    public void disable() {
//        System.out.println("Disable on" + target.getLabel());
        pEnable((Menu)target.getParent(), false);
    }
    public void setLabel(String label) {
        pSetLabel((Menu)target.getParent(), label); // change a menu existing label
    }

    // Win to Java event
    public void handleAction(long time, int msgData) {
//        System.out.println("Menu selected: " + target.getLabel());
	    target.postEvent(new Event(target, Event.ACTION_EVENT, target.getLabel()));
    }
}
