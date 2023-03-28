/*
 * @(#)GifImageDecoder.java	1.23 95/10/09 Patrick Naughton, Arthur van Hoff
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
 *	Reads GIF images from an InputStream and reports the
 *	image data to an InputStreamImageSource object.
 *
 * The algorithm is copyright of CompuServe.
 */
package sun.awt.image;

import java.util.Vector;
import java.util.Hashtable;
import java.io.InputStream;
import java.io.IOException;
import java.awt.image.*;

/**
 * Gif Image converter
 * 
 * @version 1.23 09 Oct 1995
 * @author Arthur van Hoff
 */
public class GifImageDecoder extends ImageDecoder {
    private final boolean verbose = false;

    private static final int IMAGESEP 		= 0x2c;
    private static final int EXBLOCK 		= 0x21;
    private static final int EX_GRAPHICS_CONTROL= 0xf9;
    private static final int EX_COMMENT 	= 0xfe;
    private static final int EX_APPLICATION 	= 0xff;
    private static final int TERMINATOR 	= 0x3b;
    private static final int INTERLACEMASK 	= 0x40;
    private static final int COLORMAPMASK 	= 0x80;

    PixelStore8 store;
    int num_colors;
    byte[] colormap;
    IndexColorModel model;
    Hashtable props = new Hashtable();

    public GifImageDecoder(InputStreamImageSource src, InputStream is) {
	super(src, is);
    }

    public synchronized boolean catchupConsumer(InputStreamImageSource src,
						ImageConsumer ic)
    {
	return ((store == null) || store.replay(src, ic));
    }

    public synchronized void makeStore(int width, int height) {
	store = new PixelStore8(width, height);
    }

    /**
     * An error has occurred. Throw an exception.
     */
    private static void error(String s1) throws ImageFormatException {
	throw new ImageFormatException(s1);
    }

    /**
     * Read a number of bytes into a buffer.
     */
    boolean readBytes(byte buf[], int off, int len) {
	while (len > 0) {
	    int n;
	    try {
		n = input.read(buf, off, len);
	    } catch (IOException e) {
		n = -1;
	    }
	    if (n < 0) {
		return false;
	    }
	    off += n;
	    len -= n;
	}
	return true;
    }

    /**
     * produce an image from the stream.
     */
    public void produceImage() throws IOException, ImageFormatException {
	int trans_pixel = -1;
	try {
	    readHeader();

	    while (true) {
		int code;

		switch (code = input.read()) {
		  case EXBLOCK:
		    switch (code = input.read()) {
		      case EX_GRAPHICS_CONTROL: {
			byte buf[] = new byte[6];
			if (!readBytes(buf, 0, 6)) {
			    return;//error("corrupt GIF file");
			}
			if ((buf[0] != 4) || (buf[5] != 0)) {
			    return;//error("corrupt GIF file (GCE size)");
			}
			// Get the index of the transparent color
			trans_pixel = buf[4] & 0xFF;
			break;
		      }

		      case EX_COMMENT:
		      case EX_APPLICATION:
		      default:
			String comment = "";
			while (true) {
			    int n = input.read();
			    if (n == 0) {
				break;
			    }
			    byte buf[] = new byte[n];
			    if (!readBytes(buf, 0, n)) {
				return;//error("corrupt GIF file");
			    }
			    if (code == EX_COMMENT) {
				comment += new String(buf, 0);
			    }
			}
			if (code == EX_COMMENT) {
			    props.put("comment", comment);
			}
			break;
		    }
		    break;

		  case IMAGESEP:
		    try {
			model = new IndexColorModel(8, num_colors, colormap,
						    0, false, trans_pixel);
			colormap = null;
			if (readImage()) {
			    store.imageComplete();
			    if (store.getBitState() != PixelStore.BITS_LOST) {
				source.setPixelStore(store);
			    }
			    source.imageComplete(ImageConsumer.STATICIMAGEDONE);
			}
		    } catch (ArrayIndexOutOfBoundsException e) {
			if (verbose) {
			    e.printStackTrace();
			}
		    }
		    return;

		  case TERMINATOR:
		    return;

		  case -1:
		    return;

		  default:
		    return;//error("corrupt GIF file (parse) [" + code + "].");
		    //break;
		}
	    }
	} finally {
	    try {
		input.close();
	    } catch (IOException e) {
	    }
	}
    }

