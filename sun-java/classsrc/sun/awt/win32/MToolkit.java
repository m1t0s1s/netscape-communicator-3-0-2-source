/*
 * @(#)MToolkit.java	1.25 95/12/08 Sami Shaio
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

package sun.awt.win32;

import java.util.Hashtable;
import java.awt.*;
import java.awt.image.*;
import java.awt.peer.*;
import java.net.URL;
import java.lang.SecurityManager;
import sun.awt.image.FileImageSource;
import sun.awt.image.URLImageSource;
import sun.awt.image.ImageRepresentation;

public class MToolkit extends java.awt.Toolkit implements Runnable {
    final static String AwtLibraryName = "awt3221";

    boolean serverThread = true;
    boolean libraryLoaded = true;

    public MToolkit() {
	new Thread(this, "AWT-Win32").start();
        synchronized (this) {
            try {
                while( serverThread ) {
                    wait(5000);
                }
            } catch (InterruptedException e) {
                System.err.println("Exception received while starting " +
                                   "AWT main thread, aborting.");
                System.exit(1);
            }
        }
	if( libraryLoaded ) {
	    new Thread(this, "AWT-Callback-Win32").start();
	}
    }

    public native void init(Thread t);

    public void run() {
        if (serverThread) {
            try {
		SecurityManager.setScopePermission();
                System.loadLibrary(AwtLibraryName);
		SecurityManager.resetScopePermission();
                serverThread = false;

                init(Thread.currentThread());
                eventLoop();
            } catch (UnsatisfiedLinkError e) {
                libraryLoaded = false;
                serverThread = false;
                System.err.println("Unable to load AWT widget library: " + AwtLibraryName);
            }
        } else {
            callbackLoop();
        }
    }

    /* 
     * Notifies the thread that started the server
     * to indicate that initialization is complete.
     * Then enters an infinite loop that retrieves and processes events.  
     */
    native void eventLoop();

    /* 
     * A thread needs to be created to call this method.  This
     * thread will be responsible for making all the callbacks
     * from the window system to the java code.
     */
    native void callbackLoop();

    /*
     * Create peer objects.
     */

    public ButtonPeer createButton(Button target) {
	return new MButtonPeer(target);
    }

    public TextFieldPeer createTextField(TextField target) {
	return new MTextFieldPeer(target);
    }

    public LabelPeer createLabel(Label target) {
	return new MLabelPeer(target);
    }

    public ListPeer createList(List target) {
	return new MListPeer(target);
    }

    public CheckboxPeer createCheckbox(Checkbox target) {
	return new MCheckboxPeer(target);
    }

    public ScrollbarPeer createScrollbar(Scrollbar target) {
	return new MScrollbarPeer(target);
    }

    public TextAreaPeer createTextArea(TextArea target) {
	return new MTextAreaPeer(target);
    }

    public ChoicePeer createChoice(Choice target) {
	return new MChoicePeer(target);
    }

    public FramePeer  createFrame(Frame target) {
	return new MFramePeer(target);
    }

    public CanvasPeer createCanvas(Canvas target) {
	return new MCanvasPeer(target);
    }

    public PanelPeer createPanel(Panel target) {
	return new MPanelPeer(target);
    }

    public WindowPeer createWindow(Window target) {
	return new MWindowPeer(target);
    }

    public DialogPeer createDialog(Dialog target) {
	return new MDialogPeer(target);
    }

    public FileDialogPeer createFileDialog(FileDialog target) {
	throw new AWTError("FileDialog is unimplemented.");
//	return new MFileDialogPeer(target);
    }

    public MenuBarPeer createMenuBar(MenuBar target) {
	return new MMenuBarPeer(target);
    }

    public MenuPeer createMenu(Menu target) {
	return new MMenuPeer(target);
    }

    public MenuItemPeer createMenuItem(MenuItem target) {
	return new MMenuItemPeer(target);
    }

    public CheckboxMenuItemPeer createCheckboxMenuItem(CheckboxMenuItem target) {
	return new MCheckboxMenuItemPeer(target);
    }

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
	Win32Image ximg = (Win32Image) img;
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
	Win32Image ximg = (Win32Image) img;
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
	return new Win32Image(producer);
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

    public Dimension getScreenSize() {
	return new Dimension(getScreenWidth(), getScreenHeight());
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
	return Win32FontMetrics.getFontMetrics(font);
    }

    public native void sync();
}