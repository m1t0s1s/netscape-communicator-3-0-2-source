/*
 * @(#)WFramePeer.java	1.1 96/03/05  Thomas Ball
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
import java.awt.image.ImageObserver;
import sun.awt.image.ImageRepresentation;

class WFramePeer extends WWindowPeer implements FramePeer
{
    //static Vector allFrames = new Vector();

    // native create function
    native void create(WComponentPeer parent);

    // FramePeer interface
    public native void setTitle(String title);
    public void setIconImage(Image im) {
// FIXME
//	ImageRepresentation ir = ((Win32Image)im).getImageRep(-1, -1);
//	ir.reconstruct(ImageObserver.ALLBITS);
//	widget_setIconImage(ir);
    }
    public native void setMenuBar(MenuBar mb);
    public native void setResizable(boolean resizable);
    public native void setCursor(int cursorType);

    native void widget_setIconImage(ImageRepresentation ir);

    // WFramePeer constructor.
    WFramePeer(Frame target) {
	    super(target);

	    //allFrames.addElement(this);

	    if (target.getTitle() != null) {
	        setTitle(target.getTitle());
	    }

	    Image icon = target.getIconImage();
	    if (icon != null) {
	        setIconImage(icon);
	    }

	    if (target.getCursorType() != Frame.DEFAULT_CURSOR) {
	        setCursor(target.getCursorType());
	    }

	    setResizable(target.isResizable());
    }


    //public void dispose() {
	//    allFrames.removeElement(this);
	//    super.dispose();
    //}


    /*
     * The following methods are event handlers called by the native
     * widget.
     */

    public void handleIconify(long when) {
	    target.postEvent(new Event(target, Event.WINDOW_ICONIFY, null));
    }

    public void handleDeiconify(long when) {
	    target.postEvent(new Event(target, Event.WINDOW_DEICONIFY, null));
    }

    /**
     * Called to inform the Frame that it has moved.
     */
    protected void handleMoved(long when, int data, int x, int y) {
	    target.postEvent(new Event(target, 0, Event.WINDOW_MOVED, x, y, 0, 0));
    }

    /**
     * Called to inform the Frame that its size has changed and it
     * should layout its children.
     */
    protected void handleResize(long when, int u1,
				int width, int height, int u4, int u5)
    {
    	target.invalidate();
    	target.validate();

    	target.repaint();
    }


}
