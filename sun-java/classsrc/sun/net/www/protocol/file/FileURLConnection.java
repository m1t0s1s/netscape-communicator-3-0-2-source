/*
 * @(#)FileURLConnection.java	1.3 95/11/14  
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

package sun.net.www.protocol.file;

import java.net.URL;
import java.io.*;
import sun.net.www.*;
import java.lang.SecurityManager;

/**
 * Open an file input stream given a URL.
 * @author	James Gosling
 * @version 	1.3, 14 Nov 1995
 */
public class FileURLConnection extends URLConnection {
    static String installDirectory;

    InputStream is;

    FileURLConnection(URL u) {
	super(u);
    }

    public void connect() throws IOException {
	String fn = url.getFile();
	MessageHeader props = new MessageHeader();
	String ctype;
	if (fn.endsWith("/")) {
	    ctype = "text/html";
	    fn = fn + "index.html";
	} else {
	    ctype = guessContentTypeFromName(fn);
	}
	if (ctype != null) {
	    props.add("content-type", ctype);
	}

	fn = fn.replace('/', File.separatorChar);

	String host = url.getHost();
	if (host.equals("~")) {
	    if (installDirectory == null) {
		SecurityManager.setScopePermission();
		installDirectory = System.getProperty("hotjava.home");
		SecurityManager.resetScopePermission();
		if (installDirectory == null) {
		    installDirectory = "/usr/local/hotjava".replace('/', File.separatorChar);
		}
	    }
	    fn = installDirectory + fn;
	}
	setProperties(props);
	is = new BufferedInputStream(new FileInputStream(fn));
    }

    public synchronized InputStream getInputStream()
	throws IOException
    {
	if (!connected) {
	    connect();
	}
	return is;
    }
}
