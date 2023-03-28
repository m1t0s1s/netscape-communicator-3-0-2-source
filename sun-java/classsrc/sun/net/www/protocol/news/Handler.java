/*
 * @(#)Handler.java	1.12 95/08/22 James Gosling
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

package sun.net.www.protocol.news;

import java.io.*;
import java.util.*;
import sun.net.nntp.*;
import java.net.URL;
import java.net.URLStreamHandler;

/** open an nntp input stream given a URL */
public class Handler extends URLStreamHandler implements Runnable {
    static newsFetcher fetch;
    public InputStream openStream(URL u) {
	u.setType("text/html");
	PipedOutputStream os = new PipedOutputStream();
	PipedInputStream ret = new PipedInputStream();
	ret.connect(os);
	if (fetch == null)
	    fetch = new newsFetcher();
	fetch.fireUp(u.file, os);	/* create a thread to generate this
					 * article */
	return ret;
    }
}

