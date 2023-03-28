package sun.awt.macos;

import java.awt.*;
import java.io.*;
import java.util.*;
import java.awt.image.ImageObserver;
import sun.awt.image.OffScreenImageSource;
import sun.awt.image.ImageRepresentation;

public class MacGraphics extends Graphics {

    int		pData;

    Color	foreground;
    Font	font;
    int		originX;	// REMIND: change to float
    int		originY;	// REMIND: change to float
    float	scaleX = 1.0f;
    float	scaleY = 1.0f;

    private native void createFromComponent(MComponentPeer w);
    private native void createFromGraphics(MacGraphics g);
    private native void createFromIRepresentation(ImageRepresentation ir);
    private native void pSetFont(Font f);
    private native void pSetForeground(Color c);
    private native void pSetScaling(float sx, float sy);
    private native void pSetOrigin(int x, int y);

    ImageRepresentation imageRepresentation;
    MComponentPeer	componentPeer;

    MacGraphics(MacGraphics g) {
		imageRepresentation = g.imageRepresentation;
		componentPeer = g.componentPeer;
		createFromGraphics(g);
    }

    MacGraphics(MComponentPeer comp) {
		componentPeer = comp;
		createFromComponent(comp);
    }

	public MacGraphics(Image image) {
		OffScreenImageSource osis = (OffScreenImageSource)image.getSource();
		imageRepresentation = osis.getImageRep();
		createFromIRepresentation(imageRepresentation);
        setFont(new Font("Dialog", Font.PLAIN, 12));
	}

    /**
     * Create a new MacGraphics Object based on this one.
     */
    public Graphics create() {
	MacGraphics g = new MacGraphics(this);
	g.foreground = foreground;
	g.font = font;
	g.originX = originX;
	g.originY = originY;
	g.scaleX = scaleX;
	g.scaleY = scaleY;
	g.imageRepresentation = imageRepresentation;
	return g;
    }

    /**
     * Translate
     */
    public void translate(int x, int y) {
	originX += x * scaleX;
	originY += y * scaleY;
	pSetOrigin(originX, originY);
    }

    /**
     * Scale
     */
    public void scale(float sx, float sy) {
	scaleX *= sx;
	scaleY *= sy;
	pSetScaling(scaleX, scaleY);
    }

    /**
     * Disposes of this MacGraphics context. It cannot be used after being
     * disposed.
     */
    public native void dispose();

	public void finalize() { dispose(); }

    public void setFont(Font font) {
	if ((font != null) && (this.font != font)) {
	    this.font = font;
	    pSetFont(font);
	}
    }
    public Font getFont() {
	return font;
    }

    /**
     * Gets font metrics for the given font.
     */
    public FontMetrics getFontMetrics(Font font) {
		return MacFontMetrics.getFontMetrics(font);
    }

    /**
     * Sets the foreground color.
     */
    public void setColor(Color c) {
	if ((c != null) && (c != foreground)) {
	    foreground = c;
	    pSetForeground(c);
	}
    }
    public Color getColor() {
	return foreground;
    }

    /**
     * Sets the paint mode to overwrite the destination with the
     * current color. This is the default paint mode.
     */
    public native void setPaintMode();

    /**
     * Sets the paint mode to alternate between the current color
     * and the given color.
     */
    public native void setXORMode(Color c1);

    /** 
     * Gets the current clipping area 
     */
    public Rectangle getClipRect() {
	Rectangle clip = new Rectangle();
	getClipRect(clip);
	return clip;
    }
    native void getClipRect(Rectangle clip);

    /** Sets the clipping rectangle for this MacGraphics context. */
    public native void clipRect(int X, int Y, int W, int H);
    
    /** Clears the rectangle indicated by x,y,w,h. */
    public native void clearRect(int x, int y, int w, int h);

    /** Fills the given rectangle with the foreground color. */
    public native void fillRect(int X, int Y, int W, int H);

    /** Draws the given rectangle. */
    public native void drawRect(int X, int Y, int W, int H);


    /** Draws the given string. */
    public native void drawString(String str, int x, int y);

    /** Draws the given character array. */
    public native void drawChars(char data[], int offset, int length, int x, int y);

    /** Draws the given byte array. */
    public native void drawBytes(byte data[], int offset, int length, int x, int y);

    /** Draws the given string and returns the length of the drawn
      string in pixels.  If font isn't set then returns -1. */
    public native int drawStringWidth(String str, int x, int y);

    /** Draws the given character array and return the width in
      pixels. If font isn't set then returns -1. */
    public native int drawCharsWidth(char data[], int offset, int length, int x, int y);

    /** Draws the given character array and return the width in
      pixels. If font isn't set then returns -1. */
    public native int drawBytesWidth(byte data[], int offset, int length, int x, int y);

    /** Draws the given line. */
    public native void drawLine(int x1, int y1, int x2, int y2);

    /**
     * Draws an image at x,y in nonblocking mode with a callback object.
     */
    public boolean drawImage(Image img, int x, int y, ImageObserver observer) {
		ImageRepresentation ir = ((MacImage)img).getImageRep(-1, -1);
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
		if (width < 0) width = -1;
		if (height < 0) height = -1;
		ImageRepresentation ir = ((MacImage)img).getImageRep(width, height);
		return ir.drawImage(this, x, y, null, observer);
    }


    /**
     * Draws an image at x,y in nonblocking mode with a solid background
     * color and a callback object.
     */
    public boolean drawImage(Image img, int x, int y, Color bg,
			     ImageObserver observer) {
		MacImage macimg = (MacImage) img;
		ImageRepresentation ir = macimg.getImageRep(-1, -1);
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
		MacImage macimg = (MacImage) img;
		if (width < 0) width = -1;
		if (height < 0) height = -1;
		ImageRepresentation ir = macimg.getImageRep(width, height);
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
    public native void copyArea(int X, int Y, int W, int H, int dx, int dy);


    /** Draws a rounded rectangle. */
    public native void drawRoundRect(int x, int y, int w, int h,
				     int arcWidth, int arcHeight);

    /** Draws a filled rounded rectangle. */
    public native void fillRoundRect(int x, int y, int w, int h,
				     int arcWidth, int arcHeight);

    /** Draws a polygon defined by an array of x points and y points */
    public native void drawPolygon(int xPoints[], int yPoints[], int nPoints);

    /** Fills a polygon with the current fill mask */
    public native void fillPolygon(int xPoints[], int yPoints[], int nPoints);

    /** Draws an oval to fit in the given rectangle */
    public native void drawOval(int x, int y, int w, int h);

    /** Fills an oval to fit in the given rectangle */
    public native void fillOval(int x, int y, int w, int h);

    /**
     * Draws an arc bounded by the given rectangle from startAngle to
     * endAngle. 0 degrees is a vertical line straight up from the
     * center of the rectangle. Positive angles indicate clockwise
     * rotations, negative angle are counter-clockwise.
     */
    public native void drawArc(int x, int y, int w, int h,
			       int startAngle,
			       int endAngle);

    /** fills an arc. arguments are the same as drawArc. */
    public native void fillArc(int x, int y, int w, int h,
			       int startAngle,
			       int endAngle);

    public String toString() {	
	return getClass().getName() + "[" + originX + "," + originY + "," + scaleX + "," + scaleY + "]";
    }

    /* Outline the given region. */
    //public native void drawRegion(Region r);

    /* Fill the given region. */
    //public native void fillRegion(Region r);
}
