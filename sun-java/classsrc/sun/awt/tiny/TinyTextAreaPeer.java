/*
 * @(#)TinyTextAreaPeer.java
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

class TinyTextAreaPeer extends TinyComponentPeer implements TextAreaPeer, TinyScrollbarClient {
    
    public static final int MARGIN = 4;
    public static final int BORDER = 2;
    public static final int SCROLLBAR = 17;
    public static final int HORSCROLLBAR = 0;
    public static final int VERSCROLLBAR = 1;
    public static final int TEXTAREA = 2;
    
    TinyVerticalScrollbar 	vsb;
    TinyHorizontalScrollbar 	hsb;
    
    boolean 	hasFocus;
    char 	text[];
    int 	lines[];
    int 	nrOfLines;
    int 	textLength;
    int 	mpos;
    int 	selStart;
    int 	selEnd;
    long 	mtm;
    int 	topLine;
    int 	movedRight;
    int 	active;
    int 	fontAscent;
    int 	fontLeading;
    int 	fontHeight;
    
    /**
     * Create a Text area.
     */
    TinyTextAreaPeer(TextArea target) {
	winCreate((TinyComponentPeer)target.getParent().getPeer());
	init(target);
	target.isEditable();
        
 	FontMetrics fm = getFontMetrics(target.getFont());
	fontAscent = fm.getAscent();
	fontLeading = fm.getLeading();
	fontHeight = fm.getHeight();
	
	// initialize Horizontal Scrollbar
	hsb = new TinyHorizontalScrollbar();
	hsb.sb = this;
	hsb.line = 10;

        // initialize Vertical Scrollbar
	vsb = new TinyVerticalScrollbar();
	vsb.sb = this;
	vsb.line = 1;

	// set the text of this object to the text of it's target
	setText(target.getText());
    }

    /**
     * Compute minimum size.
     */
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

    
    /**
     * Update the index Array from the specified lineNumber
     */
    protected void updateIndex(int nr) {
    	nrOfLines = nr + 1;
	int offset = 0;
	lines[0] = 0;
	for (int i = lines[nr]; i < textLength; i++) {
	    if (lines.length == nrOfLines) {
	        int tmp[] = new int[nrOfLines + 10];
		System.arraycopy(lines, 0, tmp, 0, nrOfLines);
		lines = tmp;        	
	    }	    
	    if(text[i] == '\n') {
		offset = i + 1;
		lines[nrOfLines++] = offset;
	    }
	    else if(text[i] == '\r') {
		offset = i + 1;
		if((i+1) < textLength) {
		    if(text[i+1] == '\n') {
			offset++;
		    }		    
		}
		lines[nrOfLines++] = offset;
	    }
	}
    }

    /** 
     * return the position on the line "ln" given the old xPosition
     */
    int xPos(int xo, int ln) {
	FontMetrics fm = getFontMetrics(target.getFont());	
	int pose = (ln + 1 < nrOfLines) ? lines[ln + 1] - 1 : textLength;	
	int x = MARGIN, widths[] = fm.getWidths();
	int posn = lines[ln];
	while((x < xo) && (posn < pose)) {
	    x += widths[text[posn]];
	    posn++;	
	}
	return posn;
    }	

    /**
     * number of lines that can fit in the window
     */
    int linesInWindow() {
	int htotal = height - ((2 * MARGIN) + SCROLLBAR);
	return htotal / fontHeight;
    }

    /** 
     * return the position in the array "lines" from the values x and y.
     */
    public int xy2pos(int x, int y) {
    	int ln = 0;
	int endl = lineNumber(selEnd);
	if (y < 0) {
	    if (topLine - 1 >= 0) { 
	    	ln = topLine - 1;
	    } else {
		return 0;
	    }	
	} else if (y > height) {
	    if (endl < nrOfLines - 1) {
		ln = endl + 1;
	    } else {
		return textLength;
	    }
	} else {
	    ln = topLine;
	    int ys = MARGIN + fontHeight;
	    while ((y > ys) && (ln < nrOfLines - 1)) {
		ys += fontHeight;
	    	ln++;
	    }
        }
	FontMetrics fm = getFontMetrics(target.getFont());	
	int widths[] = fm.getWidths();
	int begin = lines[ln];
	int end = (ln + 1 < nrOfLines) ? lines[ln + 1] - 1 : textLength;
	x -= MARGIN;
	for (int i = begin; i < end ; i++) {
	    int w = widths[text[i]];
	    if (x < (w / 2)) {
		return i;
	    }
	    x -= w;
	}
	return end;
    }
    
    /**
     * return the x position of the position pos 	
     */
     int pos2x(int pos) {
	FontMetrics fm = getFontMetrics(target.getFont());	
	int widths[] = fm.getWidths();
	int ln = lineNumber(pos);
	int begin = lines[ln];
	int x = MARGIN;
	for (int i = begin ; i < pos ; i++) {
	    x += widths[text[i]];
	}
	return x;
    }
    
    /**
     * return the y position of the lineNumber
     */
    int ln2y(int ln) {
	return MARGIN + fontLeading + ((ln - topLine) * fontHeight);
    }

    /**
     * return the lineNumber
     * use binairy search
     */
    int lineNumber(int pos) {	
	int i = 0, j = nrOfLines - 1;
	while (i < j) {
	    int m = i + ((j - i) >> 1);
	    int s = lines[m];
	    int e = lines[m + 1];
	    if (pos < s) {
		j = m - 1;
	    } else if (pos >= e) {
		i = m + 1;
	    } else {
		return m;
	    }
	}
	return i;
    }

    /**
     * return the length of the largest line
     */
    int maxLength() {
	FontMetrics fm = getFontMetrics(target.getFont());	
	int max = hsb.val;
	for(int i = 0 ; i < nrOfLines ; i++) {
	    int b = lines[i];
	    int e = (i + 1 == nrOfLines) ? textLength - 1 : lines[i + 1];
	    int l = fm.charsWidth(text, b, e - b);
	    max = Math.max(max, l);
	}
    	return max;
    }

    /**
     * Paint the vertical scrollbar to the screen
     */
    void paintVerScrollbar(Graphics g) {
	
	// initialize scrollbar
	int l = linesInWindow();
	vsb.vis = l;
	vsb.page = l;
	vsb.max = Math.max(0, nrOfLines - l);
	vsb.val = Math.min(topLine, vsb.max);
	
	// paint scrollbar
	Graphics ng = g.create();
	g.setColor(target.getBackground());
	g.fillRect(width - 14, 1, 13, height - (SCROLLBAR - 2));
	
	try {
	    ng.translate(width - (SCROLLBAR - 2), 0);
	    vsb.paint(ng, target.getBackground(), SCROLLBAR - 2, height - (SCROLLBAR - 2));
	} finally {
	    ng.dispose();
	}
    }    
    
    /**
     * paint the horizontal scrollbar to the screen
     */
    void paintHorScrollbar(Graphics g) {
	int w = width - ((2 * BORDER) + SCROLLBAR);
	
	// initialize
	hsb.vis = w;
	hsb.page = hsb.vis;
	hsb.max = Math.max(0, (MARGIN + maxLength()) - w);
	hsb.val = Math.min(hsb.max, movedRight);
	
	// paint scrollbar
	Graphics ng = g.create();
	g.setColor(target.getBackground());
	g.fillRect(1, height - 14, width - (SCROLLBAR + 2), 13);
	
	try {
	    ng.translate(0, height - (SCROLLBAR - 2));
	    hsb.paint(ng, target.getBackground(), width - (SCROLLBAR - 2), SCROLLBAR - 2);
	} finally {
	    ng.dispose();
	}
    }       
	    
   /**
    * Paint the component
    * this method is called when the repaint instruction has been used
    */
    public void paintComponent(Graphics g) {
	TextArea area = (TextArea)target;
	
	int w = width - ((2 * BORDER) + SCROLLBAR);
	int h = height - ((2 * BORDER) + SCROLLBAR);
	
	g.setFont(area.getFont());
	g.setColor(area.isEditable() ? target.getBackground().brighter() : target.getBackground());
	g.fillRect(BORDER, BORDER, w, h);
	paintVerScrollbar(g);
	paintHorScrollbar(g);
	
	if (hasFocus) {
	    g.setColor(Color.black);
	    g.drawRect(0, 0, width - (SCROLLBAR + 1), height - (SCROLLBAR + 1));
	    g.drawRect(1, 1, width - (SCROLLBAR + 3), height - (SCROLLBAR + 3));	    
	} else {
	    g.setColor(target.getBackground());
	    g.drawRect(0, 0, width - (SCROLLBAR + 1), height - (SCROLLBAR + 1));
	    g.draw3DRect(1, 1, width - (SCROLLBAR + 3), height - (SCROLLBAR + 3), false);
	} 
	
	if (text != null) {
	    int l = linesInWindow();
	    h = height - ((2 * MARGIN) + SCROLLBAR);
	    int e = Math.min(nrOfLines - 1, (topLine + l) - 1);
	    paintLines(g, topLine, e);
	}
    }
    
    /**
     * fill the position on the screen with the desired color
     * the color is given by "s". 
     * true = selectColor
     * false = backGroundColor
     */
    void paintRange(Graphics g, int start, int end) {
	int lnrStart = lineNumber(start);
	int lnrEnd = lineNumber(end);
	Graphics ng = g.create();
	
	if (lnrStart == lnrEnd) {
	    int xmax = width - (BORDER + SCROLLBAR);
	    int xs = Math.max(BORDER, pos2x(start) - movedRight);
	    int xe = Math.min(xmax, pos2x(end) - movedRight);
	    int w = (start == end) ? 1 : xe - xs;
	    int y = ln2y(lnrStart) - fontLeading;
	    ng.clipRect(xs, y, w, fontHeight);
	    paintLine(ng, lnrStart);
	} else {
	    int diff = lnrEnd - lnrStart;
	    int w = width - ((2 * BORDER) + SCROLLBAR);
	    int h = height - ((2 * MARGIN) + SCROLLBAR);
	    ng.clipRect(BORDER, MARGIN, w, h);
	    paintLine(ng, lnrStart);
	    lnrStart++;
	    
	    while (diff > 1) {
		paintLine(ng, lnrStart);
		lnrStart++;
		diff--;
	    }
	    paintLine(ng, lnrEnd);
	}
   	ng.dispose();
    }
    	    
    /**
     * draw the lines to the screen
     */
    void paintLines(Graphics g, int s, int e) {
	int w = width - ((2 * BORDER) + SCROLLBAR);
	int h = height - ((2 * MARGIN) + SCROLLBAR);
	int lm = linesInWindow() + topLine;
	s = Math.max(topLine, s);
	e = Math.min(e, lm - 1);
	Graphics ng = g.create();
	ng.clipRect(BORDER, MARGIN, w, h);
	
	for (int i = s ; i <= e; i++) {
	    paintLine(ng, i);
	}     
    	ng.dispose();
    }

    /**
     * paint the background color of the line "lnr" to the screen and then paint draw the 
     * line to the screen
     * if the line number is greater than nrOfLines - 1 than do a fillrect to the
     * position of the line, use the background color
     */
    void paintLine(Graphics g, int lnr) {
	int l = linesInWindow();

	if((lnr < topLine) || (lnr >= l + topLine)) { 
	    return;
	}
	int w = width - ((2 * BORDER) + SCROLLBAR); 
	int y = ln2y(lnr);
	
	if (lnr > nrOfLines - 1) {
	    g.setColor(backGroundColor());
	    g.fillRect(BORDER, y - fontLeading, w, fontHeight);
	    return;
	}
	int s = lines[lnr];
	int e = (lnr < nrOfLines - 1) ? lines[lnr + 1] - 1 : textLength;
	int xs = pos2x(selStart) - movedRight;
	int xe = pos2x(selEnd) - movedRight;

	if ((selStart < s) && (selEnd > e)) {
	    g.setColor(selectColor());
	    g.fillRect(BORDER, y - fontLeading, w, fontHeight);
	} else {
	    g.setColor(backGroundColor());
	    g.fillRect(BORDER, y - fontLeading, w, fontHeight);
		
	    if ((selStart >= s) && (selStart <= e)) {
		g.setColor(selectColor());
		    
		if (selEnd > e) {
		    g.fillRect(xs, y - fontLeading, (w + BORDER) - xs, fontHeight);
		} else if (selStart == selEnd) { 
		    g.fillRect(xs, y - fontLeading, 1, fontHeight);
		} else { 
			g.fillRect(xs, y - fontLeading, xe - xs, fontHeight);
		}		
	    } else if ((selEnd >= s) && (selEnd <= e)) {
		g.setColor(selectColor());
		g.fillRect(BORDER, y - fontLeading, xe - BORDER, fontHeight);
	    }	    
	}
	g.setColor(target.getForeground());
	g.drawChars(text, s, e - s, MARGIN - movedRight, y + fontAscent);	
    }    

    /**
     * return the background color
     */
    Color backGroundColor() {
	TextArea area = (TextArea)target;
	return (area.isEditable() ? target.getBackground().brighter() : target.getBackground()); 
    }

    /**
     * return the selection color
     */
    Color selectColor() {
	TextArea area = (TextArea)target;
	return (area.isEditable() ? target.getBackground() : target.getBackground().darker());
    }
    
    /**
     * get the value back from the scrollbar
     */
    public void notifyValue(Object obj, int c, int b) {
    	int w = width - ((2 * BORDER) + SCROLLBAR);
	int h = height - ((2 * BORDER) + SCROLLBAR);
	Graphics g = getGraphics();
	
	if ((TinyVerticalScrollbar)obj == vsb) {
	    scrollVertical(g, b - topLine);
	    paintVerScrollbar(g);
	} else if ((TinyHorizontalScrollbar)obj == hsb) {
	    scrollHorizontal(g, b - movedRight); 
	    paintHorScrollbar(g);
	}
    	g.dispose();
    }

    /**
     * is (x, y) in the horizontal scrollbar
     */
    boolean inHorScrollbar(int x, int y) {
	return (y >= height - SCROLLBAR) && (y <= height) && (x <= width - SCROLLBAR);
    }
    
    /**
     * is (x, y) in the vertical scrollbar
     */
     boolean inVerScrollbar(int x, int y) {
	return (x >= width - SCROLLBAR) && (x <= width) && (y <= height - SCROLLBAR);
    }
    
    /**
     * is (x, y) in the Textfield
     */ 
    boolean inTextField(int x, int y) {
	return (x < width - SCROLLBAR) && (y < height - SCROLLBAR); 
    }

    /**
     * Handle events.
     */
    public boolean handleWindowEvent(Event evt) {
	TextArea area = (TextArea)target;
	switch (evt.id) {
			
	case Event.MOUSE_DOWN:
	    if (inVerScrollbar(evt.x, evt.y)) {
		evt.x -= width - SCROLLBAR;
		active = VERSCROLLBAR;
		return vsb.handleWindowEvent(evt, SCROLLBAR, height - SCROLLBAR);
	    } else if (inHorScrollbar(evt.x, evt.y)) {	
		evt.y -= height - SCROLLBAR;
		active = HORSCROLLBAR;
		return hsb.handleWindowEvent(evt, width - SCROLLBAR, SCROLLBAR);
	    } else if (inTextField(evt.x, evt.y)) {
		if (!hasFocus()) {
		    requestFocus();
		}
	    	active = TEXTAREA;
		mpos = xy2pos(evt.x + movedRight, evt.y);
	    	select(mpos, mpos);
		return true;
	    }
	    break;
	
	case Event.MOUSE_UP:
	    if ((evt.when - mtm < 200) && (inTextField(evt.x, evt.y))) {
		select(0, textLength);
	    }
	    mtm = evt.when;
	    return true;

	case Event.MOUSE_DRAG:
	    
	    if (active == VERSCROLLBAR) {
		evt.x -= width - SCROLLBAR;
		return vsb.handleWindowEvent(evt, SCROLLBAR, height - SCROLLBAR);
	    } else if (active == HORSCROLLBAR) {	
		evt.y -= height - SCROLLBAR;
		return hsb.handleWindowEvent(evt, width - SCROLLBAR, SCROLLBAR);
	    } else if (active == TEXTAREA) {
		int mpos2 = xy2pos(evt.x + movedRight, evt.y);
	    	if (mpos < mpos2) {
		    select(mpos, mpos2);	
		    scrollTo(selEnd);
		} else {
		    select(mpos2, mpos);
		    scrollTo(selStart); 
		}
		return true;
	    }
	 
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
	        case Event.UP:
		     int ln = lineNumber(selStart);
		     if(ln > 0) {
			 int xo = pos2x(selStart);
			 int posn = xPos(xo, ln - 1);
		    	 select(posn, posn);
		     	 scrollTo(selStart);
			 return true;
		     }	
		     return false;
	 
	    	case Event.LEFT:
		    if(selStart > 0) {
			select(selStart - 1, selStart - 1);
		        scrollTo(selStart);
		        return true;
		    }
		    return false;
		    
	    	case Event.RIGHT:
		    if(selStart < textLength) {
			select(selStart + 1, selStart + 1);
		        scrollTo(selStart);
			return true;
		    }
		    return false;
		    	
		case Event.DOWN:
		    int lnr = lineNumber(selStart);
		    if(lnr + 1 < nrOfLines) { 
			int xo = pos2x(selStart);
			int posn = xPos(xo, lnr + 1);
		    	select(posn, posn);
		    	scrollTo(selStart);
			return true;
		    }
		    return false;
	    	}
         case Event.KEY_PRESS:
	     if(area.isEditable()) {
	    	switch (evt.key) {
	    		
		case '\r':
		    if(selStart != selEnd) {
			delete(selStart, selEnd);
		    }
		    insertText("\r", selStart);
		    return true;
	   	
		case '\n':
	    	    if(selStart != selEnd) {
		       delete(selStart, selEnd);
		    }
		    insertText("\n", selStart);
		    return true;
	   		
	        case 8:
	        case 127:
		    if(selStart != selEnd) {
		        delete(selStart, selEnd);
		    }		    
		    else if(selStart > 0) {
		        delete(selStart-1, selStart);
		    }
		    return true;

	        default:
		    char data[] = {(char)evt.key};
		    if(selStart != selEnd) {
		        replaceText(new String(data), selStart, selEnd);
		    }
		    else {
		        insertText(new String(data), selStart);
		    }
		    return true;
	        }
            }
	}	    
	return false;
    }
    
    /**
     * scroll the text to the position "pos". Pos is the position in the text.
     * if the pos is already on the screen then do nothing
     */
    void scrollTo(int pos) {
	
	int lnr = lineNumber(pos);
	int l = linesInWindow();
	Graphics g = getGraphics();
	
	// scroll vertical
	if (lnr >= l + topLine) {
	    if (selStart < selEnd) {
		scrollVertical(g, 1);
	    } else if (lnr == topLine + l) {
		scrollVertical(g, (l + 1) / 2);
	    } else {
		int i = (topLine + l) - 1;
		int j = (lnr - i) + (l / 2);
		scrollVertical(g, j);
	    }
	} else if (lnr < topLine) {
	    if(selStart < selEnd) {
		scrollVertical(g, -1);
	    } else if(lnr + 1 == topLine) {
		scrollVertical(g, (-l - 1) / 2);
	    } else {
		scrollVertical(g, lnr - (topLine + 1));
	    }
	}
 	
	int x = pos2x(pos);
	int w = width - (MARGIN + SCROLLBAR);
	int xMax = w + movedRight;
	int xMin = movedRight + MARGIN;

	// scroll horizontal
	if (x > xMax) {
	    int diff = x - xMax;
	    scrollHorizontal(g, diff);
	} else if (x < xMin) {
	    int diff = x - xMin;
	    scrollHorizontal(g, diff);
	}
	paintHorScrollbar(g);
	paintVerScrollbar(g);
	g.dispose();
    }

    /**
     * Scroll "mp" horizontal 
     */
    void scrollHorizontal(Graphics g, int mp) {
	int l = linesInWindow();
	int le = Math.min((topLine + l) - 1, nrOfLines - 1);
	movedRight += mp;
	paintLines(g, topLine, le);
    } 

    /**
     * scroll the text "ml"  times vertical.
     */
    void scrollVertical(Graphics g, int ml) {
			
	if (topLine < -ml) {
	    ml = -topLine;
	}
	
	if (ml < 0) {
	    int l = linesInWindow();
	    topLine += ml;
            
	    if(ml + l > 0) {
		int hg = ((ml + l) * fontHeight);
		int w = width - ((2 * BORDER) + SCROLLBAR);
		g.copyArea(BORDER, MARGIN, w, hg, 0, -ml * fontHeight);  
	    } 	
	    int le = Math.min(nrOfLines - 1, topLine + -ml); 
	    paintLines(g, topLine, le);
	} else if (ml > 0) {
	    int l = linesInWindow();
	    topLine += ml;
	    	    
	    if (ml < l) {
		int y = MARGIN + (ml * fontHeight); 
		int hg = ((l - ml) * fontHeight);
		int w = width - ((2 * BORDER) + SCROLLBAR);
		g.copyArea(BORDER, y, w, hg, 0, -ml * fontHeight);  
	    }
	    paintLines(g, topLine + (l - ml), (topLine + l) - 1);
	}
    }
    
    /**
     * setEditable
     */
    public void setEditable(boolean editable) {
	repaint();
    }
    
    /**
     * get the selection start
     */
    public int getSelectionStart() {
	return selStart;
    }
    
    /**
     * get the selection end
     */
    public int getSelectionEnd() {
	return selEnd;
    }
    
    /**
     * get the text
     */
    public String getText() {
	return String.valueOf(text, 0, textLength);
    }
    
    /**
     * select the text between selStart and selEnd
     */
    public void select(int s, int e) {
    	
	if ((s > textLength) || (s < 0) || (e > textLength) || (e < 0)) {
	    return;
	}
	
	if ((this.selStart == s) && (this.selEnd == e)) {
	    return;
	}
	
	int startOld = this.selStart;
	int endOld = this.selEnd;
	selEnd = e;
	selStart = s;
 	Graphics g = getGraphics();
		
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

    /**
     * Set the text.
     */
    public void setText(String t) {
	textLength = t.length();
	this.text = new char[textLength+100]; 
	int nr = Math.max(textLength / 20, 1);
	lines = new int[nr];
	t.getChars(0, textLength, this.text, 0); 
	updateIndex(0);
	selStart = 0;
	selEnd = selStart;
	topLine = 0;
	movedRight = 0;
	target.repaint(); 
    }
    
    /** 
     * insert the text "txt on position "pos" in the array lines
     */
    public void insertText(String txt, int p) {
	TextArea area = (TextArea)target;
	
	if((p >= 0) && (p <= textLength)) {
	    int l = txt.length();
	    int lnr = lineNumber(p);
	    char ln[] = new char[l];
	    txt.getChars(0, l, ln, 0);
	    
	    if (text.length < textLength + l) {
		char tmp[] = new char[textLength + l + 20];
		System.arraycopy(text, 0, tmp, 0, textLength);
		text = tmp;
	    }
	    System.arraycopy(text, p, text, p + l, textLength - p);
	    System.arraycopy(ln, 0, text, p, l);
	    textLength += l;
	    int oldStart = selStart;
	    selStart += l;
	    selEnd = selStart;
	    int nr = nrOfLines;
	    updateIndex(lnr);    
	    int li = linesInWindow();
	    int le = (nrOfLines != nr) ? (topLine + li) - 1 : lnr;
	    Graphics g = getGraphics();
	    paintLines(g, lnr, le);
	    g.dispose();
	    scrollTo(selStart);
	}
    }
    
    /**
     * replace the text between the position "start" and "end" with "txt"
     */
    public void replaceText(String txt, int s, int e) {
    		
	if ((e > textLength) || (s > e) || (s < 0)) {
	    return;
	}
	TextArea area = (TextArea)target;
	delete(s, e);
	insertText(txt, s);    
    }
    
    /**
     * remove the text from the position start to the position end in the aray lines
     */
    protected void delete(int s, int e) {
    	int lnr = lineNumber(s);
	int length = e - s;	
	System.arraycopy(text, e, text, s, textLength - e); 
	textLength -= length;
	selStart = s;
	selEnd = s;
	int nr = nrOfLines;
	updateIndex(lnr);    
	Graphics g = getGraphics();
	int li = linesInWindow();
	int le = (nrOfLines != nr) ? (topLine + li) - 1 : lnr;
	paintLines(g, lnr, le);
    	g.dispose();
	scrollTo(selStart);
    }
}













































