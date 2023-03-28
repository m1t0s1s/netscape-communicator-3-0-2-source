/*
 * @(#)MMenuPeer.java	1.6 95/11/17 Sami Shaio
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

public class MMenuPeer extends MMenuItemPeer implements MenuPeer {
    native void createMenu(MenuBar parent);
    native void createSubMenu(Menu parent);

    public MMenuPeer(Menu target) {
	this.target = target;
	MenuContainer parent = target.getParent();

	if (parent instanceof MenuBar) {
	    createMenu((MenuBar)target.getParent());
	} else if (parent instanceof Menu) {
	    createSubMenu((Menu)target.getParent());
	} else {
	    throw new IllegalArgumentException("unknown menu container class");
	}
    }
    public void addSeparator() {
    }
    public void addItem(MenuItem item) {
    }
    public void delItem(int index) {
    }
    native void pEnable(boolean enable);
    public native void dispose();
}
