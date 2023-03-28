/*
 * @(#)ImageFetcher.java	1.10 95/09/27 Jim Graham
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
import java.lang.SecurityManager;

class ImageFetcher extends Thread {
    static Thread fetchers[] = null;

    static final int HIGH_PRIORITY = 8;
    static final int LOW_PRIORITY = 3;

    static {
	String 	imageFetcherCountString;
	int	imageFetcherCount;
	
	SecurityManager.setScopePermission();
	imageFetcherCountString = System.getProperty("awt.imagefetchers", "4");
	SecurityManager.resetScopePermission();

	imageFetcherCount = Integer.parseInt(imageFetcherCountString);

	SecurityManager.setScopePermission();
	fetchers = new Thread[imageFetcherCount];
	SecurityManager.resetScopePermission();
	
	for (int i = 0; i < fetchers.length; i++) {
	    SecurityManager.setScopePermission();
	    Thread t = fetchers[i] = new ImageFetcher();
	    t.setName(t.getName() + " " +i);
	    t.setDaemon(true);
	    SecurityManager.resetScopePermission();
	    t.start();
	}
    }

    private static Vector waitList = new Vector();

    private static ThreadGroup getImageFetcherThreadGroup() {
	ThreadGroup g = currentThread().getThreadGroup();
	while ((g.getParent() != null) && (g.getParent().getParent() != null)) {
	    g = g.getParent();
	}
	return g;
    }

    private ImageFetcher() {
	super(getImageFetcherThreadGroup(), "Image Fetcher");
    }

    public static void add(ImageFetchable src) {
	synchronized(waitList) {
	    if (!waitList.contains(src)) {
		waitList.addElement(src);
		waitList.notify();
	    }
	}
    }

    public static boolean isFetcher(Thread t) {
	for (int i = 0; i < fetchers.length; i++) {
	    if (fetchers[i] == t) {
		return true;
	    }
	}
	return false;
    }

    public static boolean amFetcher() {
	return isFetcher(Thread.currentThread());
    }

    private static ImageFetchable nextImage() {
	synchronized(waitList) {
	    ImageFetchable src = null;
	    while (src == null) {
		while (waitList.size() == 0) {
		    try {
			waitList.wait();
		    } catch (InterruptedException e) {
			System.err.println("Image Fetcher interrupted!");
		    }
		}
		src = (ImageFetchable) waitList.elementAt(0);
		waitList.removeElement(src);
	    }
	    return src;
	}
    }

    public void run() {
	while (true) {
	    SecurityManager.setScopePermission();
	    Thread.currentThread().setPriority(HIGH_PRIORITY);
	    SecurityManager.resetScopePermission();
	    ImageFetchable src = nextImage();
	    try {
		src.doFetch();
		src = null;
	    } catch (Exception e) {
		System.err.println("Uncaught error fetching image:");
		e.printStackTrace();
	    }
	}
    }
}
