/*
 * @(#)TinyGraphics.java	1.10 95/12/04 Arthur van Hoff
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
import java.io.*;
import java.util.*;
import java.awt.image.ImageObserver;
import sun.awt.image.OffScreenImageSource;
import sun.awt.image.ImageRepresentation;

/**
 * Graphics is an object that encapsulates a graphics context for a
 * particular canvas. 
 * 
 * @version 1.10 04 Dec 1995
 * @author Arthur van Hoff
 */

public class TinyGraphics extends Graphics {
    Color	color;
    Color	xorColor;
    Font	font;
    int		originX;
    int		originY;
    Rectangle 	clip;
    boolean	touched;

    int		pDrawable;

    private native void createFromGraphics(TinyGraphics g);
    private native void createFromWindow(TinyWindow win);

    TinyGraphics(TinyGraphics g) {
	createFromGraphics(g);
    }
    TinyGraphics(TinyWindow win) {
	createFromWindow(win);
    }

    private native void imageCreate(ImageRepresentation ir);
    sun.awt.image.Image image;

    public TinyGraphics(TinyImage image) {
	OffScreenImageSource osis = (OffScreenImageSource)image.getSource();
	imageCreate(osis.getImageRep());
	this.image = image;
    }

    /**
     * Create a new Graphics Object based on this one.
     */
    public Graphics create() {
	TinyGraphics g = new TinyGraphics(this);
	g.color = color;
	g.font = font;
	g.originX = originX;
	g.originY = originY;
	g.clip = clip;
	return g;
    }

    /**
     * Translate
     */
    public void translate(int x, int y) {
	originX += x;
	originY += y;
    }

    /**
     * Disposes of this Graphics context. 
     */
    public native void dispose();

    /**
     * Get the current font.
     */
    public Font getFont() {
	return font;
    }

    /**
     * Set the current font.
     */
    public void setFont(Font font) {
	if ((font != null) && (this.font != font)) {
	    touched = true;
	    this.font = font;
	}
    }

    /**
     * Gets font metrics for the given font.
     */
    public FontMetrics getFontMetrics(Font font) {
	return TinyFontMetrics.getFontMetrics(font);
    }

    /**
     * Get the current color.
     */
    public Color getColor() {
	return color;
    }

    /**
     * Sets the current color.
     */
    public void setColor(Color c) {
	if ((c != null) && (c != color)) {
	    touched = true;
	    color = c;
	}
    }

    /**
     * Sets the paint mode to overwrite the destination with the
     * current color. This is the default paint mode.
     */
    public void setPaintMode() {
	if (xorColor != null) {
	    touched = true;
	    xorColor = null;
	}
    }

    /**
     * Sets the paint mode to alternate between the current color
     * and the given color.
     */
    public void setXORMode(Color c) {
	if ((c != null) && (c != xorColor)) {
	    touched = true;
	    xorColor = c;
	}
    }

    /** 
     * Gets the current clipping area 
     */
    public Rectangle getClipRect() {
	return clip;
    }

    /** 
     * Sets the clipping rectangle for this Graphics context. 
     */
    public void clipRect(int x, int y, int w, int h) {
	touched = true;

	x = (int)(originX + x);
	y = (int)(originY + y);
	w = (int)(w);
	h = (int)(h);

	if (clip == null) {
	    clip = new Rectangle(x, y, w, h);
	} else {
	    clip = clip.intersection(new Rectangle(x, y, w, h));
	}
    }

    /** Draws the given line. */
    public native void drawLine(int x1, int y1, int x2, int y2);

    /** Fills the given rectangle with the foreground color. */
    public native void fillRect(int x, int y, int w, int h);
    
    /** Clears the rectangle indicated by x,y,w,h. */
    public void clearRect(int x, int y, int w, int h) {
	// REMIND
    }

    /** Draws the given string. */
    public native void drawString(String str, int x, int y);

    /** Draws character. */
    public native void drawChars(char data[], int offset, int length, int x, int y);

    /** Draws byres. */
    public native void drawBytes(byte data[], int offset, int length, int x, int y);

    /**
     * Draws an image at x,y in nonblocking mode with a callback object.
     */
    public boolean drawImage(Image img, int x, int y, ImageObserver observer) {
	TinyImage i = (TinyImage)img;
	if (i.hasError()) {
	    if (observer != null) {
		observer.imageUpdate(img,
				     ImageObserver.ERROR|ImageObserver.ABORT,
				     -1, -1, -1, -1);
	    }
	    return false;
	}
	ImageRepresentation ir = i.getImageRep(-1, -1);
	return ir.drawImage(this, x, y, null, observer);
    }

    /**
     * Draws an image scaled to x,y,w,h in nonblocking mode with a
     * callback object.
     */
    public boolean drawImage(Image img, int x, int y, int width, int height,
			     ImageObserver observer) {
	if (width == 0 || height == 0) {
	    return true;
	}
	TinyImage i = (TinyImage)img;
	if (i.hasError()) {
	    if (observer != null) {
		observer.imageUpdate(img,
				     ImageObserver.ERROR|ImageObserver.ABORT,
				     -1, -1, -1, -1);
	    }
	    return false;
	}
	if (width < 0) width = -1;
	if (height < 0) height = -1;
	ImageRepresentation ir = i.getImageRep(width, height);
	return ir.drawImage(this, x, y, null, observer);
    }

