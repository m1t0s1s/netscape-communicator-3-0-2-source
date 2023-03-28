/*
 * @(#)ProgressEntry.java	1.7 95/11/04 Chris Warth
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
package sun.net;

import java.net.URL;

public class ProgressEntry {
    public static final int HTML  = 0;
    public static final int IMAGE = 1;
    public static final int CLASS = 2;
    public static final int AUDIO = 3;
    public static final int OTHER = 4;

    public int need = 0;
    public int read = 0;
    public boolean connected = false;

    // Label for this entry.
    public String label;

    public int type;

    public Object key;

    ProgressEntry(Object o, String l, String ctype) {
	key = o;
	label = l;
	type = IMAGE;
	
	setType(l, ctype);
    }


    /*
     * Usually called when a URL is finally connected after the
     * content type is known.
     */
    public void setType(String l, String ctype) {
	type = OTHER;
	if (ctype != null) {
	    if (ctype.startsWith("image")) {
		type = IMAGE;
	    } else if (ctype.startsWith("audio")) {
		type = AUDIO;
	    } else if (ctype.equals("application/java-vm")) {
		type = CLASS;
	    } else if (ctype.startsWith("text/html")) {
		type = HTML;
	    }
	}
	if (type == OTHER) {
	    if (l.endsWith(".gif") || l.endsWith(".xbm")
		|| l.endsWith(".jpeg") || l.endsWith(".jpg")
		|| l.endsWith(".jfif")) {
		type = IMAGE;
	    } else if (l.endsWith(".au")) {
		type = AUDIO;
	    } else if (l.endsWith(".class")) {
		type = CLASS;
	    } else if (l.endsWith(".html") || l.endsWith("/")) {
		type = HTML;		
	    }
	}
    }

    public void update(int total_read, int total_need) {
	if (need == 0) {
	    need = total_need;
	}
	read = total_read;
    }

/*
    this returns the previous value of the connected boolean.
    typical usage as found in Progressdata is something like
		if (te.connected() == false) {
		    lastchanged = i;
		    setChanged();
		    notifyObservers();
		}

*/
    public synchronized boolean connected() {
	if (!connected) {
	    connected = true;    
	    return false;
	} 
	return true;
    }
}
