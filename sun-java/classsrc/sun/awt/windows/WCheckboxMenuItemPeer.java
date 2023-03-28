/*
 * @(#)WCheckboxMenuItemPeer.java	1.5 95/08/07 Sami Shaio
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

class WCheckboxMenuItemPeer extends WMenuItemPeer implements CheckboxMenuItemPeer {

    native void create(Menu parent, String label, boolean enable, boolean state);

    WCheckboxMenuItemPeer(CheckboxMenuItem target) {
	    this.target = target;
	    create((Menu)target.getParent(), target.getLabel(), target.isEnabled(), target.getState());
    }

    native boolean getState(Menu owner);

    // CheckboxMenuItemPeer interface
    public void setState(boolean t) {
//        System.out.println("SetState called. Value: " + t);
        pSetState((Menu)target.getParent(), t);
    }

    public native void pSetState(Menu owner, boolean state);

    // WinToJava event
    public void handleAction(long time, int msgData) {
	    CheckboxMenuItem target = (CheckboxMenuItem)this.target;
	    target.setState(!target.getState());
	    super.handleAction(time, msgData);
    }
}

	
