/*
 * @(#)InputStreamImageSource.java	1.28 95/12/09 Jim Graham
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

import java.awt.image.*;
import java.io.InputStream;
import java.io.IOException;
import java.io.BufferedInputStream;
import java.util.Hashtable;
import java.lang.SecurityManager;

abstract class InputStreamImageSource implements ImageProducer, ImageFetchable {
    PixelStore	pixelstore;

    private ConsumerQueue consumers;

    private boolean awaitingFetch = false;
    private Thread latestFetcher;
    private ImageDecoder decoder;

    abstract boolean checkSecurity(Object context, boolean quiet);

    synchronized int countConsumers() {
	ConsumerQueue cq = consumers;
	int i = 0;
	while (cq != null) {
	    i++;
	    cq = cq.next;
	}
	return i;
    }

    public void addConsumer(ImageConsumer ic) {
	addConsumer(ic, false);
    }

    synchronized void addConsumer(ImageConsumer ic, boolean produce) {
	checkSecurity(null, false);
	ConsumerQueue cq = consumers;
	while (cq != null && cq.consumer != ic) {
	    cq = cq.next;
	}
	if (cq == null) {
	    cq = new ConsumerQueue(this, ic);
	    cq.next = consumers;
	    consumers = cq;
	} else {
	    if (!cq.secure) {
		Object context = System.getSecurityManager().getSecurityContext();
		if (cq.securityContext == null) {
		    cq.securityContext = context;
		} else if (cq.securityContext != context) {
		    // If there are two different security contexts that both
		    // have a handle on the same ImageConsumer, then there has
		    // been a security breach and whether or not they trade
		    // image data is small fish compared to what they could be
		    // trading.  Throw a Security exception anyway...
		    errorConsumer(cq);
		    throw new SecurityException("Applets are trading image data!");
		}
	    }
	    cq.interested = true;
	}
	if (produce && cq.feeder == null &&
	    !awaitingFetch && latestFetcher == null)
	{
	    startProduction();
	}
    }

    public synchronized boolean isConsumer(ImageConsumer ic) {
	ConsumerQueue cq = consumers;
	while (cq != null) {
	    if (cq.consumer == ic) {
		return true;
	    }
	    cq = cq.next;
	}
	return false;
    }

    synchronized void errorConsumer(ConsumerQueue cq) {
	cq.consumer.imageComplete(ImageConsumer.IMAGEERROR);
	removeConsumer(cq.consumer);
    }

    public synchronized void removeConsumer(ImageConsumer ic) {
	ConsumerQueue cq = consumers;
	ConsumerQueue cqprev = null;
	while (cq != null) {
	    if (cq.consumer == ic) {
		if (cqprev == null) {
		    consumers = cq.next;
		} else {
		    cqprev.next = cq.next;
		}
		cq.interested = false;
		break;
	    }
	    cqprev = cq;
	    cq = cq.next;
	}
    }

    public void startProduction(ImageConsumer ic) {
	addConsumer(ic, true);
    }

    private synchronized void startProduction() {
	if (!awaitingFetch) {
	    ImageFetcher.add(this);
	    awaitingFetch = true;
	}
    }

    public void requestTopDownLeftRightResend(ImageConsumer ic) {
	PixelStore store;
	synchronized(this) {
	    store = pixelstore;
	}
	if (store != null) {
	    store.replay(this, ic, false);
	}
    }

    protected abstract ImageDecoder getDecoder();

    protected ImageDecoder decoderForType(InputStream is,
					  String content_type) {
	if (content_type.equals("image/gif")) {
	    return new GifImageDecoder(this, is);
	} else if (content_type.equals("image/jpeg")) {
	    return new JPEGImageDecoder(this, is);
	} else if (content_type.equals("image/x-xbitmap")) {
	    return new XbmImageDecoder(this, is);
	}
	/*
	else if (content_type == URL.content_jpeg) {
	    return new JpegImageDecoder(this, is);
	} else if (content_type == URL.content_xbitmap) {
	    return new XbmImageDecoder(this, is);
	} else if (content_type == URL.content_xpixmap) {
	    return new Xpm2ImageDecoder(this, is);
	}
	*/

	return null;
    }

    protected ImageDecoder getDecoder(InputStream is) {
	if (!is.markSupported())
	    is = new BufferedInputStream(is);
	try {
	    is.mark(1000);
	    int c1 = is.read();
	    int c2 = is.read();
	    int c3 = is.read();
	    int c4 = is.read();
	    int c5 = is.read();
	    int c6 = is.read();
	    is.reset();

	    if (c1 == 'G' && c2 == 'I' && c3 == 'F' && c4 == '8') {
		return new GifImageDecoder(this, is);
	    } else if ((c1 == '\377' && c2 == '\330'
			&& c3 == '\377' && c4 == '\340') ||
		       (c1 == '\377' && c2 == '\330'
			&& c3 == '\377' && c4 == '\356')) {
		return new JPEGImageDecoder(this, is);
	    } else if (c1 == '#' && c2 == 'd' && c3 == 'e' && c4 == 'f') {
		return new XbmImageDecoder(this, is);
//	    } else if (c1 == '!' && c2 == ' ' && c3 == 'X' && c4 == 'P' && 
//		       c5 == 'M' && c6 == '2') {
//		return new Xpm2ImageDecoder(this, is);
	    }
	} catch (IOException e) {
	}

	return null;
    }

    public void doFetch() {
	Thread me = Thread.currentThread();
	ImageDecoder imgd = null;
	try {
	    if (latchConsumers(me) > 0) {
		if (updateFromStore(me)) {
		    return;
		}
		imgd = getDecoder();
		// if (imgd == null), then finally clause will send an error.
		if (imgd != null) {
		    setDecoder(imgd, me);
		    try {
			imgd.produceImage();
		    } catch (IOException e) {
			e.printStackTrace();
			// the finally clause will send an error.
		    } catch (ImageFormatException e) {
			e.printStackTrace();
			// the finally clause will send an error.
		    }
		}
	    }
	} finally {
	    // Send an error to all remaining consumers.
	    // They should have removed themselves by now if all is OK.
	    imageComplete(ImageConsumer.IMAGEERROR);
	    unlatchConsumers(imgd, me);
	}
    }

    private boolean updateFromStore(Thread me) {
	PixelStore store;
	ConsumerQueue cq;
	synchronized(this) {
	    store = pixelstore;
	    if (store == null) {
		return false;
	    }
	    cq = consumers;
	}
	while (cq != null) {
	    synchronized(this) {
		if (cq.feeder == null) {
		    if (!checkSecurity(cq.securityContext, true)) {
			errorConsumer(cq);
		    } else {
			cq.feeder = me;
		    }
		}
		if (cq.feeder != me) {
		    cq = cq.next;
		    continue;
		}
	    }
	    if (!store.replay(this, cq.consumer)) {
		return false;
	    }
	    synchronized(this) {
		cq = cq.next;
	    }
	}
	return true;
    }

    private synchronized void setDecoder(ImageDecoder decoder, Thread me) {
	ConsumerQueue cq = consumers;
	while (cq != null) {
	    if (cq.feeder == me) {
		// Now that there is a decoder, security may have changed
		// so reverify it here, just in case.
		if (!checkSecurity(cq.securityContext, true)) {
		    errorConsumer(cq);
		} else {
		    cq.decoder = decoder;
		}
	    }
	    cq = cq.next;
	}
	if (latestFetcher == me) {
	    this.decoder = decoder;
	}
    }

    private synchronized int latchConsumers(Thread me) {
	latestFetcher = me;
	awaitingFetch = false;
	ConsumerQueue cq = consumers;
	int count = 0;
	while (cq != null) {
	    if (cq.feeder == null) {
		if (!checkSecurity(cq.securityContext, true)) {
		    errorConsumer(cq);
		} else {
		    cq.feeder = me;
		    count++;
		}
	    }
	    cq = cq.next;
	}
	return count;
    }

    private synchronized void unlatchConsumers(ImageDecoder imgd, Thread me) {
	// All consumers should have removed themselves by now.
	// In case any are remaining, we dump them on the floor.
	if (latestFetcher == me) {
	    latestFetcher = null;
	}
	ConsumerQueue cq = consumers;
	ConsumerQueue cqprev = null;
	while (cq != null) {
	    if (cq.feeder == me) {
		// Just let them drop now...
		if (cqprev == null) {
		    consumers = cq.next;
		} else {
		    cqprev.next = cq.next;
		}
	    } else {
		cqprev = cq;
	    }
	    cq = cq.next;
	}
    }

    private ConsumerQueue nextConsumer(ConsumerQueue cq, Thread me) {
	synchronized(this) {
	    if (cq == null) {
		cq = consumers;
	    } else {
		cq = cq.next;
	    }
	    while (cq != null) {
		if (!cq.interested) {
		    continue;
		}
		if (cq.feeder == me) {
		    return cq;
		}
		if (cq.feeder == null && latestFetcher == me) {
		    break;
		}
		cq = cq.next;
	    }
	}
	if (cq != null) {
	    // assert(cq.feeder == null);
	    // assert(latestFetcher == me);
	    if (decoder != null
		&& decoder.catchupConsumer(this, cq.consumer))
	    {
		synchronized(this) {
		    if (cq.interested) {
			if (!checkSecurity(cq.securityContext, true)) {
			    errorConsumer(cq);
			} else {
			    cq.feeder = me;
			    cq.decoder = decoder;
			    return cq;
			}
		    }
		}
		return nextConsumer(cq, me);
	    } else {
		// We couldn't catch the guy up, so unhook us and refetch...
		synchronized(this) {
		    latestFetcher = null;
		    startProduction();
		}
	    }
	}
	return cq;
    }

    synchronized void flush() {
	ConsumerQueue cq = consumers;
	consumers = null;
	while (cq != null) {
	    cq.interested = false;
	    try {
		if (cq.feeder != null) {
		    cq.feeder.interrupt();
		}
	    } catch (NoSuchMethodError t) {
		break;
	    }
	    if (cq.decoder != null) {
		cq.decoder.close();
	    }
	    cq = cq.next;
	}
	pixelstore = null;
    }

    synchronized void setPixelStore(PixelStore storage) {
	pixelstore = storage;
    }

    int setDimensions(int w, int h) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.setDimensions(w, h);
	    count++;
	}
	return count;
    }

    int setProperties(Hashtable props) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.setProperties(props);
	    count++;
	}
	return count;
    }

    int setColorModel(ColorModel model) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.setColorModel(model);
	    count++;
	}
	return count;
    }

    int setHints(int hints) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.setHints(hints);
	    count++;
	}
	return count;
    }

    int setPixels(int x, int y, int w, int h, ColorModel model,
		  byte pix[], int off, int scansize) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	if (me.getPriority() > ImageFetcher.LOW_PRIORITY) {
	    SecurityManager.setScopePermission();
	    me.setPriority(ImageFetcher.LOW_PRIORITY);
	    SecurityManager.resetScopePermission();
	    //me.yield();
	}
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.setPixels(x, y, w, h, model, pix, off, scansize);
	    count++;
	}
	return count;
    }

    int setPixels(int x, int y, int w, int h, ColorModel model,
		  int pix[], int off, int scansize) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	if (me.getPriority() > ImageFetcher.LOW_PRIORITY) {
	    SecurityManager.setScopePermission();
	    me.setPriority(ImageFetcher.LOW_PRIORITY);
	    SecurityManager.resetScopePermission();
	    //me.yield();
	}
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.setPixels(x, y, w, h, model, pix, off, scansize);
	    count++;
	}
	return count;
    }

    int imageComplete(int status) {
	ConsumerQueue cq = null;
	Thread me = Thread.currentThread();
	int count = 0;
	while ((cq = nextConsumer(cq, me)) != null) {
	    cq.consumer.imageComplete(status);
	    count++;
	}
	return count;
    }
}

class ConsumerQueue {
    ImageConsumer consumer;
    Thread feeder;
    ImageDecoder decoder;
    ConsumerQueue next;
    boolean interested;
    Object securityContext;
    boolean secure;

    ConsumerQueue(InputStreamImageSource src, ImageConsumer ic) {
	consumer = ic;
	interested = true;
	// ImageReps and InfoGrabbers do their own security at access time.
	if (ic instanceof ImageRepresentation) {
	    ImageRepresentation ir = (ImageRepresentation) ic;
	    if (ir.image.source != src) {
		throw new SecurityException("ImageRep added to wrong image source");
	    }
	    secure = true;
	} else if (ic instanceof ImageInfoGrabber) {
	    ImageInfoGrabber ig = (ImageInfoGrabber) ic;
	    if (ig.image.source != src) {
		throw new SecurityException("ImageRep added to wrong image source");
	    }
	    secure = true;
	} else {
	    securityContext = System.getSecurityManager().getSecurityContext();
	}
    }

    public String toString() {
	return "[" + consumer + " " + securityContext + "]";
    }
}
