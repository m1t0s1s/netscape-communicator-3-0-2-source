/*
 * @(#)Image.java	1.39 95/12/08 Jim Graham
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
import java.util.Enumeration;

import java.awt.Graphics;
import java.awt.image.ColorModel;
import java.awt.image.ImageProducer;
import java.awt.image.ImageConsumer;
import java.awt.image.ImageObserver;
import sun.awt.image.OffScreenImageSource;
import sun.awt.image.ImageWatched;
import sun.awt.image.ImageRepresentation;
import sun.awt.image.FileImageSource;

public abstract class Image extends java.awt.Image {
    /**
     * The object which is used to reconstruct the original image data
     * as needed.
     */
    ImageProducer source;

    InputStreamImageSource src;

    /**
     * The object which intercepts the image dimension information
     * and redirects it into the image.
     */
    ImageInfoGrabber info;

    protected Image() {
    }

    /**
     * Construct an image for offscreen rendering to be used with a
     * given Component.
     */
    protected Image(int w, int h) {
	OffScreenImageSource osis = new OffScreenImageSource(w, h);
	source = osis;
	width = w;
	height = h;
	availinfo |= (ImageObserver.WIDTH | ImageObserver.HEIGHT);
	ImageRepresentation ir = getImageRep(-1, -1);
	representations.put(w + "x" + h, ir);
	representations.put("-1x-1", ir);
	ir.setDimensions(w, h);
	ir.offscreenInit();
	osis.setImageRep(ir);
    }

    /**
     * Construct an image from an ImageProducer object.
     */
    protected Image(ImageProducer is) {
	source = is;
	if (is instanceof InputStreamImageSource) {
	    src = (InputStreamImageSource) is;
	}
	info = new ImageInfoGrabber(this);
	info.setupConsumer();
    }

    public ImageProducer getSource() {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	return source;
    }

    private int width = -1;
    private int height = -1;
    private Hashtable properties;

    private int availinfo;

    /**
     * Return the width of the original image source.
     * If the width isn't known, then the image is reconstructed.
     */
    public int getWidth() {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.WIDTH) == 0) {
	    reconstruct(ImageObserver.WIDTH);
	}
	return width;
    }

    /**
     * Return the width of the original image source.
     * If the width isn't known, then the ImageObserver object will be
     * notified when the data is available.
     */
    public synchronized int getWidth(ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.WIDTH) == 0) {
	    addWatcher(iw, true);
	    return -1;
	}
	return width;
    }

    /**
     * Return the height of the original image source.
     * If the height isn't known, then the image is reconstructed.
     */
    public int getHeight() {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.HEIGHT) == 0) {
	    reconstruct(ImageObserver.HEIGHT);
	}
	return height;
    }

    /**
     * Return the height of the original image source.
     * If the height isn't known, then the ImageObserver object will be
     * notified when the data is available.
     */
    public synchronized int getHeight(ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.HEIGHT) == 0) {
	    addWatcher(iw, true);
	    return -1;
	}
	return height;
    }

    /**
     * Return a property of the image by name.  Individual property names
     * are defined by the various image formats.  If a property is not
     * defined for a particular image, then this method will return the
     * UndefinedProperty object.  If the properties for this image are
     * not yet known, then this method will return null and the ImageObserver
     * object will be notified later.  The property name "comment" should
     * be used to store an optional comment which can be presented to
     * the user as a description of the image, its source, or its author.
     */
    public Object getProperty(String name, ImageObserver observer) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if (properties == null) {
	    addWatcher(observer, true);
	    return null;
	}
	Object o = properties.get(name);
	if (o == null) {
	    o = Image.UndefinedProperty;
	}
	return o;
    }

    public boolean hasError() {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	return (availinfo & ImageObserver.ERROR) != 0;
    }

    public int check(ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.ERROR) == 0 &&
	    ((~availinfo) & (ImageObserver.WIDTH |
			     ImageObserver.HEIGHT |
			     ImageObserver.PROPERTIES)) != 0) {
	    addWatcher(iw, false);
	}
	return availinfo;
    }

    public void preload(ImageObserver iw) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if ((availinfo & ImageObserver.ALLBITS) == 0) {
	    addWatcher(iw, true);
	}
    }

    private synchronized void addWatcher(ImageObserver iw, boolean load) {
	if ((availinfo & ImageObserver.ERROR) != 0) {
	    if (iw != null) {
		iw.imageUpdate(this, ImageObserver.ERROR|ImageObserver.ABORT,
			       -1, -1, -1, -1);
	    }
	    return;
	}
	if (info == null) {
	    info = new ImageInfoGrabber(this);
	    info.setupConsumer();
	}
	info.addWatcher(iw);
	if (load) {
	    info.getInfo();
	}
    }

    private synchronized void reconstruct(int flags) {
	if ((flags & ~availinfo) != 0) {
	    if ((availinfo & ImageObserver.ERROR) != 0) {
		return;
	    }
	    if (info == null) {
		info = new ImageInfoGrabber(this);
	    }
	    info.getInfo();
	    while ((flags & ~availinfo) != 0) {
		try {
		    wait();
		} catch (InterruptedException e) {
		    Thread.currentThread().interrupt();
		    return;
		}
		if ((availinfo & ImageObserver.ERROR) != 0) {
		    return;
		}
	    }
	}
    }

    synchronized void addInfo(int newinfo) {
	availinfo |= newinfo;
	notifyAll();
	if (((~availinfo) & (ImageObserver.WIDTH |
			     ImageObserver.HEIGHT |
			     ImageObserver.PROPERTIES)) == 0) {
	    info.stopInfo();
	}
    }

    void setDimensions(int w, int h) {
	width = w;
	height = h;
	addInfo(ImageObserver.WIDTH | ImageObserver.HEIGHT);
    }

    void setProperties(Hashtable props) {
	if (props == null) {
	    props = new Hashtable();
	}
	properties = props;
	addInfo(ImageObserver.PROPERTIES);
    }

    synchronized void infoDone(int status) {
	if (status == ImageConsumer.IMAGEERROR ||
	    ((~availinfo) & (ImageObserver.WIDTH |
			     ImageObserver.HEIGHT)) != 0) {
	    addInfo(ImageObserver.ERROR);
	} else if ((availinfo & ImageObserver.PROPERTIES) == 0) {
	    setProperties(null);
	}
    }

    public void flush() {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	if (!(source instanceof OffScreenImageSource)) {
	    Enumeration enum;
	    synchronized (this) {
		availinfo &= ~ImageObserver.ERROR;
		enum = representations.elements();
		representations = new Hashtable();
		if (source instanceof InputStreamImageSource) {
		    ((InputStreamImageSource) source).flush();
		}
	    }
	    while (enum.hasMoreElements()) {
		ImageRepresentation ir =
		    (ImageRepresentation) enum.nextElement();
		ir.abort();
	    }
	}
    }

    Hashtable representations = new Hashtable();

    protected ImageRepresentation getImageRep(int w, int h) {
	if (src != null) {
	    src.checkSecurity(null, false);
	}
	String key = w + "x" + h;
	ImageRepresentation ir = (ImageRepresentation)representations.get(key);
	if (ir == null) {
	    // Now try it synchronized and add it if it isn't there
	    synchronized (this) {
		ir = (ImageRepresentation)representations.get(key);
		if (ir == null) {
		    ir = new ImageRepresentation(this, w, h, 0);
		    if (source instanceof OffScreenImageSource) {
			ir.offscreen = true;
		    } else {
			representations.put(key, ir);
		    }
		}
	    }
	}
	return ir;
    }
}

