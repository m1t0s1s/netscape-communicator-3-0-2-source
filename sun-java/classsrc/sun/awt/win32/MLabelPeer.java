/*
 * @(#)MLabelPeer.java	1.7 95/09/05 Sami Shaio
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

class MLabelPeer extends MComponentPeer implements LabelPeer {
    native void create(MComponentPeer parent);
    //public native void setFont(Font f);

    public void initialize() {
	Label	l = (Label)target;
	String  txt;
	int	align;

	if ((txt = l.getText()) != null) {
	    setText(l.getText());
	}
	if ((align = l.getAlignment()) != Label.LEFT) {
	    setAlignment(align);
	}
	super.initialize();
    }

    MLabelPeer(Label target) {
	super(target);
    }

    public Dimension minimumSize() {
	FontMetrics fm = getFontMetrics(target.getFont());
	String label = ((Label)target).getText();
	if (label == null) label = "";
	return new Dimension(fm.stringWidth(label) + 14, 
			     fm.getHeight() + 8);
    }

    public native void setText(String label);
    public native void setAlignment(int alignment);
}