    /**
     * Read Image header
     */
    private void readHeader() throws IOException, ImageFormatException {
	// Create a buffer
	byte buf[] = new byte[13];

	// Read the header
	if (!readBytes(buf, 0, 13)) {
	    throw new IOException();
	}

	// Check header
	if ((buf[0] != 'G') || (buf[1] != 'I') || (buf[2] != 'F')) {
	    error("not a GIF file.");
	}

	// colormap info
	int ch = buf[10] & 0xFF;
	if ((ch & COLORMAPMASK) == 0) {
	    error("no global colormap in GIF file.");
	}
	num_colors = 1 << ((ch & 0x7) + 1);

	// supposed to be NULL
	if (buf[12] != 0) {
	    error("corrupt GIF file (no null).");
	}

	// Read colors
	colormap = new byte[num_colors * 3];
	if (!readBytes(colormap, 0, num_colors * 3)) {
	    throw new IOException();
	}
    }

    /**
     * The ImageConsumer hints flag for a non-interlaced GIF image.
     */
    private static final int normalflags =
	ImageConsumer.TOPDOWNLEFTRIGHT | ImageConsumer.COMPLETESCANLINES |
	ImageConsumer.SINGLEPASS | ImageConsumer.SINGLEFRAME;

    /**
     * The ImageConsumer hints flag for an interlaced GIF image.
     */
    private static final int interlaceflags =
	ImageConsumer.RANDOMPIXELORDER | ImageConsumer.COMPLETESCANLINES |
	ImageConsumer.SINGLEPASS | ImageConsumer.SINGLEFRAME;

    private native boolean parseImage(int width, int height,
				      boolean interlace, int initCodeSize,
				      byte block[], byte rasline[]);

    private int sendPixels(int y, int width, byte rasline[]) {
	int count = source.setPixels(0, y, width, 1, model,
				     rasline, 0, width);
	if (store.setPixels(0, y, width, 1, rasline, 0, width)) {
	    count++;
	}
	return count;
    }

    /**
     * Read Image data
     */
    private boolean readImage() throws IOException {
	long tm = 0;

	if (verbose) {
	    tm = System.currentTimeMillis();
	}

	// Allocate the buffer
	byte block[] = new byte[256 + 3];

	// Read the image descriptor
	if (!readBytes(block, 0, 10)) {
	    throw new IOException();
	}
	int width = (block[4] & 0xFF) | (block[5] << 8);
	int height = (block[6] & 0xFF) | (block[7] << 8);
	boolean interlace = (block[8] & INTERLACEMASK) != 0;
	int initCodeSize = block[9] & 0xFF;

	// Notify the consumers
	source.setDimensions(width, height);
	makeStore(width, height);

	source.setProperties(props);
	store.setProperties(props);

	source.setColorModel(model);
	store.setColorModel(model);

	int hints = (interlace ? interlaceflags : normalflags);
	source.setHints(hints);
	store.setHints(hints);

	// allocate the raster data
	byte rasline[] = new byte[width];

	if (verbose) {
	    System.out.print("Reading a " + width + " by " + height + " " +
		      (interlace ? "" : "non-") + "interlaced image...");
	}

	boolean ret = parseImage(width, height, interlace, initCodeSize,
				 block, rasline);

	if (verbose) {
	    System.out.println("done in "
			       + (System.currentTimeMillis() - tm)
			       + "ms");
	}

	return ret;
    }
}
