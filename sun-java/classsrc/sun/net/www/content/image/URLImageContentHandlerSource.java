/*
 * @(#)URLImageContentHandlerSource.java	1.1 95/11/15  James Gosling
 * 
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for NON-COMMERCIAL purposes and without fee is hereby
 * granted provided that this copyright notice appears in all copies. Please
 * refer to the file "copyright.html" for further important copyright and
 * licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

package sun.net.www.content.image;

import java.net.URL;
import java.net.URLConnection;
import java.net.*;
import sun.awt.image.*;
import java.io.InputStream;
import java.io.IOException;

class URLImageContentHandlerSource extends InputStreamImageSource {
    URL url;
    URLConnection conn;

    public URLImageContentHandlerSource (URLConnection uc) {
	url = uc.getURL();
	conn = uc;
    }

    private synchronized URLConnection getConnection() throws IOException {
	URLConnection c = conn;
	if (c != null) {
	    conn = null;
	} else {
	    c = url.openConnection();
	}
	return c;
    }

    protected synchronized ImageDecoder getDecoder() {
	try {
	    URLConnection c = getConnection();
	    String type = c.getContentType();
	    ImageDecoder id = null;
	    InputStream is = c.getInputStream();

	    id = new sun.awt.image.GifImageDecoder(this, is);
	    if (id == null) {
		id = getDecoder(is);
	    }
	    return id;
	} catch(IOException e) {
	    return null;
	}
    }
}
