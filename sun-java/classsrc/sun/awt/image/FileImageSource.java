/*
 * @(#)FileImageSource.java	1.8 95/12/08 Jim Graham
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

import java.io.InputStream;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.FileNotFoundException;

public class FileImageSource extends InputStreamImageSource {
    String imagefile;

    public FileImageSource(String filename) {
	System.getSecurityManager().checkRead(filename);
	imagefile = filename;
    }

    final boolean checkSecurity(Object context, boolean quiet) {
	// File based images only ever need to be checked statically
	// when the image is retrieved from the cache.
	return true;
    }

    protected ImageDecoder getDecoder() {
	InputStream is;
	try {
	    is = new BufferedInputStream(new FileInputStream(imagefile));
	} catch (FileNotFoundException e) {
	    return null;
	}
	int suffixpos = imagefile.lastIndexOf('.');
	if (suffixpos >= 0) {
	    String suffix = imagefile.substring(suffixpos+1).toLowerCase();
	    if (suffix.equals("gif")) {
		return new GifImageDecoder(this, is);
	    } else if (suffix.equals("jpeg") || suffix.equals("jpg") ||
		       suffix.equals("jpe") || suffix.equals("jfif")) {
		return new JPEGImageDecoder(this, is);
	    } else if (suffix.equals("xbm")) {
		return new XbmImageDecoder(this, is);
	    }
	}
	return getDecoder(is);
    }
}
