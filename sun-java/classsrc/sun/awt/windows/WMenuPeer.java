/*
 * @(#)WMenuPeer.java	1.6 95/11/17 Sami Shaio
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

//
//
// All the job in the Java menu story is done in the peers constructors.
// No implementation is provided here for MenuPeer interface, neither 
// in WMenuBarPeer for MenuBarPeer. The assumption is addNotify be called at 
// the right time in the right place.
//
//
public class WMenuPeer extends WMenuItemPeer implements MenuPeer {

    // create according to the parent
    native void createMenu(MenuBar parent, String name, boolean enable);
    native void createSubMenu(Menu parent, String name, boolean enable);

    public WMenuPeer(Menu target) {
	    this.target = target;
	    MenuContainer parent = target.getParent();

	    if (parent instanceof MenuBar) {
	        createMenu((MenuBar)target.getParent(), target.getLabel(), target.isEnabled());
	    } 
        else if (parent instanceof Menu) {
	        createSubMenu((Menu)target.getParent(), target.getLabel(), target.isEnabled());
	    } 
        else {
	        throw new IllegalArgumentException("unknown menu container class");
	    }
    }

    // native methods
    public native void dispose();

    native void pEnable(boolean enable);
    native void pSetLabel(String label);

    //
    // MenuItemPeer interface
    public void enable() {
        pEnable(true);
    }
    public void disable() {
        pEnable(false);
    }
    public void setLabel(String label) {
        pSetLabel(label); // change a menu existing label
    }

    // MenuPeer interface 
    // do nothing since at this point everything has been arranged already,
    // if not we threw an exception sometimes in the past.
    public void addSeparator() {
    }
    public void addItem(MenuItem item) {
    }
    public void delItem(int index) {
    }

}
