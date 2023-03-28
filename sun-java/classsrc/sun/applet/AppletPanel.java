/*
 * @(#)AppletPanel.java	1.12 95/11/13
 *
 * Copyright (c) 1994-1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.applet;

import java.util.*;
import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import java.awt.*;
import java.applet.*;
import java.awt.image.MemoryImageSource;
import java.awt.image.ColorModel;
import java.lang.SecurityManager;


/**
 * Applet panel class. The panel manages and manipulates the
 * applet as it is being loaded. It forks a seperate thread in a new
 * thread group to call the applet's init(), start(), stop(), and
 * destroy() methods.
 *
 * @version 	1.12, 13 Nov 1995
 * @author 	Arthur van Hoff
 */
public abstract class AppletPanel extends Panel implements AppletStub, Runnable {
    /**
     * The applet (if loaded).
     */
    Applet applet;

    /**
     * The classloader for the applet.
     */
    AppletClassLoader loader;

    /**
     * The current status. Either: APPLET_DISPOSE, APPLET_LOAD, 
     * APPLET_INIT, APPLET_START.
     */
    int status;

    /**
     * The thread for the applet.
     */
    Thread handler;

    /* applet event ids */
    public final static int APPLET_DISPOSE = 0;
    public final static int APPLET_LOAD = 1;
    public final static int APPLET_INIT = 2;
    public final static int APPLET_START = 3;
    public final static int APPLET_STOP = 4;
    public final static int APPLET_DESTROY = 5;
    public final static int APPLET_QUIT = 6;

    /* send to the parent to force relayout */
    public final static int APPLET_RESIZE = 51234;

    /**
     * The applet size.
     */
    Dimension appletSize = new Dimension(100, 100);

    /**
     * Construct an applet viewer and start the applet.
     */
    public void init() {
	setLayout(new BorderLayout());

	// Get the width (if any)
	String att = getParameter("width");
	if (att != null) {
	    appletSize.width = Integer.valueOf(att).intValue();
	}

	// Get the height (if any)
	att = getParameter("height");
	if (att != null) {
	    appletSize.height = Integer.valueOf(att).intValue();
	}

	SecurityManager.setScopePermission();
	// Create a thread group for the applet, and start a new
        // thread to load the applet.
	String nm = "applet-" + getParameter("code");
	handler = new Thread(new AppletThreadGroup("group " + nm), this, "thread " + nm);
	SecurityManager.resetScopePermission();
	handler.start();
    }

    /**
     * Minimum size
     */
    public Dimension minimumSize() {
	return new Dimension(appletSize.width, appletSize.height);
    }

    /**
     * Preferred size
     */
    public Dimension preferredSize() {
	return minimumSize();
    }

    /**
     * Event Queue
     */
    Event queue = null;

    /**
     * Send an event.
     */
    public void sendEvent(int id) {
	//System.out.println("SEND= " + id);
	sendEvent(new Event(null, id, null));
    }

    /**
     * Send an event. Queue it for execution by the handler thread.
     */
    protected synchronized void sendEvent(Event evt) {
	if (queue == null) {
	    //System.out.println("SEND0= " + evt);
	    evt.target = queue;
	    queue = evt;
	    notifyAll();
	} else {
	    //System.out.println("SEND1= " + evt);
	    Event q = queue;
	    for (; q.target != null ; q = (Event)q.target);
	    q.target = evt;
	}
    }

    /**
     * Get an event from the queue.
     */
    synchronized Event getNextEvent() throws InterruptedException {
	while (queue == null) {
	    wait();
	}
	Event evt = queue;
	queue = (Event)queue.target;
	evt.target = this;
	return evt;
    }

