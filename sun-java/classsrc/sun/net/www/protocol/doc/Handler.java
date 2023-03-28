/*
 * @(#)Handler.java	1.14 95/11/16 Sami Shaio
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

/*-
 *	doc urls point either into the local filesystem or externally
 *      through an http url.
 */

package sun.net.www.protocol.doc;

import java.net.URL;
import java.net.URLConnection;
import java.net.MalformedURLException;
import java.net.URLStreamHandler;
import java.io.InputStream;
import java.io.IOException;
import java.lang.SecurityManager;

public class Handler extends URLStreamHandler {
    static URL base;

    /*
     * Attempt to find a load the given url using the local
     * filesystem. If that fails, then try opening using http.
     */
    public synchronized URLConnection openConnection(URL u)
	throws IOException
    {
	URLConnection uc;
	URL ru;
	String file = u.getFile();
	try {
	    ru = new URL("file", "~", file);

	    uc = ru.openConnection();
	    InputStream is = uc.getInputStream();	// Check for success.
	} catch (MalformedURLException e) {
	    uc = null;
	} catch (IOException e) {
	    uc = null;
	}
	if (uc == null) {
	    try {
		if (base == null) {
		    SecurityManager.setScopePermission();
		    String doc_url = System.getProperty("doc.url");
		    SecurityManager.resetScopePermission();
		    base = new URL(doc_url);
		}
		ru = new URL(base, file);
	    } catch (MalformedURLException e) {
		ru = null;
	    }
	    if (ru != null) {
		uc = ru.openConnection();
	    }
	}
	if (uc == null) {
	    throw new IOException("Can't find file for URL: "
				  +u.toExternalForm());
	}
	return uc;
    }
}

