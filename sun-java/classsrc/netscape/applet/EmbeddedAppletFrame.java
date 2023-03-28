/*
 * @(#)EmbeddedAppletFrame.java	1.8 95/09/26
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.applet;

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
 * Applet viewer class. The viewer manages and manipulates the
 * applet as it is being loaded. It forks a seperate thread in a new
 * thread group to call the applet's init(), start(), stop(), and
 * destroy() methods.
 *
 * @version 	1.8, 26 Sep 1995
 * @author 	Warren Harris, munged from Arthur van Hoff's AppletPanel
 */
class EmbeddedAppletFrame extends Frame implements AppletStub, Runnable {
    int pData;	// raw appletID for native code
    Integer appletID;

    MozillaAppletContext context;

    /**
     * The time the applet was last started or stopped (used for trimming).
     */
    long timestamp;
    boolean inHistory = false;

    /**
     * The document url.
     */
    URL documentURL;

    /**
     * The base url.
     */
    URL codebaseURL;

    /**
     * The zip file url.
     */
    URL archiveURL;

    /**
     * The attributes of the applet.
     */
    Hashtable atts;

    /**
     * The applet (if loaded).
     */
    Applet applet;

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
    final static int APPLET_DISPOSE = 0;
    final static int APPLET_LOAD = 1;
    final static int APPLET_INIT = 2;
    final static int APPLET_START = 3;
    final static int APPLET_STOP = 4;
    final static int APPLET_DESTROY = 5;

    /* send to the parent to force relayout */
    final static int APPLET_RESIZE = 51234;

    /**
     * The applet size.
     */
    Dimension appletSize = new Dimension(100, 100);

    AppletClassLoader classLoader;

    /* Object used to synchronize access to the event queue*/
    Object eventQueueLock = new Object();

    /**
     * Construct an applet viewer and start the applet.
     */
    EmbeddedAppletFrame(URL documentURL, URL codebaseURL, URL archiveURL,
                        Hashtable atts, MozillaAppletContext context,
                        Integer appletID) {
        this.appletID = appletID;
        this.pData = appletID.intValue();
        this.context = context;
        this.documentURL = documentURL;
        this.codebaseURL = codebaseURL;
        this.archiveURL = archiveURL;
        this.atts = atts;
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
    }

    /**
     * Clean up after an EmbeddedAppletFrame.
     */
    protected void finalize() {
        classLoader.releaseClassLoader();
        classLoader = null;
    }

    void dumpState(PrintStream out, int i) {
	if (false) {
	    MozillaAppletContext.indent(out, i);
	    out.println("EmbeddedAppletFrame id="+pData+" documentURL="+documentURL);
	    MozillaAppletContext.indent(out, i);
	    out.println("  codebaseURL="+codebaseURL+" status="+statusToString(status));
	    MozillaAppletContext.indent(out, i);
	    out.println("  handler="+handler);
	    for (Enumeration e = atts.keys(); e.hasMoreElements(); ) {
		String key = (String) e.nextElement();
		String value = (String) atts.get(key);
		MozillaAppletContext.indent(out, i+1);
		out.println(key + " = " + value);
	    }
	}
	else {
	    MozillaAppletContext.indent(out, i);
	    out.println("EmbeddedAppletFrame id="+pData+" applet="+applet);
	}
    }

