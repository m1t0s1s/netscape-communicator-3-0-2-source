/*
 * @(#)WComponentPeer.java
 *
 * Copyright (c) 1996 Sun Microsystems, Inc. All Rights Reserved.
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

import java.awt.*;
import java.awt.peer.*;
import sun.awt.ScreenUpdater;
import sun.awt.UpdateClient;
import sun.awt.image.ImageRepresentation;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;

public
abstract class WComponentPeer implements ComponentPeer, UpdateClient 
{
    private int updateX1, updateY1, updateX2, updateY2;
    Component   target;
    int         pNativeWidget;
    int         pWinMsg;

    //
    // WComponentPeer constructor.
    //
    WComponentPeer(Component target) {
        this.target = target;
        Container parent = target.getParent();
        create((WComponentPeer)((parent != null) ? parent.getPeer() : null));
        initialize();
    }

    protected void finalize() {
        if (pNativeWidget != 0) {
	    dispose();
	}
    }

    abstract void create(WComponentPeer parent);

    // -----------------------------------------------------------------------
    //
    // Methods which implement of the java.awt.peer.ComponentPeer interface
    //
    // -----------------------------------------------------------------------
    public native void dispose();

    public native void show();
    public native void hide();
    public native void enable();
    public native void disable();

    public native void setForeground(Color c);
    public native void setBackground(Color c);
    public native void setFont(Font f);

    public native void requestFocus();
    public native void nextFocus();


    public native void reshape(int x, int y, int width, int height);
    public native boolean handleEvent(Event e);

    public Dimension minimumSize() 
    {
        return target.size();
    }

    public Dimension preferredSize() 
    {
        return minimumSize();
    }

    public ColorModel getColorModel() 
    {
        return WToolkit.getStaticColorModel();
    }

    public java.awt.Toolkit getToolkit() 
    {
        return Toolkit.getDefaultToolkit();
    }

    public Graphics getGraphics() 
    {
	      Graphics g = new WGraphics(this);
	      g.setColor(target.getForeground());
	      g.setFont(target.getFont());
	      return g;
    }

    public FontMetrics getFontMetrics(Font font) 
    {
	return WFontMetrics.getFontMetrics(font);
    }

    public void repaint(long tm, int x, int y, int width, int height) 
    {
        addRepaintArea(x, y, width, height);
        ScreenUpdater.updater.notify(this, tm);
    }

    public void paint(Graphics g) 
    {
        g.setColor(target.getForeground());
        g.setFont(target.getFont());
        target.paint(g);
    }

    public void update(Graphics g) 
    {
        g.setColor(target.getForeground());
        g.setFont(target.getFont());
        target.update(g);
    }

    public void print(Graphics g) 
    {
        Dimension d = target.size();
        g.setColor(target.getForeground());
        g.setFont(target.getFont());
        g.drawRect(0, 0, d.width - 1, d.height - 1);
        target.print(g);
    }


    public Image createImage(ImageProducer producer) 
    {
	      return new WImage(producer);
    }
    public Image createImage(int width, int height) 
    {
	      return new WImage(width, height);
    }


    public boolean prepareImage(Image img, int w, int h, ImageObserver o) 
    {
        return WToolkit.prepareScrImage(img, w, h, o);
    }

    public int checkImage(Image img, int w, int h, ImageObserver o) 
    {
        return WToolkit.checkScrImage(img, w, h, o);
    }

    //
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //
    // Methods which implement of the sun.awt.UpdateClient interface
    //
    // -----------------------------------------------------------------------

    public int updatePriority() 
    {
        return Thread.NORM_PRIORITY;
    }

    public void updateClient(Object arg) 
    {
        int x1, y1, x2, y2;

        synchronized (this) {
            x1 = updateX1;
            y1 = updateY1;
            x2 = updateX2;
            y2 = updateY2;
            updateX1 = updateX2;
        }

        if (x1 != x2) {
            widget_repaint(x1, y1, x2 - x1, y2 - y1);
        }
    }

    //
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    //
    // Protected routines which manipulate the native widget
    //
    // -----------------------------------------------------------------------
    protected native void widget_repaint(int x, int y, int width, int height);

    // -----------------------------------------------------------------------
    //
    // Package-private routine to initialize a peer object
    //
    // -----------------------------------------------------------------------
    void initialize() 
    {
        //
        // Show the widget (if necessary)
        //
        if (target.isVisible()) {
            show();
        } else {
            hide();
        }
        //
        // Initialize the foreground, background and font of the widget
        //
        Color c;
        Font  f;

        if ((c = target.getForeground()) != null) {
            setForeground(c);
        }
        if ((c = target.getBackground()) != null) {
            setBackground(c);
        }
        if ((f = target.getFont()) != null) {
            setFont(f);
        }

        //
        // Disable the widget (if necessary)
        //
        if (! target.isEnabled()) {
            disable();
        }

        //
        // Resize the widget to the correct shape
        //
        Rectangle r = target.bounds();
        reshape(r.x, r.y, r.width, r.height);
    }

    // -----------------------------------------------------------------------
    //
    // Routine to update the invalid region of the widget.
    // Each time repaint(...) is called the refresh area is included in
    // the invalid region.  
    //
    // When the Screen Updater notifies the component to repaint via the
    // UpdateClient(...) method, this area is used as the clip region 
    // for the repaint...
    //
    // -----------------------------------------------------------------------
    private synchronized void addRepaintArea(int x, int y, int w, int h) 
    {
        if (updateX1 == updateX2) {
            updateX1 = x;
            updateY1 = y;
            updateX2 = x + w;
            updateY2 = y + h;
        } else {
            if (x < updateX1) updateX1 = x;
            if (y < updateY1) updateY1 = y;
            if (x + w > updateX2) updateX2 = x + w;
            if (y + h > updateY2) updateY2 = y + h;
        }
    }

    public String toString() 
    {
        return getClass().getName() + "[" + target + "]";
    }


    //
    // This native method is a horrible hack to bypass the fact that 
    // Event.data is private and not accessable from the outside
    //
    private native Event setData(int data, Event ev);
    /*
    private Event setData(int data, Event ev) 
    {
        ev.data = data;
        return ev;
    }
    */

    // -----------------------------------------------------------------------
    //
    // Callback functions used by native code...
    //
    // -----------------------------------------------------------------------

    protected void handleQuit(long when) 
    {
		Event e = new Event(target, Event.WINDOW_DESTROY, null);
		target.postEvent(e);
    }


    protected void handleMoved(long when, int data, int x, int y) {
	    //target.postEvent(new Event(target, 0, Event.WINDOW_MOVED, x, y, 0, 0));
    }

    protected void handleResize(long when, int u1, 
				int width, int height, int u4, int u5)
    {
    }


    protected void handleGotFocus(long when, int data,
				  int u2, int u3, int u4, int u5) 
    {
        Event e = setData(data, new Event(target, Event.GOT_FOCUS, null));
        target.postEvent(e);
    }

    protected void handleLostFocus(long when, int data,
				  int u2, int u3, int u4, int u5) 
    {
		Event e = setData(data, new Event(target, Event.LOST_FOCUS, null));
        target.postEvent(e);
    }

    protected void handleActionKey(long when, int data,
                                    int x, int y, int key, int flags) 
    {
		Event e = setData(data, new Event(target, when, Event.KEY_ACTION, 
                          x, y, key, flags))
						  ;
        target.postEvent(e);
    }

    protected void handleActionKeyRelease(long when, int data, 
                                           int x, int y, int key, int flags) 
    {
		Event e = setData(data, new Event(target, when, Event.KEY_ACTION_RELEASE, 
                          x, y, key, flags))
						  ;
        target.postEvent(e);
    }

    protected void handleKeyPress(long when, int data,
                                   int x, int y, int key, int flags) 
    {
		Event e = setData(data, new Event(target, when, Event.KEY_PRESS, 
                          x, y, key, flags));

        target.postEvent(e);
    }

    protected void handleKeyRelease(long when, int data,
                                     int x, int y, int key, int flags) 
    {
		Event e = setData(data, new Event(target, when, Event.KEY_RELEASE, 
                          x, y, key, flags));

        target.postEvent(e);
    }

    protected void handleMouseEnter(long when, int data, int x, int y) 
    {
		Event e = new Event(target, when, Event.MOUSE_ENTER, x, y, 0, 0, null);

        target.postEvent(e);
    }

    protected void handleMouseExit(long when, int data, int x, int y) 
    {
		Event e = new Event(target, when, Event.MOUSE_EXIT, x, y, 0, 0, null);

        target.postEvent(e);
    }

    protected void handleMouseDown(long when, int data,
                                    int x, int y, int clickCount, int flags) 
    {
        Event e = setData(data, 
                          new Event(target, when, Event.MOUSE_DOWN, 
                                    x, y, 0, flags, null));
        e.clickCount = clickCount;

        target.postEvent(e);
    }

    protected void handleMouseUp(long when, int data, 
				 int x, int y, int u4, int flags) 
    {
		Event e = setData(data, new Event(target, when, Event.MOUSE_UP, 
                          x, y, 0, flags, null));

        target.postEvent(e);
    }

    protected void handleMouseMoved(long when, int data, 
                                     int x, int y, int flags) 
    {
		Event e = setData(data, new Event(target, when, Event.MOUSE_MOVE, 
                      x, y, 0, flags, null));

        target.postEvent(e);
    }

    protected void handleMouseDrag(long when, int data, 
                                    int x, int y, int flags) 
    {
        Event e = setData(data, new Event(target, when, Event.MOUSE_DRAG, 
                          x, y, 0, flags, null));

        target.postEvent(e);
    }

    protected void handleExpose(long when, int data,
                                 int x, int y, int w, int h) 
    {
        Graphics g = getGraphics();
        try {
            g.clipRect(x, y, w, h);
            paint(g);
        } finally {
            g.dispose();
        }
    }

    protected void handleRepaint(long when, int data,
                                 int x, int y, int w, int h) 
    {
        Graphics g = getGraphics();
        try {
            g.clipRect(x, y, w, h);
            update(g);
        } finally {
            g.dispose();
        }
    }
}
