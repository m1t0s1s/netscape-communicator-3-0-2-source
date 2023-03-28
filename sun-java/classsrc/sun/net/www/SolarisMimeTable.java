/*
 * @(#)SolarisMimeTable.java	1.10 95/08/22  
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

package sun.net.www;

import java.net.URL;
import java.io.*;
import java.lang.SecurityManager;

/** OS dependent class to find mime description files */
class SolarisMimeTable extends MimeTable {
    SolarisMimeTable() {
	InputStream is = null;
	SecurityManager.setScopePermission();
	String slist[] = {
	    System.getProperty("user.mailcap"),
	    System.getProperty("user.home") + "/.mailcap",
	    "/etc/mailcap",
	    "/usr/etc/mailcap",
	    "/usr/local/etc/mailcap",
	    System.getProperty("hotjava.home", "/usr/local/hotjava") + "/lib/mailcap"
	};
	SecurityManager.resetScopePermission();
	for (int i = 0; i < slist.length; i++) {
	    if (slist[i] != null) {
		try {
		    is = new FileInputStream(slist[i]);
		    break;
		} catch(Exception e) {
		}
	    }
	}
	if (is != null) {
	    try {
	    ParseMailcap(is);
	    is.close();
	    } catch(IOException e) {}
	}
	add(new MimeEntry ("application/postscript", "imagetool %s; rm %s"));
	add(new MimeEntry ("application/x-dvi", "xdvi %s"));
	add(new MimeEntry ("application/x-troff", "xterm -title troff -e sh -c \"nroff %s | col | more -w ; rm %s\""));
	add(new MimeEntry ("application/x-troff-man", "xterm -title troff -e sh -c \"nroff -man %s | col | more -w ; rm %s\""));
	add(new MimeEntry ("application/x-troff-me", "xterm -title troff -e sh -c \"nroff -me %s | col | more -w ; rm %s\""));
	add(new MimeEntry ("application/x-troff-ms", "xterm -title troff -e sh -c \"nroff -ms %s | col | more -w ; rm %s\""));
	add(new MimeEntry ("video/mpeg", "mpeg_play %s; rm %s"));
	add(new MimeEntry ("application/x-tar", "loadtofile"));
	add(new MimeEntry ("application/x-gtar", "loadtofile"));
	add(new MimeEntry ("application/x-hdf", "loadtofile"));
	add(new MimeEntry ("application/x-netcdf", "loadtofile"));
	add(new MimeEntry ("application/x-shar", "loadtofile"));
	add(new MimeEntry ("application/x-sv4cpio", "loadtofile"));
	add(new MimeEntry ("application/x-sv4crc", "loadtofile"));
	add(new MimeEntry ("application/zip", "loadtofile"));
	add(new MimeEntry ("application/x-bcpio", "loadtofile"));
	add(new MimeEntry ("application/x-cpio", "loadtofile"));
	add(new MimeEntry ("application/octet-stream", "loadtofile"));
	add(new MimeEntry ("application/x-ustar", "loadtofile"));
	add(new MimeEntry ("audio/", "audiotool %s"));
	add(new MimeEntry ("image/", "xv %s; rm %s"));
    }

    String TempTemplate() {
	return "/tmp/%s.wrt";
    }
}
