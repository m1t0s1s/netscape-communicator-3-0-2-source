/*
 * @(#)ImageWatched.java	1.9 95/12/06 Jim Graham
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
import java.util.Enumeration;
import java.util.NoSuchElementException;
import java.awt.Image;
import java.awt.image.ImageObserver;

public class ImageWatched {
    public ImageWatched() {
    }

    protected Vector watchers;

    public synchronized void addWatcher(ImageObserver iw) {
	if (iw != null && !isWatcher(iw)) {
	    if (watchers == null) {
		watchers = new Vector();
	    }
	    watchers.addElement(iw);
	}
    }

    public synchronized boolean isWatcher(ImageObserver iw) {
	return (watchers != null && iw != null && watchers.contains(iw));
    }

    public synchronized void removeWatcher(ImageObserver iw) {
	if (iw != null && watchers != null) {
	    watchers.removeElement(iw);
	    if (watchers.size() == 0) {
		watchers = null;
	    }
	}
    }

    public void newInfo(Image img, int info, int x, int y, int w, int h) {
	if (watchers != null) {
	    Enumeration enum = watchers.elements();
	    Vector uninterested = null;
	    while (enum.hasMoreElements()) {
		ImageObserver iw;
		try {
		    iw = (ImageObserver) enum.nextElement();
		} catch (NoSuchElementException e) {
		    break;
		}
		if (!iw.imageUpdate(img, info, x, y, w, h)) {
		    if (uninterested == null) {
			uninterested = new Vector();
		    }
		    uninterested.addElement(iw);
		}
	    }
	    if (uninterested != null) {
		enum = uninterested.elements();
		while (enum.hasMoreElements()) {
		    ImageObserver iw = (ImageObserver) enum.nextElement();
		    removeWatcher(iw);
		}
	    }
	}
    }
}
