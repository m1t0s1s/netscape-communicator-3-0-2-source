/*
 * @(#)ScreenUpdater.java	1.10 95/08/30  
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

package sun.awt;

import java.lang.SecurityManager;

class ScreenUpdaterEntry {
    UpdateClient client;
    long when;
    ScreenUpdaterEntry next;
    Object arg;

    ScreenUpdaterEntry(UpdateClient client, long when, Object arg, ScreenUpdaterEntry next) {
	this.client = client;
	this.when = when;
	this.arg = arg;
	this.next = next;
    }
}

/**
 * A seperate low priority thread that warns clients
 * when they need to update the screen. Clients that
 * need a wakeup call need to call Notify().
 *
 * @version 	1.10, 30 Aug 1995
 * @author 	Arthur van Hoff
 */
public
class ScreenUpdater extends Thread {
    private final int PRIORITY = Thread.NORM_PRIORITY - 1;
    private ScreenUpdaterEntry first;

    static ThreadGroup findSystemThreadGroup() {
	// This hack was put in so that the ScreenUpdater didn't get put in some
	// applet's thread group, and would consequently get killed when the applet
	// was killed. Take it out when Sun delivers a fix. -Warren
	Thread thread = Thread.currentThread();
	ThreadGroup group = thread.getThreadGroup();
	while (group != null && !group.getName().equals("main")) {
	    group = group.getParent();
	}
	return group;
    }

    static ScreenUpdater newScreenUpdater() {
	SecurityManager.setScopePermission();
        return new ScreenUpdater(findSystemThreadGroup(), "ScreenUpdater");
    }

    /**
     * The screen updater. 
     */
    public final static ScreenUpdater updater = newScreenUpdater();

    /**
     * Constructor. Starts the thread.
     */
    private ScreenUpdater(ThreadGroup group, String name) {
	super(group, name);
	start();
    }

    /**
     * Constructor. Starts the thread.
     */
    private ScreenUpdater() {
	start();
    }

    /**
     * Update the next client
     */
    private synchronized ScreenUpdaterEntry nextEntry() throws InterruptedException {
	while (true) {
	    ScreenUpdaterEntry entry;
	    if (first == null) {
		entry = null;
		wait();
		continue;
	    }

	    long delay = first.when - System.currentTimeMillis();
	    if (delay <= 0) {
		entry = first;
		first = first.next;
		return entry;
	    }

	    entry = null;
	    wait(delay);
	}
    }

    /**
     * The main body of the screen updater.
     */
    public void run() {
	SecurityManager.setScopePermission();
	setName("ScreenUpdater");

	try {
	    while (true) {
		setPriority(PRIORITY);
		ScreenUpdaterEntry entry = nextEntry();
		setPriority(entry.client.updatePriority());

		try {
		    entry.client.updateClient(entry.arg);
		} catch (Throwable e) {
		    e.printStackTrace();
		}

		// Because this thread is permanent, we don't want to defeat
		// the garbage collector by having dangling references to
		// objects we no longer care about. Clear out entry so that
		// when we go back to sleep in nextEntry we won't hold a
		// dangling reference to the previous entry we processed.
		entry = null;
	    }
	} catch (InterruptedException e) {
	}
    }

    /**
     * Notify the screen updater that a client needs
     * updating. As soon as the screen updater is
     * scheduled to run it will ask all of clients that
     * need updating to update the screen.
     */
    public void notify(UpdateClient client) {
	notify(client, 100, null);
    }
    public synchronized void notify(UpdateClient client, long delay) {
	notify(client, delay, null);
    }
    public synchronized void notify(UpdateClient client, long delay, Object arg) {
	long when = System.currentTimeMillis() + delay;
	long nextwhen = (first != null) ? first.when : -1L;

	if (first != null) {
	    if (first.client == client) {
		when = Math.min(first.when, when);
		first = first.next;
	    } else {
		for (ScreenUpdaterEntry e = first ; e.next != null ; e = e.next) {
		    if ((e.next.client == client) && (e.next.arg == arg)) {
			when = Math.min(e.next.when, when);
			e.next = e.next.next;
			break;
		    }
		}
	    }
	}

	if ((first == null) || (first.when > when)) {
	    first = new ScreenUpdaterEntry(client, when, arg, first);
	} else {
	    for (ScreenUpdaterEntry e = first ; ; e = e.next) {
		if ((e.next == null) || (e.next.when > when)) {
		    e.next = new ScreenUpdaterEntry(client, when, arg, e.next);
		    break;
		}
	    }
	}
	if (nextwhen != first.when) {
	    super.notify();
	}
    }
}
