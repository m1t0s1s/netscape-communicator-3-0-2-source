/*
 * @(#)MCheckboxMenuItemPeer.java	1.5 95/08/07 Sami Shaio
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

class MCheckboxMenuItemPeer extends MMenuItemPeer implements CheckboxMenuItemPeer {
    MCheckboxMenuItemPeer(CheckboxMenuItem target) {
	this.target = target;
	isCheckbox = true;
	create((Menu)target.getParent());
    }

    public native void setState(boolean t);

    public void action(boolean state) {
	CheckboxMenuItem target = (CheckboxMenuItem)this.target;
	target.setState(state);
	super.action();
    }
}

	
