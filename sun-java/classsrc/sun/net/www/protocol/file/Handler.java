/*
 * @(#)Handler.java	1.19 95/08/22  
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
import java.net.URLConnection;
import java.net.URLStreamHandler;

/**
 * Open an file input stream given a URL.
 * @author	James Gosling
 * @version 	1.19, 22 Aug 1995
 */
public class Handler extends URLStreamHandler {
    static String installDirectory;

    public synchronized URLConnection openConnection(URL u) {
	return new FileURLConnection(u);
    }
}
