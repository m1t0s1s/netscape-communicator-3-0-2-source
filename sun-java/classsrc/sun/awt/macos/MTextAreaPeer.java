/*
 * @(#)MTextAreaPeer.java	1.8 95/08/09 Sami Shaio
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

package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

public class MTextAreaPeer extends MComponentPeer implements TextAreaPeer {

	Boolean		mEditable;

	public MTextAreaPeer(TextArea target) {
		super(target);
	}

	native void create(MComponentPeer parent);

    void initialize() {
    
		TextArea txt = (TextArea)target;
		String	text;
		
		if ((text = txt.getText()) != null) {
		    setText(text);
		}
		select(txt.getSelectionStart(), txt.getSelectionEnd());
		setEditable(txt.isEditable());
		
		super.initialize();
    
    }

	public void setBackground(Color c) {
		TextArea t = (TextArea)target;
		if (t.isEditable()) {
		    c = c.brighter();
		}
		super.setBackground(c);
    }

    public native void setEditable(boolean e);
    public native void select(int selStart, int selEnd);
    public native int getSelectionStart();
    public native int getSelectionEnd();
    public native void setText(String txt);
    public native String getText();
    public native void insertText(String txt, int pos);
    public native void replaceText(String txt, int start, int end);
	public native void reshape(int x, int y, int width, int height);

    public Dimension minimumSize() {
		return minimumSize(10, 60);
    }
    
    public Dimension preferredSize(int rows, int cols) {
		return minimumSize(rows, cols);
    }
	
	public Dimension minimumSize(int rows, int cols) {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(fm.charWidth('0') * cols + 20, fm.getHeight() * rows + 20);
    }
    

}
