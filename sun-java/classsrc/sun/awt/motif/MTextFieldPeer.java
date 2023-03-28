/*
 * @(#)MTextFieldPeer.java	1.10 95/11/30 Sami Shaio
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

public class MTextFieldPeer extends MComponentPeer implements TextFieldPeer {
    native void create(MComponentPeer parent);

    void initialize() {
	TextField txt = (TextField)target;

	setText(txt.getText());
	if (txt.echoCharIsSet()) {
	    setEchoCharacter(txt.getEchoChar());
	}
	select(txt.getSelectionStart(), txt.getSelectionEnd());
	setEditable(txt.isEditable());
	super.initialize();
    }

    public MTextFieldPeer(TextField target) {
	super(target);
    }

    public void setEditable(boolean editable) {
	pSetEditable(editable);
	setBackground(target.getBackground());
    }
    public void setBackground(Color c) {
	TextField t = (TextField)target;
	if (t.isEditable()) {
	    c = c.brighter();
	}
	super.setBackground(c);
    }
    public native void pSetEditable(boolean editable);
    public native void select(int selStart, int selEnd);
    public native int getSelectionStart();
    public native int getSelectionEnd();
    public native void setText(String l);
    public native void dispose();
    public native String getText();
    public native void setEchoCharacter(char c);

    public Dimension minimumSize() {
	X11FontMetrics fm =  (X11FontMetrics)getFontMetrics(target.getFont());
        Dimension extent = fm.stringExtent( ((TextField)target).getText() );
        extent.width += 20;
        extent.height += 15;
        return extent;
    }
    public Dimension preferredSize(int cols) {
	return minimumSize(cols);
    }
    public Dimension minimumSize(int cols) {
	X11FontMetrics fm = (X11FontMetrics)getFontMetrics(target.getFont());
        String text = ((TextField)target).getText();
        int length = text.length();
        String prefix = text.substring( 0, Math.min(cols, length) );
        Dimension size = fm.stringExtent( prefix );
        
        if( length < cols )
        {
           size.width += fm.charWidth(' ') * (cols - length);
        }

        size.width += 20;
        size.height += 15;
        return size;
    }

    public void action() {
	target.postEvent(new Event(target, Event.ACTION_EVENT, ((TextField)target).getText()));
    }
}
