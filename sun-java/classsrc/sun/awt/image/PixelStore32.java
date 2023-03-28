/*
 * @(#)PixelStore32.java	1.4 95/08/18 Jim Graham
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

package sun.awt.image;

import java.awt.image.*;

public class PixelStore32 extends PixelStore {
    public PixelStore32() {
    }

    public PixelStore32(int w, int h) {
	super(w, h);
    }

    public PixelStore32(int w, int h, ColorModel cm) {
	setDimensions(w, h);
	setColorModel(cm);
    }

    public void setDimensions(int w, int h) {
	super.setDimensions(w, h);
    }

    Object allocateLines(int num) {
	return (Object) new int[num * width];
    }

    void replayLines(ImageConsumer ic, int i, int cnt, Object line) {
	ic.setPixels(0, i, width, cnt, colormodel,
		     (int[]) line, offsets[i], width);
    }
}
