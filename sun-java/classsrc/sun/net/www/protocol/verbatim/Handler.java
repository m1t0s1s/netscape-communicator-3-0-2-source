/*
 * @(#)Handler.java	1.6 95/08/22 Arthur van Hoff
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

package sun.net.www.protocol.verbatim;

import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.*;

import sun.net.www.*;
import sun.net.www.protocol.http.*;
import sun.net.www.protocol.file.*;

public class Handler extends URLStreamHandler {
    public synchronized InputStream openStream(URL u) {
	String str = u.file.substring(1);
	try {
	    URL actualURL = new URL(str);
	    InputStream in = actualURL.openStream();
//REMIND: this is totally broken.  Need to be rewritten to cope with
//	URLConnections
//	    if (actualURL.content_type == "text/html") {
//		u.setType("text/plain");
//	    } else {
//		u.setType(actualURL.content_type);
//	    }
	    return in;
	} catch (Exception e) {
	    throw new MalformedURLException(str);
	}
    }
}

