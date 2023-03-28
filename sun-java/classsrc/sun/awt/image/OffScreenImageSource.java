/*
 * @(#)OffScreenImageSource.java	1.8 95/09/18 Jim Graham
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

import java.util.Hashtable;
import java.awt.image.ImageConsumer;
import java.awt.image.ImageProducer;
import java.awt.image.ColorModel;

public class OffScreenImageSource implements ImageProducer {
    int width;
    int height;
    ImageRepresentation baseIR;

    public OffScreenImageSource(int w, int h) {
	width = w;
	height = h;
    }

    void setImageRep(ImageRepresentation ir) {
	baseIR = ir;
    }

    public ImageRepresentation getImageRep() {
	return baseIR;
    }

    // We can only have one consumer since we immediately return the data...
    private ImageConsumer theConsumer;

    public synchronized void addConsumer(ImageConsumer ic) {
	theConsumer = ic;
	produce();
    }

    public synchronized boolean isConsumer(ImageConsumer ic) {
	return (ic == theConsumer);
    }

    public synchronized void removeConsumer(ImageConsumer ic) {
	if (theConsumer == ic) {
	    theConsumer = null;
	}
    }

    public void startProduction(ImageConsumer ic) {
	addConsumer(ic);
    }

    public void requestTopDownLeftRightResend(ImageConsumer ic) {
    }

    private native void sendPixels();

    private void produce() {
	if (theConsumer != null) {
	    theConsumer.setDimensions(width, height);
	}
	if (theConsumer != null) {
	    theConsumer.setProperties(new Hashtable());
	}
	sendPixels();
	if (theConsumer != null) {
	    theConsumer.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
	}
    }
}
