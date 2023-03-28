/*
 * @(#)CachedFile.java	1.2 95/08/22
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights reserved
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file copyright.html
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

package sun.net.www.httpd;
import java.io.*;
import sun.net.www.MessageHeader;

/**
 * A class to represent a file in the RAM cache.
 * @author  James Gosling
 */

class CachedFile {
    private byte body[];
    public MessageHeader mh;
    CachedFile(InputStream src, int size, MessageHeader fmh) {
	body = new byte[size];
	int pos = 0;
	int pending = size;
	while (pending > 0) {
	    int nr = src.read(body, pos, size);
	    if (nr < 0)
		break;
	    pending -= nr;
	    pos += nr;
	}
	if (pending != 0) {
System.out.print("Expected "+size+", missed by "+pending+"\n");
	    throw new IOException();
	}
	mh = fmh;
    }
    void sendto(OutputStream os) {
	os.write(body, 0, body.length);
    }
    void headerto(PrintStream ps) {
	if (mh != null)
	    mh.print(ps);
    }
    int length() {
	return body.length;
    }
}

