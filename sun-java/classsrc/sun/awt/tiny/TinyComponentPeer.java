/*
 * @(#)TinyComponentPeer.java	1.4 95/12/01 Arthur van Hoff
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
package sun.awt.tiny;

import java.awt.*;
import java.awt.peer.*;
import sun.awt.*;
import sun.awt.image.ImageRepresentation;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;

abstract 
class TinyComponentPeer extends TinyWindow implements ComponentPeer, UpdateClient {
    Component	target;
    Rectangle	repaintRect;

    /**
     * Initialize the component.
     */
    void init(Component target) {
	this.target = target;

	if (target.isVisible()) {
	    show();
	}

	Color c = target.getBackground();
	if (c != null) {
	    winBackground(c);
	}

	Rectangle r = target.bounds();
	reshape(r.x, r.y, r.width, r.height);
    }

    /*
     * Repaint
     */
    synchronized Rectangle getRepaintRect() {
	Rectangle r = repaintRect;
	repaintRect = null;
	return r;
    }
    public int updatePriority() {
	return Thread.NORM_PRIORITY;
    }
    public void updateClient(Object arg) {
	Rectangle r = getRepaintRect();
	if (r != null) {
	    Graphics g = new TinyGraphics(this);
	    try {
		g.clipRect(r.x, r.y, r.width, r.height);
		update(g);
	    } finally {
		g.dispose();
	    }
	}
    }
    public synchronized void repaint(long tm, int x, int y, int width, int height) {
	if (repaintRect == null) {
	    repaintRect = new Rectangle(x, y, width, height);
	} else {
	    repaintRect = repaintRect.union(new Rectangle(x, y, width, height));
	}
	ScreenUpdater.updater.notify(this, tm);
    }
    void repaint() {
	repaint(0, 0, 0, width, height);
    }

    void paintComponent(Graphics g) {
	g.setColor(target.getForeground());
	g.drawRect(0, 0, width-1, height-1);
	g.setColor(target.getBackground());
	g.fillRect(1, 1, width-2, height-2);
    }
    public void paint(Graphics g) {
	paintComponent(g);
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	target.paint(g);
    }
    public void update(Graphics g) {
	paint(g);
    }
    public void print(Graphics g) {
	paintComponent(g);
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	target.print(g);
    }

    public Graphics getGraphics() {
	Graphics g = new TinyGraphics(this);
	g.setColor(target.getForeground());
	g.setFont(target.getFont());
	return g;
    }

    /*
     * Changing parameters
     */
    public void reshape(int x, int y, int w, int h) {
	winReshape(x, y, w, h);
    }
    public void show() {
	winShow();
    }
    public void hide() {
	winHide();
    }
    public void enable() {
	repaint();
    }
    public void disable() {
	repaint();
    }
    public void setFont(Font f) {
	repaint();
    }
    public void setBackground(Color c) {
	repaint();
	winBackground(c);
    }
    public void setForeground(Color c) {
	repaint();
    }
    public Dimension minimumSize() {
	return target.size();
    }
    public Dimension preferredSize() {
	return minimumSize();
    }
    public ColorModel getColorModel() {
	return TinyToolkit.getStaticColorModel();
    }
    public FontMetrics getFontMetrics(Font font) {
	return TinyFontMetrics.getFontMetrics(font);
    }
    public java.awt.Toolkit getToolkit() {
	return Toolkit.getDefaultToolkit();
    }
    public void dispose() {
	winDispose();
    }

    public String toString() {
	return getClass().getName() + "[" + target + "]";
    }

    public void targetEvent(int id, Object arg) {
	targetEvent(new Event(target, id, arg));
    }
    public void targetEvent(Event evt) {
	try {
	    target.postEvent(evt);
	} catch (ThreadDeath e) {
	    throw e;
	} catch (Throwable e) {
	    e.printStackTrace();
	}
    }
    public boolean handleEvent(Event evt) {
	return false;
    }

    public Image createImage(ImageProducer producer) {
	return new TinyImage(producer);
    }
    public Image createImage(int width, int height) {
	return new TinyImage(width, height);
    }
    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
	return TinyToolkit.prepareScrImage(img, w, h, o);
    }
    public int checkImage(Image img, int w, int h, ImageObserver o) {
	return TinyToolkit.checkScrImage(img, w, h, o);
    }

/* ----------- INSANITY STARTS BELOW THIS LINE --------------- */

    public void nextFocus() {
    }
}
