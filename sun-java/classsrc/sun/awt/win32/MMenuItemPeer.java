/*
 * @(#)MMenuItemPeer.java	1.6 95/08/09 Sami Shaio
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

class MMenuItemPeer implements MenuItemPeer {
    int		pData;
    boolean	isCheckbox = false;
    MenuItem	target;

    native void create(Menu parent);
    native void pEnable(boolean enable);
    public native void dispose();

    protected MMenuItemPeer() {
    }

    MMenuItemPeer(MenuItem target) {
	this.target = target;
	create((Menu)target.getParent());
    }
    public void enable() {
        pEnable(true);
    }
    public void disable() {
        pEnable(false);
    }
    public void setLabel(String label) {
    }

    public void action() {
	target.postEvent(new Event(target, Event.ACTION_EVENT, target.getLabel()));
    }
}