    String statusToString(int status) {
        switch (status) {
          case APPLET_DISPOSE: return "dispose";
          case APPLET_LOAD: return "load";
          case APPLET_INIT: return "init";
          case APPLET_START: return "start";
          case APPLET_STOP: return "stop";
          case APPLET_DESTROY: return "destroy";
          default:
            return Integer.toString(status, 10);
        }
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
    void sendEvent(int id) {
        sendEvent(new Event(null, id, null));
    }

    /*
    ** We precreate the events needed for destroying the applet because
    ** we may need to destroy the applet in a low-memory situation.
    */
    static Event shutdownEvents =
        new Event(new Event(new Event(null, APPLET_DISPOSE, null),
                            APPLET_DESTROY,
                            null),
                  APPLET_STOP,
                  null);

    /**
     * Send an event. Queue it for execution by the handler thread.
     */
    void sendEvent(Event evt) {
        synchronized(eventQueueLock) {
            if (queue == null) {
                evt.target = queue;
                queue = evt;
                eventQueueLock.notifyAll();
            } else {
                Event q = queue;
                for (; q.target != null ; q = (Event)q.target);
                q.target = evt;
            }
        }
    }

    /**
     * Get an event from the queue.
     */
    Event getNextEvent() throws InterruptedException {
        Event evt;

        synchronized(eventQueueLock) {
            while (queue == null) {
                eventQueueLock.wait();
            }
            evt = queue;
            queue = (Event)queue.target;
            evt.target = this;
        }
        return evt;
    }

    /**
     * Start handling events.
     */
    public void start() {
        SecurityManager.setScopePermission();
        // Create a thread group for the applet, and start a new
        // thread to load the applet.
        handler = new Thread(new AppletThreadGroup("applet-" + atts.get("code"), this), this);
//      System.err.println("\n### starting new applet: "+handler);
        SecurityManager.resetScopePermission();
        handler.start();
    }

    /**
     * Execute applet events.
     */
    public void run() {
        // We have to wait to create the AppletClassLoader until we're on the
        // applet's thread. This is because now the construction of an
        // AppletClassLoader can notice that the codebase is a zip file and try
        // to download it. Only the mac supports doing java network activity on
        // the mozilla thread.
        classLoader =
            AppletClassLoader.getClassLoader(context, codebaseURL, archiveURL,
                                     getParameter("mayscript") != null);

        while (true) {
            Event evt;
            try {
                evt = getNextEvent();
            } catch (InterruptedException e) {
                showAppletException(e, "interrupted, bailing out...");
                return;
            }

            //System.err.println("EVENT = " + evt.id);
            try {
                switch (evt.id) {
                  case APPLET_LOAD:
                    if (status != APPLET_DISPOSE) {
                        wrongState("can't load", "not disposed");
                        break;
                    }

                    // Create the applet using the class loader
                    String code = getParameter("code");
                    if (code.endsWith(".class")) {
                        code = code.substring(0, code.length() - 6).replace('/', '.');
                    }
                    if (code.endsWith(".java")) {
                         code = code.substring(0, code.length() - 5).replace('/', '.');
                    }
                    try {
                        applet = (Applet)classLoader.loadClass(code).newInstance();
                    } catch (VerifyError e) {
                        showAppletException(e, "class " + e.getMessage()
                                            + " got a security violation: method verification error");
                        break;
                    } catch (SecurityException e) {
                        showAppletException(e, "class " + code + " got a security violation: " + e.getMessage());
                        break;
                    } catch (ClassNotFoundException e) {
                        showAppletException(e, "class " + code + " not found");
                        break;
                    } catch (InstantiationException e) {
                        showAppletException(e, "class " + code + " can't be instantiated");
                        break;
                    } catch (IllegalAccessException e) {
                        showAppletException(e, "class " + code + " is not public or has no public constructor");
                        break;
                    } catch (Exception e) {
                        showAppletException(e, "exception: " + e.toString());
                        break;
                    } catch (ThreadDeath e) {
                        showAppletStatus("killed");
                        return;
                    } catch (Error e) {
                        showAppletException(e, "error: " + e.toString());
                        break;
                    }

                    // Stick it in the frame
                    applet.setStub(this);
                    applet.hide();
                    add("Center", applet);
                    status = APPLET_LOAD;
                    rightState("loaded");
                    validate();
                    break;

                  case APPLET_INIT:
                    if (status != APPLET_LOAD) {
                        wrongState("can't init", "applet not loaded");
                        break;
                    }
                    pack();
                    show();
                    applet.resize(appletSize);
                    applet.init();
                    validate();
                    status = APPLET_INIT;
                    rightState("initialized");
                    break;

                  case APPLET_START:
                    if (status != APPLET_INIT) {
                        wrongState("can't start", "applet not initialized");
                        break;
                    }
                    status = APPLET_START;
                    timestamp = System.currentTimeMillis();
                    applet.resize(appletSize);
                    applet.start();
                    applet.show();
                    validate();
                    context.mochaOnLoad(0);
                    rightState("running");
                    break;

                  case APPLET_STOP:
                    if (status != APPLET_START) {
                        wrongState("can't stop", "applet not started");
                        break;
                    }
                    status = APPLET_INIT;
                    timestamp = System.currentTimeMillis();
                    applet.hide();
                    SecurityManager.setScopePermission();
                    applet.stop();
                    SecurityManager.resetScopePermission();
                    rightState("stopped");
                    break;

                  case APPLET_DESTROY:
                    if (status != APPLET_INIT) {
                        wrongState("can't destroy", "applet not stopped");
                        break;
                    }
                    status = APPLET_LOAD;
                    SecurityManager.setScopePermission();
                    applet.destroy();
                    SecurityManager.resetScopePermission();
                    hide();
                    rightState("destroyed");
                    break;

                  case APPLET_DISPOSE:
                    if (status != APPLET_LOAD) {
                        wrongState("can't dispose", "applet not destroyed");
                        break;
                    }
                    status = APPLET_DISPOSE;
                    remove(applet);
                    applet = null;
                    rightState("disposed");
                    dispose();              // Dispose of the frame too

                    // And then exit the thread
                    return;
                }
            } catch (SecurityException e) {
                showAppletException(e, "security violation: " + e.getMessage());
            } catch (Exception e) {
                showAppletException(e, "exception: " + e.toString());
            } catch (ThreadDeath e) {
                showAppletStatus("killed");
                return;
            } catch (Error e) {
                showAppletException(e, "error: " + e.toString());
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
     * Get an applet parameter.
     */
    public String getParameter(String name) {
        return (String)atts.get(name.toLowerCase());
    }

    /**
     * Get the document url.
     */
    public URL getDocumentBase() {
        return documentURL;
    }

    /**
     * Get the base url.
     */
    public URL getCodeBase() {
        return codebaseURL;
    }

    /**
     * Get the applet context. For now this is
     * also implemented by the EmbeddedAppletFrame class.
     */
    public AppletContext getAppletContext() {
        return context;
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
     * The last status message -- printed again when the mouse enters the
     * applet window.
     */
    String currentStatus = "";

    /**
     * Status line. Called by the EmbeddedAppletFrame to provide
     * feedback on the Applet's state.
     */
    protected void showAppletStatus(String status) {
        if (applet == null) {
            // could occur because of a load error
            currentStatus = "Applet "+status;
        }
        else {
            String name = applet.getParameter("name");
            if (name == null)
                name = applet.getClass().getName().toString();
            currentStatus = "Applet "+name+" "+status;
        }
        getAppletContext().showStatus(currentStatus);
    }

    boolean noisy = true;

    /**
     * Called by the EmbeddedAppletFrame to print to the log.
     */
    protected void showAppletLog(String str) {
        if (noisy) {
            String longStr;
            if (applet == null) {
                // could occur because of a load error
                longStr = "# Applet log: "+str;
            }
            else {
                String name = applet.getParameter("name");
                if (name == null)
                    name = applet.getClass().getName().toString();
                longStr = "# Applet "+name+" "+" log: "+str;
            }
            System.err.println(longStr);
        }
    }

    String errorReason = null;

    protected void rightState(String message) {
        errorReason = null;
        showAppletStatus(message);
    }

    protected void wrongState(String message, String newReason) {
        showAppletStatus(message + ": "
                         + (errorReason != null ? errorReason : newReason));
    }

    /**
     * Called by the EmbeddedAppletFrame to provide
     * feedback when an exception has happend.
     */
    protected void showAppletException(Throwable t, String message) {
        if (noisy) {
            // send mocha a message that exception occurred on load
            context.mochaOnLoad(-1);
            if (message == null)
                message = t.toString();
            errorReason = message;
            System.err.println("# Applet exception: " + message);
            t.printStackTrace();
            showAppletStatus(message);
        }
    }

    // Reprint the current status:
    public boolean mouseEnter(Event evt, int x, int y) {
        getAppletContext().showStatus(currentStatus);
        return true;
    }

    // Clear the current status:
    public boolean mouseExit(Event evt, int x, int y) {
        getAppletContext().showStatus("");
        return true;
    }
}
