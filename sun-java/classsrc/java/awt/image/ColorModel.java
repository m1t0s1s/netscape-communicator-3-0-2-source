/*
 * @(#)ColorModel.java	1.8 95/09/08 Jim Graham
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

/**
 * A class that encapsulates the methods for translating from pixel values
 * to alpha, red, green, and blue color components for an image.  This
 * class is abstract.
 *
 * @see IndexColorModel
 * @see DirectColorModel
 *
 * @version	1.8 08 Sep 1995
 * @author 	Jim Graham
 */
public abstract class ColorModel {
    protected int pixel_bits;

    private static ColorModel RGBdefault;

    /**
     * Return a ColorModel which describes the default format for
     * integer RGB values used throughout the AWT image interfaces.
     * The format for the RGB values is an integer with 8 bits
     * each of alpha, red, green, and blue color components ordered
     * correspondingly from the most significant byte to the least
     * significant byte, as in:  0xAARRGGBB
     */
    public static ColorModel getRGBdefault() {
	if (RGBdefault == null) {
	    RGBdefault = new DirectColorModel(32,
					      0x00ff0000,	// Red
					      0x0000ff00,	// Green
					      0x000000ff,	// Blue
					      0xff000000	// Alpha
					      );
	}
	return RGBdefault;
    }

    /**
     * Construct a ColorModel which describes a pixel of the specified
     * number of bits.
     */
    public ColorModel(int bits) {
	pixel_bits = bits;
    }

    /**
     * Return the number of bits per pixel described by this ColorModel.
     */
    public int getPixelSize() {
	return pixel_bits;
    }

    /**
     * The subclass must provide a function which provides the red
     * color compoment for the specified pixel.
     * @return		The red color component ranging from 0 to 255
     */
    public abstract int getRed(int pixel);

    /**
     * The subclass must provide a function which provides the green
     * color compoment for the specified pixel.
     * @return		The green color component ranging from 0 to 255
     */
    public abstract int getGreen(int pixel);

    /**
     * The subclass must provide a function which provides the blue
     * color compoment for the specified pixel.
     * @return		The blue color component ranging from 0 to 255
     */
    public abstract int getBlue(int pixel);

    /**
     * The subclass must provide a function which provides the alpha
     * color compoment for the specified pixel.
     * @return		The alpha transparency value ranging from 0 to 255
     */
    public abstract int getAlpha(int pixel);

    /**
     * Return the color of the pixel in the default RGB color model.
     * @see ColorModel#getRGBdefault
     */
    public int getRGB(int pixel) {
	return (getAlpha(pixel) << 24)
	    | (getRed(pixel) << 16)
	    | (getGreen(pixel) << 8)
	    | (getBlue(pixel) << 0);
    }
}
