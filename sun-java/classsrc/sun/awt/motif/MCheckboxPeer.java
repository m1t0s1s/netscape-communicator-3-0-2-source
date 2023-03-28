/*
 * @(#)MCheckboxPeer.java	1.7 95/08/06 Sami Shaio
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
	String label = ((Checkbox)target).getLabel();
        Dimension size;
        
	if (label != null) {
	    X11FontMetrics fm = ( X11FontMetrics )getFontMetrics(target.getFont());
            size = fm.stringExtent( label );
            size.width += 30;
            size.height = Math.max( (size.height + 8), 15 );
	}
        else {
           size = new Dimension(20, 15);
        }
        
	return size;
    }

    void action(boolean state) {
	Checkbox target = (Checkbox)this.target;
	target.setState(state);
	target.postEvent(new Event(target, Event.ACTION_EVENT, new Boolean(state)));
    }
}
