/*
 * @(#)TransPerf.java	1.6 95/08/22
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
import java.net.URL;

/**
 * A class to measure the transaction performance of an http server.
 * @author  James Gosling
 */

class TransPerf implements Runnable {
    int ntrans;
    long totbytes;
    long totmillis;
    long totconnmillis;
    long stime;

    String protocol = "http";
    String host = "tachyon.eng";
    String port = "";
    String ufile = "files.list";
    String uprefix;

    String urls[] = new String[10];
    int nurls;
    int nproc = 20;

    synchronized void addSample(int nbytes, long nmillis, long cmillis) {
	totbytes += nbytes;
	ntrans += 1;
	totmillis += nmillis;
	totconnmillis += cmillis;
    }
    synchronized void clearStats() {
	ntrans = 0;
	totbytes = 0;
	totmillis = 0;
	totconnmillis = 0;
	stime = System.currentTimeMillis();
    }
    synchronized void printStats(PrintStream out) {
	long etime = System.currentTimeMillis();

	out.print("Results for "+uprefix+"   "+nproc+" threads\n");
	out.print(ntrans + "\ttotal transactions\n");
	out.print(totbytes / (1024. * 1024) + "\tMb transferred\n");
	out.print((etime - stime) / 1000 + "\tseconds elapsed\n");
	out.print((ntrans * 1000.) / (etime - stime) + "\ttransactions/second\n");
	out.print(totbytes / ((double) (etime - stime)) + "\tKb/second\n");
	out.print(totconnmillis / ntrans + "\taverage msec to first byte\n");
    }

    void doTransaction(String uname, byte inbuf[]) {
	URL u = new URL(null, uname);
	long T0 = System.currentTimeMillis();
	InputStream in = u.openStream();
	long T1 = System.currentTimeMillis();
	int nbytes = 0;
	int n = 0;
	while ((n = in.read(inbuf, 0, inbuf.length)) >= 0)
	    nbytes += n;
	in.close();
	long T2 = System.currentTimeMillis();
	addSample(nbytes, T2 - T0, T1 - T0);
    }
    public void run() {
	byte inbuf[] = new byte[8 * 1024];
	while (true) {
	    try {
		doTransaction(uprefix + urls[(int) (Math.random() * nurls)], inbuf);
	    } catch(Exception e) {
		e.printStackTrace();
	    }
	}
    }
    void runTest(String argv[]) {
	double t = 0.5;
	for (int i = 0; i < argv.length; i++) {
	    String s = argv[i];
	    try {
		if (s.equals("-t"))
		    t = Double.valueOf(argv[++i]).doubleValue();
		else if (s.equals("-n"))
		    nproc = Integer.parseInt(argv[++i]);
		else if (s.equals("-protocol"))
		    protocol = argv[++i];
		else if (s.equals("-h"))
		    host = argv[++i];
		else if (s.equals("-p"))
		    port = ":" + argv[++i];
		else if (s.equals("-f"))
		    ufile = ":" + argv[++i];
		else {
		    System.out.print("Illegal argument: " + s + "\n");
		    return;
		}
	    } catch(Exception e) {
		System.out.print("Error in argument: " + s + "\n");
		return;
	    }
	}
	uprefix = protocol + "://" + host + port;
	try {
	    InputStream in = new FileInputStream(ufile);
	    StreamTokenizer st = new StreamTokenizer(new BufferedInputStream(in));
	    st.resetSyntax();
	    st.wordChars(0, 0377);
	    st.whitespaceChars(0, ' ');
	    st.quoteChar('"');
	    st.quoteChar('\'');
	    while (st.nextToken() != StreamTokenizer.TT_EOF)
		if (st.sval != null) {
		    if (nurls >= urls.length) {
			String nu[] = new String[nurls * 2];
			System.arraycopy(urls, 0, nu, 0, urls.length);
			urls = nu;
		    }
		    urls[nurls++] = st.sval.charAt(0) == '/' ? st.sval : "/" + st.sval;
		}
	    in.close();
	} catch(Exception e) {
	    System.out.print("Error reading file list: " + ufile + "\n");
	    System.exit(-1);
	}
	clearStats();
	for (int i = nproc; --i >= 0;)
	    new Thread(this).start();
	Thread.sleep(60000 * t);
	printStats(System.out);
    }
    public static void main(String argv[]) {
	new TransPerf ().runTest(argv);
	System.exit(0);
    }
}
