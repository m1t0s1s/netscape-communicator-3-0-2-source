/*
 * @(#)X11FontMetrics.java	1.13 95/11/24 Jim Graham
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
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
import java.util.Hashtable;

/** 
 * A font metrics object for a WServer font.
 * 
 * @version 1.13 24 Nov 1995
 * @author Jim Graham
 */
class X11FontMetrics extends FontMetrics {
    /**
     * The widths of the first 256 characters.
     */
    int widths[];

    /** 
     * The standard ascent of the font.  This is the logical height
     * above the baseline for the Alphanumeric characters and should
     * be used for determining line spacing.  Note, however, that some
     * characters in the font may extend above this height.
     */
    int ascent;

    /** 
     * The standard descent of the font.  This is the logical height
     * below the baseline for the Alphanumeric characters and should
     * be used for determining line spacing.  Note, however, that some
     * characters in the font may extend below this height.
     */
    int descent;

    /** 
     * The standard leading for the font.  This is the logical amount
     * of space to be reserved between the descent of one line of text
     * and the ascent of the next line.  The height metric is calculated
     * to include this extra space.
     */
    int leading;

    /** 
     * The standard height of a line of text in this font.  This is
     * the distance between the baseline of adjacent lines of text.
     * It is the sum of the ascent+descent+leading.  There is no
     * guarantee that lines of text spaced at this distance will be
     * disjoint; such lines may overlap if some characters overshoot
     * the standard ascent and descent metrics.
     */
    int height;

    /** 
     * The maximum ascent for all characters in this font.  No character
     * will extend further above the baseline than this metric.
     */
    int maxAscent;

    /** 
     * The maximum descent for all characters in this font.  No character
     * will descend further below the baseline than this metric.
     */
    int maxDescent;

    /** 
     * The maximum possible height of a line of text in this font.
     * Adjacent lines of text spaced this distance apart will be
     * guaranteed not to overlap.  Note, however, that many paragraphs
     * that contain ordinary alphanumeric text may look too widely
     * spaced if this metric is used to determine line spacing.  The
     * height field should be preferred unless the text in a given
     * line contains particularly tall characters.
     */
    int maxHeight;

    /** 
     * The maximum advance width of any character in this font. 
     */
    int maxAdvance;

    /**
     * Calculate the metrics from the given WServer and font.
     */
    public X11FontMetrics(Font font) {
	super(font);
	init();
    }

    /**
     * Get leading
     */
    public int getLeading() {
	return leading;
    }

    /**
     * Get ascent.
     */
    public int getAscent() {
	return ascent;
    }

    /**
     * Get descent
     */
    public int getDescent() {
	return descent;
    }

    /**
     * Get height
     */
    public int getHeight() {
	return height;
    }

    /**
     * Get maxAscent
     */
    public int getMaxAscent() {
	return maxAscent;
    }

    /**
     * Get maxDescent
     */
    public int getMaxDescent() {
	return maxDescent;
    }

    /**
     * Get maxAdvance
     */
    public int getMaxAdvance() {
	return maxAdvance;
    }

    /** 
     * Return the width of the specified string in this Font. 
     */
    public native int stringWidth(String str);

    /** 
     * Return the smallest rectangle that contains the given string.
     */
    public native Dimension stringExtent( String str );
   
    /**
     * Return the width of the specified char[] in this Font.
     */
    public native int charsWidth(char data[], int off, int len);

    /**
     * Return the width of the specified byte[] in this Font. 
     */
    public native int bytesWidth(byte data[], int off, int len);

    /**
     * Get the widths of the first 256 characters in the font.
     */
    public int[] getWidths() {
	return widths;
    }

    native void init();

    static Hashtable table = new Hashtable();
    
    static synchronized FontMetrics getFontMetrics(Font font) {
	FontMetrics fm = (FontMetrics)table.get(font);
	if (fm == null) {
	    table.put(font, fm = new X11FontMetrics(font));
	}
	return fm;
    }
}
