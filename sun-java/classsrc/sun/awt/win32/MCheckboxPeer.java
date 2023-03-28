/*
 * @(#)MCheckboxPeer.java	1.5 95/08/07 Sami Shaio
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

public class MCheckboxPeer extends MComponentPeer implements CheckboxPeer {
    native void create(MComponentPeer parent);
    public native void setLabel(String label);

    void initialize() {
	Checkbox t = (Checkbox)target;

	setState(t.getState());
	setCheckboxGroup(t.getCheckboxGroup());
	super.initialize();
    }

    public MCheckboxPeer(Checkbox target) {
	super(target);
    }

    public native void setState(boolean state);
    public native void setCheckboxGroup(CheckboxGroup g);

    public Dimension minimumSize() {
	String lbl = ((Checkbox)target).getLabel();
	if (lbl != null) {
	    FontMetrics fm = getFontMetrics(target.getFont());
	    return new Dimension(30 + fm.stringWidth(lbl), 
				 Math.max(fm.getHeight() + 8, 15));
	}
	return new Dimension(20, 15);
    }

    void action(boolean state) {
	Checkbox target = (Checkbox)this.target;
	target.setState(state);
	target.postEvent(new Event(target, Event.ACTION_EVENT, new Boolean(state)));
    }
}
