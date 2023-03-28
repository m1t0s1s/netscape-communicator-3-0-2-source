/*
 * @(#)ImageTest.java	1.19 95/08/29 Jim Graham
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
import java.awt.*;
import java.awt.image.*;
import sun.awt.image.URLImageSource;

public class ImageTest extends Frame {
    public ImageTest() {
	setTitle("ImageTest");
	setBackground(Color.lightGray);
	setLayout(new BorderLayout());
	add("Center", new ImagePanel());
	add("North", new ImageHelp());
	reshape(0, 0, 800, 600);
	show();
    }

    public static void main(String args[]) {
	new ImageTest();
    }
}

class ImageHelp extends Panel {
    public ImageHelp() {
	setLayout(new GridLayout(0, 2));
	add(new Label("Move the images using the arrow keys",
		      Label.CENTER));
	add(new Label("Resize the images using the PgUp/PgDn keys",
		      Label.CENTER));
	add(new Label("Toggle a red/blue color filter using the Home key",
		      Label.CENTER));
	add(new Label("Change the alpha using the shifted PgUp/PgDn keys",
		      Label.CENTER));
    }
}

class ImagePanel extends Panel {
    public ImagePanel() {
	setLayout(new BorderLayout());
	Panel grid = new Panel();
	grid.setLayout(new GridLayout(0, 2));
	add("Center", grid);
	String dir = "/net/benden.eng/export/home/benden0/htdocs/";
	String base = "http://benden.eng/";
	grid.add(new ImageCanvas(base, "gulls.gif", 0.5));
	grid.add(new ImageCanvas(base, "host.icon.gif", 0.5));
	grid.add(new ImageCanvas(makeDitherImage(), 0.5));
	grid.add(new ImageCanvas(base, "graphics/joust_91.gif", 0.05));
	reshape(0, 0, 20, 20);
    }

    sun.awt.image.Image makeDitherImage() {
	int w = 100;
	int h = 100;
	int pix[] = new int[w * h];
	int index = 0;
	for (int y = 0; y < h; y++) {
	    int red = (y * 255) / (h - 1);
	    for (int x = 0; x < w; x++) {
		int blue = (x * 255) / (w - 1);
		pix[index++] = (255 << 24) | (red << 16) | blue;
	    }
	}
	return (sun.awt.image.Image)Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(w, h, pix, 0, w));
    }
}

class ImageCanvas extends Canvas implements ImageObserver {
    double 	hmult = 0;
    int		xadd = 0;
    int		yadd = 0;
    int		imgw = -1;
    int		imgh = -1;
    int		scalew = -1;
    int		scaleh = -1;
    boolean	focus = false;
    String	imgfilename;
    boolean	usefilter = false;
    static final int numalphas = 8;
    int		alpha = numalphas - 1;
    sun.awt.image.Image	imagevariants[] = new sun.awt.image.Image[numalphas * 2];
    ImageFilter colorfilter;
    sun.awt.image.Image	origimage;
    sun.awt.image.Image	curimage;

    public ImageCanvas(sun.awt.image.Image img, double mult) {
	imgfilename = "Precalculated Memory Image";
	origimage = img;
	imagevariants[numalphas - 1] = (sun.awt.image.Image)origimage;
	hmult = mult;
	pickImage();
	reshape(0, 0, 100, 100);
    }

    public ImageCanvas(String dir, String filename, double mult) {
	imgfilename = dir+filename;
	try {
	    origimage = (sun.awt.image.Image)Toolkit.getDefaultToolkit().createImage(new URLImageSource(imgfilename));
	} catch (java.net.MalformedURLException e) {
	}
	imagevariants[numalphas - 1] = (sun.awt.image.Image)origimage;
	hmult = mult;
	pickImage();
	reshape(0, 0, 100, 100);
    }

    public void gotFocus() {
	focus = true;
	repaint();
    }

    public void lostFocus() {
	focus = false;
	repaint();
    }

    public void paint(Graphics g) {
	Rectangle r = bounds();
	int hlines = r.height / 10;
	int vlines = r.width / 10;

	if (focus) {
	    g.setColor(Color.red);
	} else {
	    g.setColor(Color.darkGray);
	}
	g.drawRect(0, 0, r.width-1, r.height-1);
	g.drawLine(0, 0, r.width, r.height);
	g.drawLine(r.width, 0, 0, r.height);
	g.drawLine(0, r.height / 2, r.width, r.height / 2);
	g.drawLine(r.width / 2, 0, r.width / 2, r.height);
	if (imgw < 0) {
	    imgw = curimage.getWidth(this);
	    imgh = curimage.getHeight(this);
	    if (imgw < 0 || imgh < 0) {
		return;
	    }
	}
	if (scalew < 0) {
	    scalew = (int) (imgw * hmult);
	    scaleh = (int) (imgh * hmult);
	}
	if (imgw != scalew || imgh != scaleh) {
	    g.drawImage(curimage, xadd, yadd,
				       scalew, scaleh, this);
	} else {
	    g.drawImage(curimage, xadd, yadd, this);
	}

    }

    long lastUpdate = 0;
    long updateRate = 100;

    public synchronized boolean imageUpdate(Image img, int infoflags,
					    int x, int y, int w, int h) {
	if (img != curimage) {
	    return false;
	}
	boolean ret = true;
	boolean dopaint = false;
	if ((infoflags & WIDTH) != 0) {
	    imgw = w;
	    dopaint = true;
	}
	if ((infoflags & HEIGHT) != 0) {
	    imgh = h;
	    dopaint = true;
	}
	if ((infoflags & ALLBITS) != 0) {
	    dopaint = true;
	    ret = false;
	}
	if ((infoflags & ERROR) != 0) {
	    ret = false;
	}
	if ((infoflags & SOMEBITS) != 0) {
	    long now = System.currentTimeMillis();
	    if ((now - lastUpdate) > updateRate) {
		dopaint = true;
		lastUpdate = now;
	    }
	}
	if (dopaint) {
	    repaint();
	}
	return ret;
    }

    public synchronized sun.awt.image.Image pickImage() {
	int index = alpha;
	if (usefilter) {
	    index += numalphas;
	}
	sun.awt.image.Image choice = imagevariants[index];
	if (choice == null) {
	    choice = imagevariants[alpha];
	    if (choice == null) {
		int alphaval = (alpha * 255) / (numalphas - 1);
		ImageFilter imgf = new AlphaFilter(alphaval);
		choice = (sun.awt.image.Image)Toolkit.getDefaultToolkit().createImage(new FilteredImageSource(origimage.getSource(),
							      imgf));
		imagevariants[alpha] = choice;
	    }
	    if (usefilter) {
		if (colorfilter == null) {
		    colorfilter = new RedBlueSwapFilter();
		}
		choice = (sun.awt.image.Image)Toolkit.getDefaultToolkit().createImage(new FilteredImageSource(choice.getSource(),
									  colorfilter));
	    }
	    imagevariants[index] = choice;
	}
	curimage = choice;
	return choice;
    }

    public synchronized boolean handleEvent(Event e) {
	switch (e.id) {
	  case Event.KEY_ACTION:
	  case Event.KEY_PRESS:
	    switch (e.key) {
	      case Event.HOME:
		usefilter = !usefilter;
		pickImage();
		repaint();
		return true;
	      case Event.UP:
		yadd -= 5;
		repaint();
		return true;
	      case Event.DOWN:
		yadd += 5;
		repaint();
		return true;
	      case Event.RIGHT:
	      case 'r':
		xadd += 5;
		repaint();
		return true;
	      case Event.LEFT:
		xadd -= 5;
		repaint();
		return true;
	      case Event.PGUP:
		if ((e.modifiers & Event.SHIFT_MASK) != 0) {
		    if (++alpha > numalphas - 1) {
			alpha = numalphas - 1;
		    }
		    pickImage();
		} else {
		    hmult *= 1.2;
		}
		scalew = scaleh = -1;
		repaint();
		return true;
	      case Event.PGDN:
		if ((e.modifiers & Event.SHIFT_MASK) != 0) {
		    if (--alpha < 0) {
			alpha = 0;
		    }
		    pickImage();
		} else {
		    hmult /= 1.2;
		}
		scalew = scaleh = -1;
		repaint();
		return true;
	      default:
		return false;
	    }

	  default:
	    return false;
	}
    }

}

class RedBlueSwapFilter extends RGBImageFilter {
    public RedBlueSwapFilter() {
	canFilterIndexColorModel = true;
    }

    public void setColorModel(ColorModel model) {
	if (model instanceof DirectColorModel) {
	    DirectColorModel dcm = (DirectColorModel) model;
	    int rm = dcm.getRedMask();
	    int gm = dcm.getGreenMask();
	    int bm = dcm.getBlueMask();
	    int am = dcm.getAlphaMask();
	    int bits = dcm.getPixelSize();
	    dcm = new DirectColorModel(bits, bm, gm, rm, am);
	    substituteColorModel(model, dcm);
	    consumer.setColorModel(dcm);
	} else {
	    super.setColorModel(model);
	}
    }

    public int filterRGB(int x, int y, int rgb) {
	return ((rgb & 0xff00ff00)
		| ((rgb & 0xff0000) >> 16)
		| ((rgb & 0xff) << 16));
    }
}

class AlphaFilter extends RGBImageFilter {
    ColorModel origmodel;
    ColorModel newmodel;

    int alphaval;

    public AlphaFilter(int alpha) {
	alphaval = alpha;
	canFilterIndexColorModel = true;
    }

    public int filterRGB(int x, int y, int rgb) {
	return ((rgb & 0x00ffffff)
		| (alphaval << 24));
    }
}
