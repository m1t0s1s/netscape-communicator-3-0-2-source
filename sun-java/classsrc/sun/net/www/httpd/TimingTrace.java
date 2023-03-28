/*
 * @(#)TimingTrace.java	1.3 95/08/22
 * 
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights reserved Permission to
 * use, copy, modify, and distribute this software and its documentation for
 * NON-COMMERCIAL purposes and without fee is hereby granted provided that
 * this copyright notice appears in all copies. Please refer to the file
 * copyright.html for further important copyright and licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

package sun.net.www.httpd;
import java.io.*;

/**
 * A class to maintain a trace of timing information through
 * an execution path.
 * @author  James Gosling
 */

public class TimingTrace {
    private long times[];
    private String terms[];
    private long t0;
    private int ntimes;
    String title;

    public TimingTrace (String t, int size) {
	times = new int[size];
	terms = new String[size];
	title = t;
	reset();
    }

    public void reset() {
	t0 = System.currentTimeMillis();
	ntimes = 0;
    }

    public void stamp(String s) {
	int n = ntimes;
	if (n < terms.length) {
	    times[n] = System.currentTimeMillis();
	    terms[n] = s;
	    ntimes = n + 1;
	}
    }

    static private void po(PrintStream s, int n, int width) {
	if (n <= 0) {
	    while (--width >= 0)
		s.write(' ');
	} else {
	    po(s, n / 10, width - 1);
	    s.write('0' + n % 10);
	}
    }
    static synchronized void print(PrintStream s, TimingTrace t) {
	int lt = t.t0;
	s.print("\nTiming title: " + t.title + "\n");
	for (int i = 0; i < t.ntimes; i++) {
	    int T = t.times[i];
	    po(s, T - lt, 6);
	    po(s, T - t.t0, 6);
	    s.print("  ");
	    lt = T;
	    s.print(t.terms[i]);
	    s.print("\n");
	}
    }
    public void print() {
	print(System.out);
    }
    public void print(PrintStream s) {
	print(s, this);
    }
}
