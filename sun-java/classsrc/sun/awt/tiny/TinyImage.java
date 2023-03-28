/*
 * @(#)TinyImage.java	1.4 95/12/09 Arthur van Hoff
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

import java.awt.Graphics;
import java.awt.image.ImageProducer;

class TinyImage extends sun.awt.image.Image {
    /**
     * Construct an image for offscreen rendering to be used with a
     * given Component.
     */
    public TinyImage(int w, int h) {
	super(w, h);
    }

    /**
     * Construct an image from an ImageProducer object.
     */
    public TinyImage(ImageProducer producer) {
	super(producer);
    }

    public Graphics getGraphics() {
	return new TinyGraphics(this);
    }

    protected sun.awt.image.ImageRepresentation getImageRep(int w, int h) {
	return super.getImageRep(w, h);
    }
}
