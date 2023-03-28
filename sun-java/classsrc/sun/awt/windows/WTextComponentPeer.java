/*
 * @(#)WTextComponentPeer.java	1.9 95/09/19 Sami Shaio
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

public abstract class WTextComponentPeer extends WComponentPeer implements TextComponentPeer {

	// pass thru constructor
	public WTextComponentPeer(Component target) {
		super(target);
	}

	// TextComponentPeer interface 
    public void setEditable(boolean editable) {
		widget_setEditable(editable);
		setBackground(target.getBackground());
    }

    public native String getText();
    public native void setText(String l);
    public native int getSelectionStart();
    public native int getSelectionEnd();
    public native void select(int selStart, int selEnd);

	// native public
    public native void widget_setEditable(boolean e);

	// java public
	public void setBackground(Color c) {
		TextComponent t = (TextComponent)target;
		if (t.isEditable()) {
			c = c.brighter();
		}
		super.setBackground(c);
    }

}

