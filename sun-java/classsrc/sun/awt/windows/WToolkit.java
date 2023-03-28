/*
 * @(#)WTextAreaPeer.java	1.8 95/08/09 Sami Shaio
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

import java.util.Hashtable;
import java.awt.*;
import java.awt.image.*;
import java.awt.peer.*;
import java.io.*;
import java.net.URL;
import java.lang.SecurityManager;
import sun.awt.image.FileImageSource;
import sun.awt.image.URLImageSource;
import sun.awt.image.ImageRepresentation;


public class WToolkit extends java.awt.Toolkit implements Runnable
{
    boolean serverThread  = true;
    boolean libraryLoaded = false;
    Thread callbackThread;

    Thread getCallbackThread() {
        return callbackThread;
    }

    public WToolkit()
    {
	new Thread(this, "AWT-Windows").start();
	synchronized (this) {
	    try {
		while (serverThread) {
		    wait(5000);
		}
	    } catch (InterruptedException e) {
//	        System.err.println("Exception received while starting " +
//		                   "Awt main thread - aborting.");
		System.exit(1);
	    }
	}

	if (libraryLoaded) {
	    callbackThread = new Thread(this, "AWT-Callback");
	    callbackThread.start();
	}
    }

    private native void init(Thread t);

    public void run()
    {
	if (serverThread) {
           try {
                //
                // Load either the 16-bit or 32-bit version of Awt...
                //
		SecurityManager.setScopePermission();
                String nm = System.getProperty("os.name");
                if (nm.compareTo("16-bit Windows") == 0) {
                    System.loadLibrary("awt1630");
                } else {
                    System.loadLibrary("awt32301");
                }
		SecurityManager.resetScopePermission();

                init(Thread.currentThread());
                libraryLoaded = true;
                serverThread = false;

                eventLoop();
            } catch (UnsatisfiedLinkError e) {
                libraryLoaded = false;
                serverThread = false;

//                System.err.println("Unable to load AWT widget library.");
            }
        } else {
	    callbackLoop();
	}
    }

    /*
     * Loop to deliver events from Awt to java
     */
    native void callbackLoop();

    /*
     * Notifies the thread that started the server
     * to indicate that initialization is complete.
     * Then enters an infinite loop that retrieves and processes events.
     */
    native void eventLoop();

/* FIXME */
    public native void notImplemented();  // throws an assertion in awt3230.dll
/* FIXME */

    /*
     * Create peer objects.
     */

    protected ButtonPeer createButton(Button target)
    {
        return new WButtonPeer(target);
    }

    protected TextFieldPeer createTextField(TextField target)
    {
	return new WTextFieldPeer(target);
    }

    protected LabelPeer createLabel(Label target)
    {
	return new WLabelPeer(target);
    }

    protected ListPeer createList(List target)
    {
	return new WListPeer(target);
    }

    protected CheckboxPeer createCheckbox(Checkbox target)
    {
	return new WCheckboxPeer(target);
    }

    protected ScrollbarPeer createScrollbar(Scrollbar target)
    {
	return new WScrollbarPeer(target);
    }

    protected TextAreaPeer createTextArea(TextArea target)
    {
        return new WTextAreaPeer(target);
    }

    protected ChoicePeer createChoice(Choice target)
    {
	return new WChoicePeer(target);
    }

    protected FramePeer  createFrame(Frame target)
    {
        return new WFramePeer(target);
    }

    protected CanvasPeer createCanvas(Canvas target)
    {
        return new WCanvasPeer(target);
    }

    protected PanelPeer createPanel(Panel target)
    {
        return new WPanelPeer(target);
    }

    protected WindowPeer createWindow(Window target)
    {
        return new WWindowPeer(target);
    }

    protected DialogPeer createDialog(Dialog target)
    {
		return new WDialogPeer(target);
    }

    protected FileDialogPeer createFileDialog(FileDialog target)
    {
//      return new WFileDialogPeer(target);
        System.out.println("createFileDialog not implemented");
        return null;
    }

    protected MenuBarPeer createMenuBar(MenuBar target)
    {
        return new WMenuBarPeer(target);
    }

    protected MenuPeer createMenu(Menu target)
    {
        return new WMenuPeer(target);
    }

    protected MenuItemPeer createMenuItem(MenuItem target)
    {
        return new WMenuItemPeer(target);
    }

    protected CheckboxMenuItemPeer createCheckboxMenuItem(CheckboxMenuItem target)
    {
        return new WCheckboxMenuItemPeer(target);
    }

    static Hashtable imgHash = new Hashtable();

    static synchronized Image getImageFromHash(Toolkit tk, URL url)
    {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkConnect(url.getHost(), url.getPort());
        }

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

    static synchronized Image getImageFromHash(Toolkit tk, String filename)
    {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkRead(filename);
        }

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

    public Image getImage(String filename)
    {
        return getImageFromHash(this, filename);
    }

    public Image getImage(URL url) {
        return getImageFromHash(this, url);
    }

    static boolean prepareScrImage(Image img, int w, int h, ImageObserver o)
    {
        if (w == 0 || h == 0) {
            return true;
        }

        WImage ximg = (WImage) img;
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
        //return false;
    }

    static int checkScrImage(Image img, int w, int h, ImageObserver o)
    {
        WImage ximg = (WImage) img;
        int repbits;
        if (w == 0 || h == 0) {
            repbits = ImageObserver.ALLBITS;
        } else {
            if (w < 0) w = -1;
            if (h < 0) h = -1;
            repbits = ximg.getImageRep(w, h).check(o);
        }
        return ximg.check(o) | repbits;
        //return 0;
    }

    public int checkImage(Image img, int w, int h, ImageObserver o)
    {
        return checkScrImage(img, w, h, o);
    }

    public boolean prepareImage(Image img, int w, int h, ImageObserver o)
    {
        return prepareScrImage(img, w, h, o);
    }

    public Image createImage(ImageProducer producer)
    {
//         System.out.println("Toolkit.public Image createImage(ImageProducer producer) Called");
         return new WImage(producer);
        //notImplemented();
        //return null;
    }

    static native ColorModel makeColorModel();
    static ColorModel screenmodel;

    static ColorModel getStaticColorModel()
    {
        if (screenmodel == null) {
            screenmodel = makeColorModel();
        }
        return screenmodel;
    }

    public ColorModel getColorModel()
    {
        return getStaticColorModel();
    }

    public Dimension getScreenSize()
    {
        return new Dimension(getScreenWidth(), getScreenHeight());
    }

    public native int getScreenResolution();
    public native void sync();

    native int getScreenWidth();
    native int getScreenHeight();

    public String[] getFontList()
    {
        // REMIND: return something useful
        String list[] = {"Dialog", "Helvetica", "TimesRoman", "Courier", "Symbol"};
        return list;
    }

    public FontMetrics getFontMetrics(Font font)
    {
        return WFontMetrics.getFontMetrics(font);
    }

}
