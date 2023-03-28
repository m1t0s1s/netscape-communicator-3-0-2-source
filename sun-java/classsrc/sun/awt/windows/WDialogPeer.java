/*
 * @(#)WDialogPeer.java	1.8 95/11/29 Sami Shaio
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
package sun.awt.windows;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;

class WDialogPeer extends WWindowPeer implements DialogPeer {

    boolean notify = false;

    //static Vector allDialogs = new Vector();

    native void create(WComponentPeer parent);

    // DialogPeer interface
    public native void setTitle(String title);
    public native void setResizable(boolean resizable);

    public native void showModal(boolean spinLoop);
    public native void hideModal();

    WDialogPeer(Dialog target) {
		super(target);

		//allDialogs.addElement(this);

		if (target.getTitle() != null) {
			setTitle(target.getTitle());
		}

		setResizable(target.isResizable());
    }

    public void dispose() {
	//	allDialogs.removeElement(this);
	    hide();
		super.dispose();
    }

    public void show() {
        // check if this is a modal dialog box
        if (((Dialog)target).isModal()) {
            if (((WToolkit)getToolkit()).getCallbackThread() == Thread.currentThread())
                // it's running in the callback thread
                showModal(true);
            else {
                // it's running on some thread....
                // show the dialog and put the thread in wait

                // since show is synchronized on the dialog we cannot
                // synchronize here and call wait otherwise we'll deadlock
                showModal(false);

                // synchronize and wait on the dialog
                synchronized(target) {
                    notify = true;
                    try {
                        target.wait(0);
                    }
            	    catch (InterruptedException e) {
            	    }
                }
        	}
        }
        else
            super.show();
    }

    public void hide() {
        if (((Dialog)target).isModal()) {
            if (notify) {

                // notify the waiting thread the dialog is being closed
                synchronized(target) {
                    notify = false;
                    try {
                        target.notifyAll();
                    }
            	    catch (IllegalMonitorStateException e) {
            	    }
        	    }

            }

            hideModal();
        }
        else
            super.hide();
    }


    /**
     * Called to inform the Dialog that it has moved.
     */
    public synchronized void handleMoved(long when, int data, int x, int y) {
		target.postEvent(new Event(target, 0, Event.WINDOW_MOVED, x, y, 0, 0));
    }


    public void handleIconify(long when) {
		target.postEvent(new Event(target, Event.WINDOW_ICONIFY, null));
    }

    public void handleDeiconify(long when) {
		target.postEvent(new Event(target, Event.WINDOW_DEICONIFY, null));
    }

    /**
     * Called to inform the Dialog that its size has changed and it
     * should layout its children.
     */
    protected void handleResize(long when, int u1,
				int width, int height, int u4, int u5) {
		//target.resize(width, height);
		target.invalidate();
		target.validate();
		target.repaint();
    }
}

