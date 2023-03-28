/*
 * @(#)MTextFieldPeer.java	1.9 95/09/19 Sami Shaio
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
	FontMetrics fm = getFontMetrics(target.getFont());
	return new Dimension(fm.stringWidth(((TextField)target).getText()) + 20, 
				    fm.getHeight() + 8);
    }
    public Dimension preferredSize(int cols) {
	return minimumSize(cols);
    }
    public Dimension minimumSize(int cols) {
	FontMetrics fm = getFontMetrics(target.getFont());
	return new Dimension(fm.charWidth('0') * cols + 20,fm.getHeight() + 8);
    }

    public void action() {
	target.postEvent(new Event(target, Event.ACTION_EVENT, ((TextField)target).getText()));
    }
}
