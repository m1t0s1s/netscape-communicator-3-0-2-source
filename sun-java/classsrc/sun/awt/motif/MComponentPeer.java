/*
 * @(#)MComponentPeer.java	1.58 95/12/19 Sami Shaio
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
package sun.awt.motif;

import java.awt.*;
import java.awt.peer.*;
import sun.awt.*;
import sun.awt.image.ImageRepresentation;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.lang.SecurityManager;

public /* REMIND: should not be public */
abstract class MComponentPeer implements ComponentPeer, UpdateClient {
    Component	target;
    int 	pData;

    abstract void create(MComponentPeer parent);
    void create(MComponentPeer parent, Object arg) {
	create(parent);
    }
    native void disposeEvent(Event e);
    native void pInitialize();
    native void pShow();
    native void pHide();
    native void pEnable();
    native void pDisable();
    native void pReshape(int x, int y, int width, int height);
    native void pAddRepaint(int x, int y, int width, int height);
    native void pTriggerRepaint();
    native void pDispose();

    void initialize() {
	if (target.isVisible()) {
	    show();
	} else {
	    hide();
	}
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
	if (! target.isEnabled()) {
	    disable();
	}
	Rectangle r = target.bounds();
	reshape(r.x, r.y, r.width, r.height);
	pInitialize();
    }

    MComponentPeer(Component target, Object arg) {
	this.target = target;
	Container parent = target.getParent();
	create((MComponentPeer)((parent != null) ? parent.getPeer() : null), arg);
	initialize();
    }

    MComponentPeer(Component target) {
	this.target = target;
	Container parent = target.getParent();
	create((MComponentPeer)((parent != null) ? parent.getPeer() : null));
	initialize();
    }

    public native void setForeground(Color c);
    public native void setBackground(Color c);
    public native void setFont(Font f);

    public ColorModel getColorModel() {
	return MToolkit.getStaticColorModel();
    }

    public void show() {
	pShow();
    }
    public void hide() {
	pHide();
    }
    public void enable() {
	pEnable();
    }
    public void disable() {
	pDisable();
    }
    public int updatePriority() {
	return Thread.NORM_PRIORITY;
    }
    public void updateClient(Object arg) {
	pTriggerRepaint();
    }
    public void repaint(long tm, int x, int y, int width, int height) {
	pAddRepaint(x, y, width, height);
	SecurityManager.setScopePermission();
	ScreenUpdater.updater.notify(this, tm);
    }
    public void paint(Graphics g) {
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	target.paint(g);
    }
    public void update(Graphics g) {
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	target.update(g);
    }
    public void print(Graphics g) {
	Dimension d = target.size();
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	g.drawRect(0, 0, d.width - 1, d.height - 1);
	target.print(g);
    }
    public void reshape(int x, int y, int width, int height) {
	pReshape(x, y, width, height);
    }

    public native boolean handleEvent(Event e);

    public Dimension minimumSize() {
	return target.size();
    }
    public Dimension preferredSize() {
	return minimumSize();
    }
    public java.awt.Toolkit getToolkit() {
	// XXX: bogus
	return Toolkit.getDefaultToolkit();
    }
    public Graphics getGraphics() {
	Graphics g = new X11Graphics(this);
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	return g;
    }
    public Image createImage(ImageProducer producer) {
	return new X11Image(producer);
    }
    public Image createImage(int width, int height) {
	return new X11Image(width, height);
    }
    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
	return MToolkit.prepareScrImage(img, w, h, o);
    }
    public int checkImage(Image img, int w, int h, ImageObserver o) {
	return MToolkit.checkScrImage(img, w, h, o);
    }
    public FontMetrics getFontMetrics(Font font) {
	return X11FontMetrics.getFontMetrics(font);
    }
    public void dispose() {
	pDispose();
    }
    public native void requestFocus();

    public native void nextFocus();

    protected void gotFocus() {
	target.postEvent(new Event(target, Event.GOT_FOCUS, null));
    }

    protected void lostFocus() {
	target.postEvent(new Event(target, Event.LOST_FOCUS, null));
    }

    protected void handleActionKey(long when, int data,
				   int x, int y, int key, int flags) {
	Event e = setData(data, new Event(target, when,
					  Event.KEY_ACTION,
					  x, y, key, flags, null));
	target.postEvent(e);
	disposeEvent(e);
    }

    protected void handleActionKeyRelease(long when, int data, int x, int y, int key, int flags) {
	Event e = setData(data, new Event(target, when,
					  Event.KEY_ACTION_RELEASE,
					  x, y, key, flags, null));
	target.postEvent(e);
	disposeEvent(e);
    }

    protected void handleKeyPress(long when, int data,
				  int x, int y, int key, int flags) {
	Event e = setData(data, new Event(target, when,
					  Event.KEY_PRESS, x, y,
					  key, flags, null));
	target.postEvent(e);
	disposeEvent(e);
    }

    protected void handleKeyRelease(long when, int data,
				    int x, int y, int key, int flags) {
	Event e = setData(data, new Event(target, when, Event.KEY_RELEASE,
					  x, y, key, flags, null));
	target.postEvent(e);
	disposeEvent(e);
    }

    protected void handleMouseEnter(long when, int x, int y) {
	target.postEvent(new Event(target, when, Event.MOUSE_ENTER, x, y, 0, 0, null));
    }

    protected void handleMouseExit(long when, int x, int y) {
	target.postEvent(new Event(target, when, Event.MOUSE_EXIT, x, y, 0, 0, null));
    }

    protected void handleMouseDown(long when, int data, int x, int y,
				   int xroot, int yroot, int
				   clickCount, int flags) {
	Event e = setData(data, new Event(target, when, Event.MOUSE_DOWN, x, y, 0, flags, null));

	e.clickCount = clickCount;
	target.postEvent(e);
    }

    protected void handleMouseUp(long when, int data, int x, int y,
				 int xroot, int yroot, int flags) {
	target.postEvent(setData(data, new Event(target, when, Event.MOUSE_UP, x, y, 0, flags, null)));
    }

    protected void handleMouseMoved(long when, int data, int x, int y,
				    int xroot, int yroot, int flags) {
	target.postEvent(setData(data, new Event(target, when, Event.MOUSE_MOVE, x, y, 0, flags, null)));
    }

    protected void handleMouseDrag(long when, int data, int x, int y,
				   int xroot, int yroot, int flags) {
	target.postEvent(setData(data, new Event(target, when, Event.MOUSE_DRAG, x, y, 0, flags, null)));
    }

    native Event setData(int data, Event ev);

    /* Callbacks for window-system events to the frame */
    void handleExpose(int x, int y, int w, int h) {
	Graphics g = getGraphics();
	try {
	    g.clipRect(x, y, w, h);
	    paint(g);
	} finally {
	    g.dispose();
	}
    }
    /* Callbacks for window-system events to the frame */
    void handleRepaint(int x, int y, int w, int h) {
	Graphics g = getGraphics();
	try {
	    g.clipRect(x, y, w, h);
	    update(g);
	} finally {
	    g.dispose();
	}
    }

    public String toString() {
	return getClass().getName() + "[" + target + "]";
    }
}
