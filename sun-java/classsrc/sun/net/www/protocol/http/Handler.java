/*
 * @(#)Handler.java	1.41 95/12/08 James Gosling
 * 
 * Copyright (c) 1994-95 Sun Microsystems, Inc. All Rights Reserved.
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

/*-
 *	HTTP stream opener
 */

package sun.net.www.protocol.http;

import java.net.URL;

/** open an http input stream given a URL */
public class Handler extends java.net.URLStreamHandler {
    String proxy;
    int proxyPort;

    public Handler () {
	proxy = null;
	proxyPort = -1;
    }

    public Handler (String proxy, int port) {
	this.proxy = proxy;
	this.proxyPort = port;
    }

    protected java.net.URLConnection openConnection(URL u) {
	return new HttpURLConnection(u, this);
    }
}
