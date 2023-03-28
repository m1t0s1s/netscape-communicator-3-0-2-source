/*
 * @(#)ImageRepresentation.java	1.39 95/12/08 Jim Graham
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

import java.awt.Color;
import java.awt.Graphics;
import java.awt.AWTException;
import java.awt.Rectangle;
import java.awt.image.ColorModel;
import java.awt.image.ImageConsumer;
import java.awt.image.ImageObserver;
import sun.awt.image.ImageWatched;
import java.util.Hashtable;

public class ImageRepresentation extends ImageWatched implements ImageConsumer {
    int pData;

    InputStreamImageSource src;
    Image image;
    int tag;

    int srcW;
    int srcH;
    int width;
    int height;
    int hints;

    int availinfo;

    boolean offscreen;

    Rectangle newbits;

    /**
     * Create an ImageRepresentation for the given Image scaled
     * to the given width and height and dithered or converted to
     * a ColorModel appropriate for the given image tag.
     */
    ImageRepresentation(Image im, int w, int h, int t) {
	image = im;
	tag = t;
	if (w == 0 || h == 0) {
	    throw new IllegalArgumentException();
	}
	if (w > 0) {
	    width = w;
	    availinfo |= ImageObserver.WIDTH;
	}
	if (h > 0) {
	    height = h;
	    availinfo |= ImageObserver.HEIGHT;
	}
	if (image.getSource() instanceof InputStreamImageSource) {
	    src = (InputStreamImageSource) image.getSource();
	}
    }

    /**
     * Initialize this ImageRepresentation object to act as the
     * destination drawable for this OffScreen Image.
     */
    native void offscreenInit();

    /* REMIND: Only used for Frame.setIcon - should use ImageWatcher instead */
    public synchronized void reconstruct(int flags) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	int missinginfo = flags & ~availinfo;
	if (missinginfo != 0) {
	    numWaiters++;
	    try {
		startProduction();
		missinginfo = flags & ~availinfo;
		while (missinginfo != 0) {
		    try {
			wait();
		    } catch (InterruptedException e) {
			Thread.currentThread().interrupt();
			return;
		    }
		    missinginfo = flags & ~availinfo;
		}
	    } finally {
		decrementWaiters();
	    }
	}
    }

    public synchronized void setDimensions(int w, int h) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	srcW = w;
	srcH = h;
	if ((availinfo & ImageObserver.WIDTH) == 0
	    && (availinfo & ImageObserver.HEIGHT) == 0)
	{
	    width = w;
	    height = h;
	} else if ((availinfo & ImageObserver.WIDTH) == 0) {
	    // Height is set, but width is defaulted.
	    width = w * height / h;
	} else if ((availinfo & ImageObserver.HEIGHT) == 0) {
	    // Width is set, but height is defaulted.
	    height = h * width / w;
	} else {
	    // Both were set, no need to do a notification.
	    return;
	}
	if (srcW <= 0 || srcH <= 0 || width <= 0 || height <= 0) {
	    imageComplete(ImageConsumer.IMAGEERROR);
	    return;
	}
	availinfo |= ImageObserver.WIDTH | ImageObserver.HEIGHT;
    }

    public void setProperties(Hashtable props) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
    }

    public void setColorModel(ColorModel model) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
    }

    public void setHints(int h) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	hints = h;
    }

    private native boolean setBytePixels(int x, int y, int w, int h,
					 ColorModel model,
					 byte pix[], int off, int scansize);

    public void setPixels(int x, int y, int w, int h,
				       ColorModel model,
				       byte pix[], int off, int scansize) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	boolean ok = false;
	int cx, cy, cw, ch;
	synchronized (this) {
	    if (newbits == null) {
		newbits = new Rectangle(0, 0, width, height);
	    }
	    if (setBytePixels(x, y, w, h, model, pix, off, scansize)) {
		availinfo |= ImageObserver.SOMEBITS;
		ok = true;
	    }
	    cx = newbits.x;
	    cy = newbits.y;
	    cw = newbits.width;
	    ch = newbits.height;
	}
	if (ok && !offscreen) {
	    newInfo(image, ImageObserver.SOMEBITS, cx, cy, cw, ch);
	}
    }

    private native boolean setIntPixels(int x, int y, int w, int h,
					ColorModel model,
					int pix[], int off, int scansize);

    public void setPixels(int x, int y, int w, int h,
				       ColorModel model,
				       int pix[], int off, int scansize) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	boolean ok = false;
	int cx, cy, cw, ch;
	synchronized (this) {
	    if (newbits == null) {
		newbits = new Rectangle(0, 0, width, height);
	    }
	    if (setIntPixels(x, y, w, h, model, pix, off, scansize)) {
		availinfo |= ImageObserver.SOMEBITS;
		ok = true;
	    }
	    cx = newbits.x;
	    cy = newbits.y;
	    cw = newbits.width;
	    ch = newbits.height;
	}
	if (ok && !offscreen) {
	    newInfo(image, ImageObserver.SOMEBITS, cx, cy, cw, ch);
	}
    }

    private boolean consuming = false;

    private native boolean finish(boolean force);

    public void imageComplete(int status) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	boolean done;
	int info;
	switch (status) {
	default:
	case ImageConsumer.IMAGEABORTED:
	    done = true;
	    info = ImageObserver.ABORT;
	    break;
	case ImageConsumer.IMAGEERROR:
	    image.addInfo(ImageObserver.ERROR);
	    done = true;
	    info = ImageObserver.ERROR;
	    dispose();
	    break;
	case ImageConsumer.STATICIMAGEDONE:
	    done = true;
	    info = ImageObserver.ALLBITS;
	    if (finish(false)) {
		image.getSource().requestTopDownLeftRightResend(this);
		finish(true);
	    }
	    break;
	case ImageConsumer.SINGLEFRAMEDONE:
	    done = false;
	    info = ImageObserver.FRAMEBITS;
	    break;
	}
	synchronized (this) {
	    if (done) {
		image.getSource().removeConsumer(this);
		consuming = false;
		newbits = null;
	    }
	    availinfo |= info;
	    notifyAll();
	}
	if (!offscreen) {
	    newInfo(image, info, 0, 0, width, height);
	}
    }

    /*synchronized*/ void startProduction() {
	if (!consuming) {
	    consuming = true;
	    image.getSource().startProduction(this);
	}
    }

    private int numWaiters;

    private synchronized void checkConsumption() {
	if (watchers == null && numWaiters == 0
	    && ((availinfo & ImageObserver.ALLBITS) == 0))
	{
	    dispose();
	}
    }

    public synchronized void removeWatcher(ImageObserver iw) {
	super.removeWatcher(iw);
	checkConsumption();
    }

    private synchronized void decrementWaiters() {
	--numWaiters;
	checkConsumption();
    }

    public boolean prepare(ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.ERROR) != 0) {
	    if (iw != null) {
		iw.imageUpdate(image, ImageObserver.ERROR|ImageObserver.ABORT,
			       -1, -1, -1, -1);
	    }
	    return false;
	}
	boolean done = ((availinfo & ImageObserver.ALLBITS) != 0);
	if (!done) {
	    addWatcher(iw);
	    startProduction();
	}
	return done;
    }

    public int check(ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & (ImageObserver.ERROR | ImageObserver.ALLBITS)) != 0) {
	    addWatcher(iw);
	}
	return availinfo;
    }

    native synchronized void imageDraw(Graphics g, int x, int y, Color c);

    public boolean drawImage(Graphics g, int x, int y, Color c,
			     ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.ERROR) != 0) {
	    if (iw != null) {
		iw.imageUpdate(image, ImageObserver.ERROR|ImageObserver.ABORT,
			       -1, -1, -1, -1);
	    }
	    return false;
	}
	boolean done = ((availinfo & ImageObserver.ALLBITS) != 0);
	if (!done) {
	    addWatcher(iw);
	    startProduction();
	}
	imageDraw(g, x, y, c);
	return done;
    }

    private native void disposeImage();

    synchronized void abort() {
	image.getSource().removeConsumer(this);
	consuming = false;
	newbits = null;
	disposeImage();
	newInfo(image, ImageObserver.ABORT, -1, -1, -1, -1);
	availinfo &= ~(ImageObserver.SOMEBITS
		       | ImageObserver.FRAMEBITS
		       | ImageObserver.ALLBITS
		       | ImageObserver.ERROR);
    }

    synchronized void dispose() {
	image.getSource().removeConsumer(this);
	consuming = false;
	newbits = null;
	disposeImage();
	availinfo &= ~(ImageObserver.SOMEBITS
		       | ImageObserver.FRAMEBITS
		       | ImageObserver.ALLBITS);
    }

    public void finalize() {
	disposeImage();
    }
}