class ImageInfoGrabber extends ImageWatched implements ImageConsumer {
    Image image;

    public ImageInfoGrabber(Image img) {
	image = img;
    }

    public void setupConsumer() {
	image.getSource().addConsumer(this);
    }

    public void getInfo() {
	image.getSource().startProduction(this);
    }

    public void stopInfo() {
	image.getSource().removeConsumer(this);
	watchers = null;
    }

    public void setDimensions(int w, int h) {
	image.setDimensions(w, h);
	newInfo(image, (ImageObserver.WIDTH | ImageObserver.HEIGHT),
		0, 0, w, h);
    }

    public void setProperties(Hashtable props) {
	image.setProperties(props);
	newInfo(image, ImageObserver.PROPERTIES, 0, 0, 0, 0);
    }

    public void setColorModel(ColorModel model) {
	return;
    }

    public void setHints(int hints) {
	return;
    }

    public void setPixels(int srcX, int srcY, int srcW, int srcH,
			  ColorModel model,
			  byte pixels[], int srcOff, int srcScan) {
	return;
    }

    public void setPixels(int srcX, int srcY, int srcW, int srcH,
			  ColorModel model,
			  int pixels[], int srcOff, int srcScan) {
	return;
    }

    public void imageComplete(int status) {
	image.getSource().removeConsumer(this);
	if (status == ImageConsumer.IMAGEERROR) {
	    newInfo(image, ImageObserver.ERROR, -1, -1, -1, -1);
	} else if (status == ImageConsumer.IMAGEABORTED) {
	    newInfo(image, ImageObserver.ABORT, -1, -1, -1, -1);
	}
	image.infoDone(status);
    }
}
