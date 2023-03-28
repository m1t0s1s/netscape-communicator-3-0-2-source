/*
 * @(#)ImageDecoder.java	1.7 95/08/14 Jim Graham
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

package sun.awt.image;

import java.util.Vector;
import java.io.InputStream;
import java.io.IOException;
import java.awt.image.*;

public abstract class ImageDecoder {
    InputStreamImageSource source;
    InputStream input;

    public ImageDecoder(InputStreamImageSource src, InputStream is) {
	source = src;
	input = is;
    }

    public abstract boolean catchupConsumer(InputStreamImageSource src,
					    ImageConsumer ic);
    public abstract void produceImage() throws IOException,
					       ImageFormatException;

    public synchronized void close() {
	if (input != null) {
	    try {
		input.close();
	    } catch (IOException e) {
	    }
	    input = null;
	}
    }
}
