/*
 * @(#)JPEGImageDecoder.java	1.6 95/11/28 Jim Graham
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

/*-
 *	Reads JPEG images from an InputStream and reports the
 *	image data to an InputStreamImageSource object.
 *
 * The native implementation of the JPEG image decoder was adapted from
 * release 6 of the free JPEG software from the Independent JPEG Group.
 */
package sun.awt.image;

import java.util.Vector;
import java.util.Hashtable;
import java.io.InputStream;
import java.io.IOException;
import java.awt.image.*;
import java.lang.SecurityManager;

/**
 * JPEG Image converter
 * 
 * @version 1.6 28 Nov 1995
 * @author Jim Graham
 */
public class JPEGImageDecoder extends ImageDecoder {
    private static ColorModel RGBcolormodel;
    private static ColorModel Graycolormodel;

    static {
	SecurityManager.setScopePermission();
	System.loadLibrary("jpeg");
	SecurityManager.resetScopePermission();
	RGBcolormodel = new DirectColorModel(24, 0xff0000, 0xff00, 0xff);
	byte g[] = new byte[256];
	for (int i = 0; i < 256; i++) {
	    g[i] = (byte) i;
	}
	Graycolormodel = new IndexColorModel(8, 256, g, g, g);
    }

    private native void readImage(InputStream is, byte buf[])
	throws ImageFormatException;

    PixelStore store;
    Hashtable props = new Hashtable();

    public JPEGImageDecoder(InputStreamImageSource src, InputStream is) {
	super(src, is);
    }

    public synchronized boolean catchupConsumer(InputStreamImageSource src,
						ImageConsumer ic)
    {
	return ((store == null) || store.replay(src, ic));
    }

    public synchronized void makeStore(int width, int height, boolean gray) {
	if (gray) {
	    store = new PixelStore8(width, height);
	} else {
	    store = new PixelStore32(width, height);
	}
    }

    /**
     * An error has occurred. Throw an exception.
     */
    private static void error(String s1) throws ImageFormatException {
	throw new ImageFormatException(s1);
    }

    public boolean sendHeaderInfo(int width, int height,
				  boolean gray, boolean multipass) {
	source.setDimensions(width, height);
	makeStore(width, height, gray);

	source.setProperties(props);
	store.setProperties(props);

	ColorModel colormodel = gray ? Graycolormodel : RGBcolormodel;
	source.setColorModel(colormodel);
	store.setColorModel(colormodel);

	int flags = hintflags;
	if (!multipass) {
	    flags |= ImageConsumer.SINGLEPASS;
	}
	source.setHints(hintflags);
	store.setHints(hintflags);

	return true;
    }

    public boolean sendPixels(int pixels[], int y) {
	int count = source.setPixels(0, y, pixels.length, 1, RGBcolormodel,
				     pixels, 0, pixels.length);
	if (store.setPixels(0, y, pixels.length, 1,
			    pixels, 0, pixels.length)) {
	    count++;
	}
	return (count > 0);
    }

    public boolean sendPixels(byte pixels[], int y) {
	int count = source.setPixels(0, y, pixels.length, 1, Graycolormodel,
				     pixels, 0, pixels.length);
	if (store.setPixels(0, y, pixels.length, 1,
			    pixels, 0, pixels.length)) {
	    count++;
	}
	return (count > 0);
    }

    /**
     * produce an image from the stream.
     */
    public void produceImage() throws IOException, ImageFormatException {
	try {
	    readImage(input, new byte[1024]);
	    store.imageComplete();
	    if (store.getBitState() != PixelStore.BITS_LOST) {
		source.setPixelStore(store);
	    }
	    source.imageComplete(ImageConsumer.STATICIMAGEDONE);
	} finally {
	    try {
		input.close();
	    } catch (IOException e) {
	    }
	}
    }

    /**
     * The ImageConsumer hints flag for a JPEG image.
     */
    private static final int hintflags =
	ImageConsumer.TOPDOWNLEFTRIGHT | ImageConsumer.COMPLETESCANLINES |
	ImageConsumer.SINGLEFRAME;
}
