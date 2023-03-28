/*
 * @(#)WTextFieldPeer.java	1.9 95/09/19 Sami Shaio
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

public class WTextFieldPeer extends WTextComponentPeer implements TextFieldPeer {

    native void create(WComponentPeer parent);

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

    public WTextFieldPeer(TextField target) {
		super(target);
    }

	// TextFieldPeer interface
    public native void setEchoCharacter(char c);
    public Dimension preferredSize(int cols) {
		return minimumSize(cols);
    }
    public Dimension minimumSize(int cols) {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(fm.charWidth('0') * cols + 20,fm.getHeight() + 8);
    }

	// java public
    public Dimension minimumSize() {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(fm.stringWidth(((TextField)target).getText()) + 20, 
						fm.getHeight() + 8);
    }
    public void handleAction(long time, int msgData) {
		target.postEvent(new Event(target, Event.ACTION_EVENT, ((TextField)target).getText()));
    }


}
