/*
 * @(#)XbmImageDecoder.java	1.6 95/10/07 James Gosling
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
 *	Reads xbitmap format images into a DIBitmap structure.
 */
package sun.awt.image;

import java.io.*;
import java.awt.image.*;

/**
 * Parse files of the form:
 * 
 * #define foo_width w
 * #define foo_height h
 * static char foo_bits[] = {
 * 0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,
 * 0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,0xnn,
 * 0xnn,0xnn,0xnn,0xnn};
 * 
 * @version 1.6 07 Oct 1995
 * @author James Gosling
 */
public class XbmImageDecoder extends ImageDecoder {
    private static byte XbmColormap[] = {(byte) 255, (byte) 255, (byte) 255,
					 0, 0, 0};
    private static int XbmHints = (ImageConsumer.TOPDOWNLEFTRIGHT |
				   ImageConsumer.COMPLETESCANLINES |
				   ImageConsumer.SINGLEPASS |
				   ImageConsumer.SINGLEFRAME);

    public XbmImageDecoder(InputStreamImageSource src, InputStream is) {
	super(src, is);
    }

    PixelStore8 store;

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
     * produce an image from the stream.
     */
    public void produceImage() throws IOException, ImageFormatException {
	char nm[] = new char[80];
	int c;
	int i = 0;
	int state = 0;
	int H = 0;
	int W = 0;
	int x = 0;
	int y = 0;
	boolean start = true;
	byte raster[] = null;
	IndexColorModel model = null;
	while ((c = input.read()) != -1) {
	    if ('a' <= c && c <= 'z' ||
		    'A' <= c && c <= 'Z' ||
		    '0' <= c && c <= '9' || c == '#' || c == '_') {
		if (i < 78)
		    nm[i++] = (char) c;
	    } else if (i > 0) {
		int nc = i;
		i = 0;
		if (start) {
		    if (nc != 7 ||
			nm[0] != '#' ||
			nm[1] != 'd' ||
			nm[2] != 'e' ||
			nm[3] != 'f' ||
			nm[4] != 'i' ||
			nm[5] != 'n' ||
			nm[6] != 'e')
		    {
			error("Not an XBM file");
		    }
		    start = false;
		}
		if (nm[nc - 1] == 'h')
		    state = 1;	/* expecting width */
		else if (nm[nc - 1] == 't' && nc > 1 && nm[nc - 2] == 'h')
		    state = 2;	/* expecting height */
		else if (nc > 2 && state < 0 && nm[0] == '0' && nm[1] == 'x') {
		    int n = 0;
		    for (int p = 2; p < nc; p++) {
			c = nm[p];
			if ('0' <= c && c <= '9')
			    c = c - '0';
			else if ('A' <= c && c <= 'Z')
			    c = c - 'A' + 10;
			else if ('a' <= c && c <= 'z')
			    c = c - 'a' + 10;
			else
			    c = 0;
			n = n * 16 + c;
		    }
		    for (int mask = 1; mask <= 0x80; mask <<= 1) {
			if (x < W) {
			    if ((n & mask) != 0)
				raster[x] = 1;
			    else
				raster[x] = 0;
			}
			x++;
		    }
		    if (x >= W) {
			source.setPixels(0, y, W, 1, model, raster, 0, W);
			store.setPixels(0, y, W, 1, raster, 0, W);
			x = 0;
			if (y++ >= H) {
			    break;
			}
		    }
		} else {
		    int n = 0;
		    for (int p = 0; p < nc; p++)
			if ('0' <= (c = nm[p]) && c <= '9')
			    n = n * 10 + c - '0';
			else {
			    n = -1;
			    break;
			}
		    if (n > 0 && state > 0) {
			if (state == 1)
			    W = n;
			else
			    H = n;
			if (W == 0 || H == 0)
			    state = 0;
			else {
			    model = new IndexColorModel(8, 2, XbmColormap,
							0, false, 0);
			    source.setDimensions(W, H);
			    makeStore(W, H);
			    source.setColorModel(model);
			    store.setColorModel(model);
			    source.setHints(XbmHints);
			    store.setHints(XbmHints);
			    raster = new byte[W];
			    state = -1;
			}
		    }
		}
	    }
	}
	input.close();
	store.imageComplete();
	if (store.getBitState() != PixelStore.BITS_LOST) {
	    source.setPixelStore(store);
	}
	source.imageComplete(ImageConsumer.STATICIMAGEDONE);
    }
}
