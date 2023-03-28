/*
 * @(#)unknownHandler.java	1.9 95/08/22 James Gosling
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

/*-
 *	unknown stream opener
 */

package sun.net.www;

import java.io.*;
import java.net.UnknownServiceException;
import java.net.*;

/** open an unknown input stream given a URL */
class unknownHandler extends URLStreamHandler {
    String protocol;

    unknownHandler(String p) {
	protocol = p;
    }

    public InputStream openStream(URL u) throws UnknownServiceException {
	throw new UnknownServiceException(protocol);
    }

    public InputStream openStreamInteractively(URL u)
	throws UnknownServiceException {
	throw new UnknownServiceException(protocol);
    }

    public java.net.URLConnection openConnection(URL u)
	throws UnknownServiceException {
	throw new UnknownServiceException(protocol);
    }
}
