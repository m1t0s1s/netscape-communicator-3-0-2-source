/*
 * @(#)WTextAreaPeer.java	1.8 95/08/09 Sami Shaio
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

public class WTextAreaPeer extends WTextComponentPeer implements TextAreaPeer 
{
    native void create(WComponentPeer parent);

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

    public WTextAreaPeer(TextArea target) {
	    super(target);
    }

    // TextAreaPeer interface
    public native void insertText(String txt, int pos);
    public native void replaceText(String txt, int start, int end);

    public Dimension preferredSize(int rows, int cols) {
	    return minimumSize(rows, cols);
    }
    public Dimension minimumSize(int rows, int cols) {
	    FontMetrics fm = getFontMetrics(target.getFont());
	    return new Dimension(fm.charWidth('0') * cols + 20, fm.getHeight() * rows + 20);
    }

    // java public
    public Dimension minimumSize() {
	    return minimumSize(10, 60);
    }
}