    /**
     * Execute applet events.
     */
    public void run() {
        // Bump up the priority, so that applet events are processed in
        // a timely manner.
        int priority = Thread.currentThread().getPriority();
	SecurityManager.setScopePermission();
        Thread.currentThread().setPriority(priority + 1);
	SecurityManager.resetScopePermission();

	while (true) {
	    Event evt;
	    try {
		evt = getNextEvent();
	    } catch (InterruptedException e) {
		showAppletStatus("bailing out...");
		return;
	    }

	    //showAppletStatus("EVENT = " + evt.id);
	    try {
		switch (evt.id) {
		  case APPLET_LOAD:
		    if (status != APPLET_DISPOSE) {
			showAppletStatus("load: not disposed");
			break;
		    }
		    // Create a class loader
		    loader = getClassLoader(getCodeBase());

		    // Create the applet using the class loader
		    String code = getParameter("code");
		    if (code.endsWith(".class")) {
			code = code.substring(0, code.length() - 6).replace('/', '.');
		    }
		    if (code.endsWith(".java")) {
			code = code.substring(0, code.length() - 5).replace('/', '.');
		    }
		    try {
			applet = (Applet)loader.loadClass(code).newInstance();
		    } catch (ClassNotFoundException e) {
			showAppletStatus("load: class " + code + " not found");
			showAppletLog("load: class " + code + " not found");
			showAppletException(e);
			break;
		    } catch (InstantiationException e) {
			showAppletStatus("load: " + code + " can't be instantiated");
			showAppletLog("load: " + code + " can't be instantiated");
			showAppletException(e);
			break;
		    } catch (IllegalAccessException e) {
			showAppletStatus("load: " + code + " is not public or has no public constructor");
			showAppletLog("load: " + code + " is not public or has no public constructor");
			showAppletException(e);
		    } catch (Exception e) {
			showAppletStatus("exception: " + e.getMessage());
			showAppletException(e);
			break;
		    } catch (ThreadDeath e) {
			showAppletStatus("killed");
			return;
		    } catch (Error e) {
			showAppletStatus("error: " + e.getMessage());
			showAppletException(e);
			break;
		    }
		
		    // Stick it in the frame
		    applet.setStub(this);
		    applet.hide();
		    add("Center", applet);
		    status = APPLET_LOAD;
		    showAppletStatus("applet loaded");
		    validate();
		    break;
		  
		  case APPLET_INIT:
		    if (status != APPLET_LOAD) {
			showAppletStatus("init: applet not loaded");
			break;
		    }
		    applet.resize(appletSize);
		    applet.init();
		    validate();
		    status = APPLET_INIT;
		    showAppletStatus("applet initialized");
		    break;

		  case APPLET_START:
		    if (status != APPLET_INIT) {
			showAppletStatus("start: applet not initialized");
			break;
		    }
		    applet.resize(appletSize);
		    applet.start();
		    applet.show();
		    validate();
		    status = APPLET_START;
		    showAppletStatus("applet started");
		    break;

		  case APPLET_STOP:
		    if (status != APPLET_START) {
			showAppletStatus("stop: applet not started");
			break;
		    }
		    status = APPLET_INIT;
		    applet.hide();
		    SecurityManager.setScopePermission();
		    applet.stop();
		    SecurityManager.resetScopePermission();
		    showAppletStatus("applet stopped");
		    break;

		  case APPLET_DESTROY:
		    if (status != APPLET_INIT) {
			showAppletStatus("destroy: applet not stopped");
			break;
		    }
		    status = APPLET_LOAD;
		    SecurityManager.setScopePermission();
		    applet.destroy();
		    SecurityManager.resetScopePermission();
		    showAppletStatus("applet destroyed");
		    break;

		  case APPLET_DISPOSE:
		    if (status != APPLET_LOAD) {
			showAppletStatus("dispose: applet not destroyed");
			break;
		    }
		    status = APPLET_DISPOSE;
		    remove(applet);
		    showAppletStatus("applet disposed");
		    break;

		  case APPLET_QUIT:
		    return;
		}
	    } catch (Exception e) {
		showAppletStatus("exception: " + e.getClass().getName() + ": " + e.getMessage());
		showAppletException(e);
	    } catch (ThreadDeath e) {
		showAppletStatus("killed");
		return;
	    } catch (Error e) {
		showAppletStatus("error: " + e.getClass().getName() + ": " + e.getMessage());
		showAppletException(e);
	    }
	}
    }

    /**
     * Return true when the applet has been started.
     */
    public boolean isActive() {
	return status == APPLET_START;
    }

    /**
     * Is called when the applet want to be resized.
     */
    public void appletResize(int width, int height) {
	appletSize.width = width;
	appletSize.height = height;
	postEvent(new Event(this, APPLET_RESIZE, preferredSize()));
    }

    /**
     * Status line. Called by the AppletPanel to provide
     * feedback on the Applet's state.
     */
    protected void showAppletStatus(String status) {
	//System.out.println("STATUS = " + status);
	getAppletContext().showStatus(status);
    }		

    /**
     * Called by the AppletPanel to print to the log.
     */
    protected void showAppletLog(String str) {
	System.out.println(str);
    }		

    /**
     * Called by the AppletPanel to provide
     * feedback when an exception has happend.
     */
    protected void showAppletException(Throwable t) {
	t.printStackTrace();
    }		

    /**
     * The class loaders
     */
    private static Hashtable classloaders = new Hashtable();
    
    /**
     * Flush the class loaders.
     */
    static synchronized void flushClassLoader(URL codebase) {
	classloaders.remove(codebase);
    }

    /**
     * Get a class loader.
     */
    static synchronized AppletClassLoader getClassLoader(URL codebase) {
	AppletClassLoader c = (AppletClassLoader)classloaders.get(codebase);
	if (c == null) {
	    c = new AppletClassLoader(codebase);
	    classloaders.put(codebase, c);
	}
	return c;
    }
}
