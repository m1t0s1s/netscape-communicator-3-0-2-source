/*
 * @(#)TinyTextFieldPeer.java	1.4 95/11/28 
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

class TinyTextFieldPeer extends TinyComponentPeer implements TextFieldPeer {
    
    public final static int BORDER = 2;
    public final static int MARGIN = 4;


    boolean hasFocus;
    String text;
    int selStart;
    int selEnd;
    char echoChar;
    int mpos;
    int moved;
    long mtm;

    /**
     * Create the text field.
     */
    TinyTextFieldPeer(TextField target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
	text = target.getText();
        selStart = target.getSelectionStart();
        selEnd = target.getSelectionEnd();
	echoChar = target.getEchoChar();
    }

    /**
     * Minumum size.
     */
    public Dimension minimumSize() {
	TextField txt = (TextField)target;
	FontMetrics fm = getFontMetrics(txt.getFont());
	return new Dimension(fm.stringWidth(text) + 10, fm.getHeight() + 10);
    }
    public Dimension preferredSize(int cols) {
	return minimumSize(cols);
    }
    public Dimension minimumSize(int cols) {
	FontMetrics fm = getFontMetrics(target.getFont());
	return new Dimension(fm.charWidth('0') * cols + 10,fm.getHeight() + 10);
    }

    /**
     * Position to x
     */
    int pos2x(int pos) {
	FontMetrics fm = getFontMetrics(target.getFont());
	int x = MARGIN, widths[] = fm.getWidths();
	for (int i = 0 ; i < pos ; i++) {
	    x += widths[text.charAt(i)];
	}
	return x;
    }

    /**
     * x 2 position
     */
    int xy2pos(int x, int y) {
	if (y < 0) {
	    return 0;
	}
	if (y > height) {
	    return text.length();
	}
	FontMetrics fm = getFontMetrics(target.getFont());
	int widths[] = fm.getWidths();
	int len = text.length();
	x -= MARGIN;
	for (int i = 0 ; i < len ; i++) {
	    int w = widths[text.charAt(i)];
	    if (x < w/2) {
		return i;
	    }
	    x -= w;
	}
	return text.length();
    }

    /**
     * Paint the text field.
     */
    public void paintComponent(Graphics g) {
	TextField txt = (TextField)target;
	g.setFont(txt.getFont());
	int w = width - (2 * BORDER); 
	
	g.setColor(txt.isEditable() ? target.getBackground().brighter() : target.getBackground());
	g.fillRect(BORDER, BORDER, w, height - (2 * BORDER));

	if (hasFocus) {
	    g.setColor(Color.black);
	    g.drawRect(0, 0, width-1, height-1);
	    g.drawRect(1, 1, width-3, height-3);
	} else {
	    g.setColor(target.getBackground());
	    g.drawRect(0, 0, width-1, height-1);
	    g.draw3DRect(1, 1, width-3, height-3, false);
	}
	
	if (text != null) {
	   g.clipRect(BORDER, MARGIN, w, height - (2 * MARGIN));
	   paintLine(g);
	}
    }

    /** 
     * Paint a range in the textField given by the values s and e
     */
    void paintRange(Graphics g, int s, int e) {
	TextField txt = (TextField)target;
	int xs = pos2x(s) - moved;
	int xe = pos2x(e) - moved;
	Graphics ng = g.create();
	int w = (s == e) ? 1 : xe - xs;
	ng.clipRect(xs, MARGIN, w, height - (2 * MARGIN));
	paintLine(ng);
    	ng.dispose();
    }

    /**
     * draw the text to the textField
     */
    void paintLine(Graphics g) {
	FontMetrics fm = g.getFontMetrics();
		
	int w = width - BORDER; 
	int h = height - (2 * MARGIN);
	int xs = pos2x(selStart) - moved;
	int xe = pos2x(selEnd) - moved;

	if ((xs < MARGIN) && (xe > w)) {
	   g.setColor(selectColor());
           g.fillRect(BORDER, MARGIN, w - BORDER, h);
	} else {
	   g.setColor(backGroundColor());
           g.fillRect(BORDER, MARGIN, w - BORDER, h);
	   
	   if ((xs >= MARGIN) && (xs <= w)) {
		g.setColor(selectColor());	
		
		if (xe > w) {
		    g.fillRect(xs, MARGIN, w - xs, h);
		} else if (xs == xe) {
		    g.fillRect(xs, MARGIN, 1, h);
		} else {
		    g.fillRect(xs, MARGIN, xe - xs, h);
		}
	   } else if ((xe >= MARGIN) && (xe <= w)) {
	       g.setColor(selectColor());
	       g.fillRect(BORDER, MARGIN, xe - BORDER, h);
	   }
	}
	g.setColor(target.getForeground());  
	int x = MARGIN - moved;
	if (echoChar == 0) {
	    g.drawString(text, x, (height + fm.getAscent()) / 2);
	} else {
	    char data[] = new char[text.length()];
	    for (int i = 0 ; i < data.length ; i++) {
		data[i] = echoChar;
	    }
	    g.drawChars(data, 0, data.length, x, (height + fm.getAscent()) / 2);
	}
    }    
    

    /**
     * return the background color
     */
    Color backGroundColor() {
	TextField field = (TextField)target;
	return (field.isEditable() ? target.getBackground().brighter() : target.getBackground()); 
    }

    /**
     * return the selection color
     */
    Color selectColor() {
	TextField field = (TextField)target;
	return (field.isEditable() ? target.getBackground() : target.getBackground().darker());
    }



    /**
     * Handle events.
     */
    public boolean handleWindowEvent(Event evt) {
	TextField field = (TextField)target;
	switch (evt.id) {
	case Event.MOUSE_DOWN:
	    if (!hasFocus) {
		requestFocus();
	    }
	    mpos = xy2pos(evt.x + moved, evt.y);
	    select(mpos, mpos);
	    return true;

	case Event.MOUSE_UP:
	    if (evt.when - mtm < 150) {
		select(0, text.length());
	    }
	    mtm = evt.when;
	    return true;

	case Event.MOUSE_DRAG:
	    int mpos2 = xy2pos(evt.x + moved, evt.y);
	    if (mpos < mpos2) {
		select(mpos, mpos2);
	    } else {
		select(mpos2, mpos);
	    }
	    scrollHorizontal(mpos2);
	    return true;

	case Event.GOT_FOCUS:
	    hasFocus = true;
	    repaint();
	    return true;
	case Event.LOST_FOCUS:
	    hasFocus = false;
	    repaint();
	    return true;

	case Event.KEY_ACTION:
	    switch(evt.key) {
	    	case Event.LEFT:
		    if(selStart > 0) {
			select(selStart - 1, selStart - 1);
		        scrollHorizontal(selStart);
			return true;
		    }
		    return false;
	    	case Event.RIGHT:
		    if(selStart < text.length()) {
			select(selEnd + 1, selEnd + 1);
		        scrollHorizontal(selStart);
			return true;
		    }
		    return false;
	    }
	case Event.KEY_PRESS:
	    if(field.isEditable()) {
		switch (evt.key) {
	      	case '\r':
	      	case '\n':
		    targetEvent(Event.ACTION_EVENT, text);
		    return true;

	      	case 8:
	      	case 127:
		    if (selStart != selEnd) {
			delete(selStart, selEnd);
		    } else if (selStart > 0) {
			delete(selStart-1, selStart);
		    }
		    return true;

	    	case 1: // ctrl-a
		    select(0, 0);
		    scrollHorizontal(selStart);
		    return true;
	   	case 2: // ctrl-b
		    if (selStart > 0) {
			select(selStart-1, selStart-1);
		    	scrollHorizontal(selStart);
		    }
		    return true;
		case 5: // ctrl-e
		    select(text.length(), text.length());
		    scrollHorizontal(selStart);
		    return true;
		case 6: // ctrl-f
		    if (selEnd < text.length()) {
			select(selEnd+1, selEnd+1);
			scrollHorizontal(selStart);
		    }
		return true;

	      	default:
		    if (selStart != selEnd) {
			delete(selStart, selEnd);
		    }	
		    char data[] = {(char)evt.key};
		    insert(selStart, new String(data));
		    return true;
		}
	    }	    
	}
	return false;
    }

    /** 
     * scroll to the position "pos"
     */
    void scrollHorizontal(int pos) {
	
	int x = pos2x(pos);
	int w = width - (2 * BORDER);
	int xmax = w + moved;
	int xmin = MARGIN + moved;
	
	if ((x > xmax) || (x < xmin)) {
	    int h = height - (2 * MARGIN);
	    Graphics g = getGraphics();
	    moved += (x > xmax) ? x - xmax : x - xmin;
	    g.clipRect(BORDER, MARGIN, w, h);
	    paintLine(g);
	    g.dispose();
	}
    }

    public void setEditable(boolean editable) {
	repaint();
    }
    
    public void setEchoCharacter(char c) {
    }
    
    public int getSelectionStart() {
	return selStart;
    }
    
    public int getSelectionEnd() {
	return selEnd;
    }
    
    public String getText() {
	return text;
    }
    
    void insert(int sel, String str) {
	text = text.substring(0, sel) + str + text.substring(sel);
	selStart += 1;
	selEnd = selStart;
    	Graphics g = getGraphics();
	g.clipRect(BORDER, MARGIN, width - (2 * BORDER), height - (2 * MARGIN));
	paintLine(g);
	g.dispose();
	scrollHorizontal(selStart);
		    
    }
    
    void delete(int start, int end) {
	text = text.substring(0, start) + text.substring(end);
	selStart = start;
	selEnd = selStart;
    	Graphics g = getGraphics();
	g.clipRect(BORDER, 4, width - (2 * BORDER), height - (2 * MARGIN));
	paintLine(g);
	g.dispose();
	scrollHorizontal(selStart);	    	    
    }

    public void select(int s, int e) {
	
	if ((s > text.length()) || (s < 0) || (e > text.length()) || (e < 0)) {
	    return;
	}
	
	if ((this.selStart == s) && (this.selEnd == e)) {
	    return;
	}
	
	int startOld = this.selStart;
	int endOld = this.selEnd;
	this.selEnd = e;
	this.selStart = s;
 	
	Graphics g = getGraphics();
	g.clipRect(BORDER, MARGIN, width - (2 * BORDER), height - (2 * MARGIN));	
	
	if ((selStart >= endOld) || (selEnd <= startOld)) {
	    paintRange(g, startOld, endOld);
	    paintRange(g, selStart, selEnd);	
	} else {
	    int start = Math.min(selStart, startOld);
	    int end = Math.max(selStart, startOld);
	    
	    if (start != end) {
		paintRange(g, start, end);
	    }	
	    start = Math.min(selEnd, endOld);
	    end = Math.max(selEnd, endOld);
				  
	    if (start != end) {
		paintRange(g, start, end);
	    }
   	}   
	g.dispose();
    }
      
    	

    public void setText(String text) {
	this.text = text;
	selStart = 0;
	selEnd = text.length();
	moved = 0;
	target.repaint();
    }
}









