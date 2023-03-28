/*
 * @(#)URLImageSource.java	1.11 95/12/08 Jim Graham
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
import java.io.IOException;
import java.net.URL;
import java.net.URLConnection;
import java.net.MalformedURLException;

public class URLImageSource extends InputStreamImageSource {
    URL url;
    URLConnection conn;
    String actualHost;
    int actualPort;

    public URLImageSource(URL u) {
	SecurityManager sm = System.getSecurityManager();
	if (sm != null) {
	    sm.checkConnect(u.getHost(), u.getPort());
	}
	url = u;
    }

    public URLImageSource(String href) throws MalformedURLException {
	this(new URL(null, href));
    }

    public URLImageSource(URL u, URLConnection uc) {
	this(u);
	conn = uc;
    }

    public URLImageSource(URLConnection uc) {
	this(uc.getURL(), uc);
    }

    final boolean checkSecurity(Object context, boolean quiet) {
	// If actualHost is not null, then the host/port parameters that
	// the image was actually fetched from were different than the
	// host/port parameters the original URL specified for at least
	// one of the download attempts.  The original URL security was
	// checked when the applet got a handle to the image, so we only
	// need to check for the real host/port.
	if (actualHost != null) {
	    try {
		System.getSecurityManager().checkConnect(actualHost,
							 actualPort,
							 context);
	    } catch (SecurityException e) {
		if (!quiet) {
		    throw e;
		}
		return false;
	    }
	}
	return true;
    }

    private synchronized URLConnection getConnection() throws IOException {
	URLConnection c;
	if (conn != null) {
	    c = conn;
	    conn = null;
	} else {
	    c = url.openConnection();
	}
	return c;
    }

    protected ImageDecoder getDecoder() {
	InputStream is = null;
	String type = null;
	try {
	    URLConnection c = getConnection();
	    is = c.getInputStream();
	    type = c.getContentType();
	    URL u = c.getURL();
	    if (u != url && (u.getHost() != url.getHost() ||
			     u.getPort() != url.getPort()))
	    {
		// The image is allowed to come from either the host/port
		// listed in the original URL, or it can come from one other
		// host/port that the URL is redirected to.  More than that
		// and we give up and just throw a SecurityException.
		if (actualHost != null && (actualHost != u.getHost() ||
					   actualPort != u.getPort()))
		{
		    throw new SecurityException("image moved!");
		}
		actualHost = u.getHost();
		actualPort = u.getPort();
	    }
	} catch (IOException e) {
	    if (is != null) {
		try {
		    is.close();
		} catch (IOException e2) {
		}
	    }
	    return null;
	}

	ImageDecoder id = decoderForType(is, type);
	if (id == null) {
	    id = getDecoder(is);
	}
	return id;
    }
}
