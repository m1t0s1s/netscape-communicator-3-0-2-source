/*
 * @(#)DirectColorModel.java	1.11 95/11/24 Jim Graham
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

package java.awt.image;

import java.awt.AWTException;

/**
 * A ColorModel class that specifies a translation from pixel values
 * to alpha, red, green, and blue color components for pixels which
 * have the color components embedded directly in the bits of the
 * pixel itself.  This color model is similar to an X11 TrueColor
 * visual.
 * <p>Many of the methods in this class are final.  The reason for
 * this is that the underlying native graphics code makes assumptions
 * about the layout and operation of this class and those assumptions
 * are reflected in the implementations of the methods here that are
 * marked final.  You can subclass this class for other reaons, but
 * you cannot override or modify the behaviour of those methods.
 *
 * @see ColorModel
 *
 * @version	1.11 24 Nov 1995
 * @author 	Jim Graham
 */
public class DirectColorModel extends ColorModel {
    private int red_mask;
    private int green_mask;
    private int blue_mask;
    private int alpha_mask;
    private int red_offset;
    private int green_offset;
    private int blue_offset;
    private int alpha_offset;
    private int red_bits;
    private int green_bits;
    private int blue_bits;
    private int alpha_bits;

    /**
     * Construct a DirectColorModel from the given masks specifying
     * which bits in the pixel contain the red, green and blue color
     * components.  Pixels described by this color model will all
     * have alpha components of 255 (fully opaque).  All of the bits
     * in each mask must be contiguous and fit in the specified number
     * of least significant bits of the integer.
     */
    public DirectColorModel(int bits,
			    int rmask, int gmask, int bmask) {
	this(bits, rmask, gmask, bmask, 0);
    }

    /**
     * Construct a DirectColorModel from the given masks specifying
     * which bits in the pixel contain the alhpa, red, green and blue
     * color components.  All of the bits in each mask must be contiguous
     * and fit in the specified number of least significant bits of the
     * integer.
     */
    public DirectColorModel(int bits,
			    int rmask, int gmask, int bmask, int amask) {
	super(bits);
	red_mask = rmask;
	green_mask = gmask;
	blue_mask = bmask;
	alpha_mask = amask;
	CalculateOffsets();
    }

    /**
     * Return the mask indicating which bits in a pixel contain the red
     * color component.
     */
    final public int getRedMask() {
	return red_mask;
    }

    /**
     * Return the mask indicating which bits in a pixel contain the green
     * color component.
     */
    final public int getGreenMask() {
	return green_mask;
    }

    /**
     * Return the mask indicating which bits in a pixel contain the blue
     * color component.
     */
    final public int getBlueMask() {
	return blue_mask;
    }

    /**
     * Return the mask indicating which bits in a pixel contain the alpha
     * transparency component.
     */
    final public int getAlphaMask() {
	return alpha_mask;
    }

    private int accum_mask = 0;

    /*
     * A utility function to decompose a single mask and verify that it
     * fits in the specified pixel size, and that it does not overlap any
     * other color component.  The values necessary to decompose and
     * manipulate pixels are calculated as a side effect.
     */
    private void DecomposeMask(int mask, String componentName, int values[]) {
	if ((mask & accum_mask) != 0) {
	    throw new IllegalArgumentException(componentName + " mask bits not unique");
	}
	int off = 0;
	int count = 0;
	if (mask != 0) {
	    while ((mask & 1) == 0) {
		mask >>>= 1;
		off++;
	    }
	    while ((mask & 1) == 1) {
		mask >>>= 1;
		count++;
	    }
	}
	if (mask != 0) {
	    throw new IllegalArgumentException(componentName + " mask bits not contiguous");
	}
	if (off + count > pixel_bits) {
	    throw new IllegalArgumentException(componentName + " mask overflows pixel");
	}
	values[0] = off;
	values[1] = count;
    }

    /*
     * A utility function to verify all of the masks and to store
     * the auxilliary values needed to manipulate the pixels.
     */
    private void CalculateOffsets() {
	int values[] = new int[2];
	DecomposeMask(red_mask, "red", values);
	red_offset = values[0];
	red_bits = values[1];
	DecomposeMask(green_mask, "green", values);
	green_offset = values[0];
	green_bits = values[1];
	DecomposeMask(blue_mask, "blue", values);
	blue_offset = values[0];
	blue_bits = values[1];
	DecomposeMask(alpha_mask, "alpha", values);
	alpha_offset = values[0];
	alpha_bits = values[1];
    }

    /**
     * Return the red color compoment for the specified pixel in the
     * range 0-255.
     */
    final public int getRed(int pixel) {
	if (red_mask == 0) return 0;
	int r = ((pixel & red_mask) >>> red_offset);
	return (red_bits == 8) ? r : (r * 255 / ((1 << red_bits) - 1));
    }

    /**
     * Return the green color compoment for the specified pixel in the
     * range 0-255.
     */
    final public int getGreen(int pixel) {
	if (green_mask == 0) return 0;
	int g = ((pixel & green_mask) >>> green_offset);
	return (green_bits == 8) ? g : (g * 255 / ((1 << green_bits) - 1));
    }

    /**
     * Return the blue color compoment for the specified pixel in the
     * range 0-255.
     */
    final public int getBlue(int pixel) {
	if (blue_mask == 0) return 0;
	int b = ((pixel & blue_mask) >>> blue_offset);
	return (blue_bits == 8) ? b : (b * 255 / ((1 << blue_bits) - 1));
    }

    /**
     * Return the alpha transparency value for the specified pixel in the
     * range 0-255.
     */
    final public int getAlpha(int pixel) {
	if (alpha_mask == 0) return 255;
	int a = ((pixel & alpha_mask) >>> alpha_offset);
	return (alpha_bits == 8) ? a : (a * 255 / ((1 << alpha_bits) - 1));
    }

    /**
     * Return the color of the pixel in the default RGB color model.
     * @see ColorModel#getRGBdefault
     */
    final public int getRGB(int pixel) {
	return (getAlpha(pixel) << 24)
	    | (getRed(pixel) << 16)
	    | (getGreen(pixel) << 8)
	    | (getBlue(pixel) << 0);
    }
}
