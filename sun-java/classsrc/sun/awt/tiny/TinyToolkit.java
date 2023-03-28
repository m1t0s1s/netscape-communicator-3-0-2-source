/*
 * @(#)TinyToolkit.java	1.9 95/12/08 Arthur van Hoff
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

import java.util.Hashtable;
import java.awt.*;
import java.awt.peer.*;
import java.awt.image.*;
import java.net.URL;
import java.lang.SecurityManager;
import sun.awt.image.FileImageSource;
import sun.awt.image.URLImageSource;
import sun.awt.image.ImageRepresentation;

class TinyInputThread extends Thread {
    TinyInputThread() {
	super("Tiny-AWT-Input");
	start();
    }
    public native void run();
}

class TinyEventThread extends Thread {
    TinyEventThread() {
	super("Tiny-AWT-Event");
	start();
    }
    public native void run();
}

public class TinyToolkit extends Toolkit {
    static {
	SecurityManager.setScopePermission();
	System.loadLibrary("tawt");
    }

    public TinyToolkit() {
	init();
	new TinyInputThread();
	new TinyEventThread();
    }

    public native void init();

    /*
     * Create peer objects.
     */

    public WindowPeer createWindow(Window target) {
	return new TinyWindowPeer(target);
    }

    public FramePeer  createFrame(Frame target) {
	return new TinyFramePeer(target);
    }

    public CanvasPeer createCanvas(Canvas target) {
	return new TinyCanvasPeer(target);
    }

    public PanelPeer createPanel(Panel target) {
	return new TinyPanelPeer(target);
    }

    public ButtonPeer createButton(Button target) {
	return new TinyButtonPeer(target);
    }

    public TextFieldPeer createTextField(TextField target) {
	return new TinyTextFieldPeer(target);
    }

    public ChoicePeer createChoice(Choice target) {
	return new TinyChoicePeer(target);
    }

    public LabelPeer createLabel(Label target) {
	return new TinyLabelPeer(target);
    }

    public ListPeer createList(List target) {
	return new TinyListPeer(target);
    }

    public CheckboxPeer createCheckbox(Checkbox target) {
	return new TinyCheckboxPeer(target);
    }

    public ScrollbarPeer createScrollbar(Scrollbar target) {
	return new TinyScrollbarPeer(target);
    }

    public TextAreaPeer createTextArea(TextArea target) {
	return new TinyTextAreaPeer(target);
    }

    public DialogPeer createDialog(Dialog target) {
	return new TinyDialogPeer(target);
    }

    public FileDialogPeer createFileDialog(FileDialog target) {
	throw new InternalError("not implemented");
	//return new FileDialogPeer(target);
    }

    public MenuBarPeer createMenuBar(MenuBar target) {
	return new TinyMenuBarPeer(target);
    }

    public MenuPeer createMenu(Menu target) {
	return new TinyMenuPeer(target);
    }

    public MenuItemPeer createMenuItem(MenuItem target) {
	return new TinyMenuItemPeer(target);
    }

    public CheckboxMenuItemPeer createCheckboxMenuItem(CheckboxMenuItem target) {
	return new TinyCheckboxMenuItemPeer(target);
    }

    public Dimension getScreenSize() {
	return new Dimension(getScreenWidth(), getScreenHeight());
    }

    static native ColorModel makeColorModel();
    static ColorModel screenmodel;

    static ColorModel getStaticColorModel() {
	if (screenmodel == null) {
	    screenmodel = makeColorModel();
	}
	return screenmodel;
    }

    public ColorModel getColorModel() {
	return getStaticColorModel();
    }

    public native int getScreenResolution();
    native int getScreenWidth();
    native int getScreenHeight();

    public String[] getFontList() {
	// REMIND: return something useful
	String list[] = {"Dialog", "Helvetica", "TimesRoman", "Courier", "Symbol"};
	return list;
    }

    public FontMetrics getFontMetrics(Font font) {
	return TinyFontMetrics.getFontMetrics(font);
    }

    public native void sync();

    static Hashtable imgHash = new Hashtable();

    static synchronized Image getImageFromHash(Toolkit tk, URL url) {
	System.getSecurityManager().checkConnect(url.getHost(), url.getPort());
	Image img = (Image)imgHash.get(url);
	if (img == null) {
	    try {	
		img = tk.createImage(new URLImageSource(url));
		imgHash.put(url, img);
	    } catch (Exception e) {
	    }
	}
	return img;
    }

    static synchronized Image getImageFromHash(Toolkit tk, String filename) {
	System.getSecurityManager().checkRead(filename);
	Image img = (Image)imgHash.get(filename);
	if (img == null) {
	    try {	
		img = tk.createImage(new FileImageSource(filename));
		imgHash.put(filename, img);
	    } catch (Exception e) {
	    }
	}
	return img;
    }

    public Image getImage(String filename) {
	return getImageFromHash(this, filename);
    }

    public Image getImage(URL url) {
	return getImageFromHash(this, url);
    }

    static boolean prepareScrImage(Image img, int w, int h, ImageObserver o) {
	if (w == 0 || h == 0) {
	    return true;
	}
	TinyImage ximg = (TinyImage) img;
	if (ximg.hasError()) {
	    if (o != null) {
		o.imageUpdate(img, ImageObserver.ERROR|ImageObserver.ABORT,
			      -1, -1, -1, -1);
	    }
	    return false;
	}
	if (w < 0) w = -1;
	if (h < 0) h = -1;
	ImageRepresentation ir = ximg.getImageRep(w, h);
	return ir.prepare(o);
    }

    static int checkScrImage(Image img, int w, int h, ImageObserver o) {
	TinyImage ximg = (TinyImage) img;
	int repbits;
	if (w == 0 || h == 0) {
	    repbits = ImageObserver.ALLBITS;
	} else {
	    if (w < 0) w = -1;
	    if (h < 0) h = -1;
	    repbits = ximg.getImageRep(w, h).check(o);
	}
	return ximg.check(o) | repbits;
    }

    public int checkImage(Image img, int w, int h, ImageObserver o) {
	return checkScrImage(img, w, h, o);
    }

    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
	return prepareScrImage(img, w, h, o);
    }

    public Image createImage(ImageProducer producer) {
	return new TinyImage(producer);
    }
}
