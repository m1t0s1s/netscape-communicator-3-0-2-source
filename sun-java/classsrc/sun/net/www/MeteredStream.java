/*
 * @(#)MeteredStream.java	1.13 95/02/07 Chris Warth
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
import java.util.*;
import java.io.*;
import sun.net.ProgressData;

public 
class MeteredStream extends FilterInputStream 
{
    // Class variables.
    private static int	total_need;	
    private static int	total_read;	
    private static int	total_connections = 0;

    // Instance variables.
    int expected;
    int	count = 0;
    public URL url = null;
   
    private synchronized static void globalJustRead(int n) {
	total_read += n;
    }

    private synchronized static void updateExpected(int est) {
	total_need += est;
    }

    private synchronized static void addConnection(MeteredStream s) {
	total_connections += 1;
    }

    private synchronized static void removeConnection(MeteredStream s) {
	total_connections -= 1;
	total_read -= s.expected;
	total_need -= s.expected;
    }

    /*
     * Provide read-only access the static variables of interest to
     * observers.  It is expected that these methods are accessed
     * with the static monitor locked.  This is usually the case
     * because the notification to observers happens through the
     * synchronized method checkUpdate().
     */
    public static synchronized 
    ProgressReport checkProgress(ProgressReport pr) {
	return pr.set(total_read, total_need, total_connections);
    }

    public MeteredStream(InputStream is, int estimate, URL u) {
	this(is, estimate);
	this.url = u;
    }

    public MeteredStream(InputStream is, int estimate) {
	super(is);
	updateExpected(estimate);
	expected = estimate;
	addConnection(this);
    }

    private final void justRead(int n) {
	if (count + n > expected) {
	    n = expected - count;
	}
	count += n;
	globalJustRead(n);
	ProgressData.pdata.update(url, count, expected);
    }


    public int read() throws java.io.IOException {
	int c = super.read();
	if (c != -1) {
	    justRead(1);
	}
	return c;
    }

    /*
    public int read(byte b[]) 
	This routine is implemented in terms of read(byte b[], int off, int len)
	so we don't need to override it here.
    */

    public int read(byte b[], int off, int len) throws java.io.IOException {
	int n = super.read(b, off, len);
	if (n != -1) {
	    justRead(n);
	}
	return n;
    }
    
    public long skip(long n) throws IOException {
	n = super.skip(n);
	if (n != -1) {
	    justRead((int)n);
	}
	return n;
    }

    boolean closed = false;

    public void close() throws IOException {
	super.close();
	if (!closed) {
	    closed = true;
	    justRead(expected - count);
	    removeConnection(this);
	}
	ProgressData.pdata.unregister(url);
    }

    protected void finalize() {
	if (!closed) {
	    closed = true;
	    justRead(expected - count);
	    removeConnection(this);
	}
	ProgressData.pdata.unregister(url);
    }
}