    /**
     * Draws an image at x,y in nonblocking mode with a solid background
     * color and a callback object.
     */
    public boolean drawImage(Image img, int x, int y, Color bg,
			     ImageObserver observer) {
	TinyImage i = (TinyImage)img;
	if (i.hasError()) {
	    if (observer != null) {
		observer.imageUpdate(img,
				     ImageObserver.ERROR|ImageObserver.ABORT,
				     -1, -1, -1, -1);
	    }
	    return false;
	}
	ImageRepresentation ir = i.getImageRep(-1, -1);
	return ir.drawImage(this, x, y, bg, observer);
    }

    /**
     * Draws an image scaled to x,y,w,h in nonblocking mode with a
     * solid background color and a callback object.
     */
    public boolean drawImage(Image img, int x, int y, int width, int height,
			     Color bg, ImageObserver observer) {
	if (width == 0 || height == 0) {
	    return true;
	}
	TinyImage i = (TinyImage)img;
	if (i.hasError()) {
	    if (observer != null) {
		observer.imageUpdate(img,
				     ImageObserver.ERROR|ImageObserver.ABORT,
				     -1, -1, -1, -1);
	    }
	    return false;
	}
	if (width < 0) width = -1;
	if (height < 0) height = -1;
	ImageRepresentation ir = i.getImageRep(width, height);
	return ir.drawImage(this, x, y, bg, observer);
    }

    /**
     * Copies an area of the canvas that this graphics context paints to.
     * @param X the x-coordinate of the source.
     * @param Y the y-coordinate of the source.
     * @param W the width.
     * @param H the height.
     * @param dx the x-coordinate of the destination.
     * @param dy the y-coordinate of the destination.
     */
    public native void copyArea(int x, int y, int w, int h, int dx, int dy);


    /** Draws a rounded rectangle. */
    public void drawRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight) {
	int aw = arcWidth / 2;
	int ah = arcHeight / 2;
	int x2 = x + w;
	int y2 = y + h;

	drawArc(x, y, arcWidth, arcHeight, 90, 90);
	drawLine(x + aw + 1, y, x2 - aw - 1, y);
	drawArc(x2 - arcWidth, y, arcWidth, arcHeight, 0, 90);
	drawLine(x2, y + ah + 1, x2, y2 - ah - 1);
	drawArc(x2 - arcWidth, y2 - arcHeight, arcWidth, arcHeight, 270, 90);
	drawLine(x2 - aw - 1, y2, x + aw + 1, y2);
	drawArc(x, y2 - arcHeight, arcWidth, arcHeight, 180, 90);
	drawLine(x, y2 - ah - 1, x, y + ah + 1);
    }

    /** Draws a filled rounded rectangle. */
    public void fillRoundRect(int x, int y, int w, int h, int arcWidth, int arcHeight) {
	int aw = arcWidth / 2;
	int ah = arcHeight / 2;
	int x2 = x + w;
	int y2 = y + h;

	fillRect(x, y + ah, aw, h - arcHeight);
	fillRect(x + aw, y, w - arcWidth, h);
	fillRect(x2 - aw, y + ah, aw, h - arcHeight);
	fillArc(x, y, arcWidth, arcHeight, 90, 90);
	fillArc(x2 - arcWidth, y, arcWidth, arcHeight, 0, 90);
	fillArc(x2 - arcWidth, y2 - arcHeight, arcWidth, arcHeight, 270, 90);
	fillArc(x, y2 - arcHeight, arcWidth, arcHeight, 180, 90);
    }

    /** Draws a polygon defined by an array of x points and y points */
    public void drawPolygon(int xPoints[], int yPoints[], int nPoints) {
	for (int i = 1 ; i < nPoints ; i++) {
	    drawLine(xPoints[i-1], yPoints[i-1], xPoints[i], yPoints[i]);
	}
    }

    /** Fills a polygon with the current fill mask */
    public native void fillPolygon(int xPoints[], int yPoints[], int nPoints);

    /** Draws an oval to fit in the given rectangle */
    public void drawOval(int x, int y, int w, int h) {
	drawArc(x, y, w, h, 0, 360);
    }

    /** Fills an oval to fit in the given rectangle */
    public void fillOval(int x, int y, int w, int h) {
	fillArc(x, y, w, h, 0, 360);
    }

    /**
     * Draws an arc bounded by the given rectangle from startAngle to
     * endAngle. 0 degrees is a vertical line straight up from the
     * center of the rectangle. Positive angles indicate clockwise
     * rotations, negative angle are counter-clockwise.
     */
    public native void drawArc(int x, int y, int w, int h, int startAngle, int endAngle);

    /** fills an arc. arguments are the same as drawArc. */
    public native void fillArc(int x, int y, int w, int h, int startAngle, int endAngle);

    public String toString() {	
	return getClass().getName() + "[" + originX + "," + originY + "]";
    }
}
