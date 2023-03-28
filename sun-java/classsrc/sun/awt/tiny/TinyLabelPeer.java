/*
 * @(#)TinyLabelPeer.java	1.2 95/10/28 Arthur van Hoff
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
package sun.awt.tiny;

import java.awt.*;
import java.awt.peer.*;

class TinyLabelPeer extends TinyComponentPeer implements LabelPeer {
    /**
     * Create the label
     */
    TinyLabelPeer(Label target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
    }

    /**
     * Minimum size.
     */
    public Dimension minimumSize() {
	Label label = (Label)target;
	FontMetrics fm = getFontMetrics(label.getFont());
	return new Dimension(fm.stringWidth(label.getText()) + 10, fm.getHeight() + 10);
    }

    /**
     * Paint the label
     */
    public void paintComponent(Graphics g) {
	Label label = (Label)target;
	g.setColor(label.getBackground());
	g.fillRect(1, 1, width-2, height-2);

	g.setColor(label.getForeground());
	g.setFont(label.getFont());
	FontMetrics fm = g.getFontMetrics();
	String lbl = label.getText();

	switch (label.getAlignment()) {
	case Label.LEFT:
	    g.drawString(lbl, 2, (height + fm.getAscent()) / 2);
	    break;
	case Label.RIGHT:
	    g.drawString(lbl, width - (fm.stringWidth(lbl) + 2), 
			 (height + fm.getAscent()) / 2);
	    break;
	case Label.CENTER:
	    g.drawString(lbl, (width - fm.stringWidth(lbl)) / 2, 
			 (height + fm.getAscent()) / 2);
	    break;
	}
    }	

    public void setText(String text) {
	repaint();
    }
    public void setAlignment(int align) {
	repaint();
    }
}


