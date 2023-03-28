/*
 * @(#)WMenuBarPeer.java	1.5 95/08/07 Sami Shaio
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
// No implementation is provided here for MenuBarPeer interface, neither 
// in WMenuPeer for MenuPeer. The assumption is addNotify be called at 
// the right time in the right place.
//
//
public class WMenuBarPeer implements MenuBarPeer {
    int		pNativeMenu;
    MenuBar	target;

    native void create(Frame frame);
    
    public WMenuBarPeer(MenuBar target) {
	    this.target = target;
	    create((Frame)target.getParent());
    }

    public native void dispose();

    // MenuBarPeer interface
    // do nothing since at this point everything has been setted already,
    // if not we threw an exception sometimes in the past.
    public void addMenu(Menu m) {
    }
    public void delMenu(int index) {
    }
    public void addHelpMenu(Menu m) {
    }
}
