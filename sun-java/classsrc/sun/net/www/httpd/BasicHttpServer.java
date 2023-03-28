/*
 * @(#)BasicHttpServer.java	1.8 95/08/23 James Gosling
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
package sun.net.www.httpd;

import sun.net.NetworkServer;
import sun.net.www.MessageHeader;
import java.net.URL;
import java.io.*;
import java.net.InetAddress;

/**
 * This is the base class for http servers.  To define a new type
 * of server define a new subclass of BasicHttpServer with a getRequest
 * method that services one request.  Start the server by executing:
 * <pre>
 *	new MyServerClass().startServer(port);
 * </pre>
 */
public class BasicHttpServer extends NetworkServer {

    /** The mime header from the request. */
    MessageHeader mh;
    static int totalConnections;

    static URL defaultContext;

    /** True iff the client expects a mime header (ie. HTTP/1.n) */
    boolean expectsMime;

    /** Satisfy one get request.  It is invoked with the clientInput and
	clientOutput streams initialized.  This method handles one client
	connection. When it is done, it can simply exit. The default
	server just echoes it's input. */
    protected void getRequest(URL u, String param) {
	try {
	    if (u.getFile().equals("/statistics.html"))
	    	generateStatistics();
	    else if (u.getFile().equals("/processes.html"))
	    	generateProcessOutput("Running processes", "/usr/ucb/ps uaxwww");
	    else if (u.getFile().equals("/uptime.html"))
	        generateProcessOutput("System uptime", "/usr/ucb/uptime");
	    else if (u.getFile().equals("/sin.dat")) {
		if (expectsMime) {
		    clientOutput.print("HTTP/1.0 200 Document follows\n" +
			      "Server: Java/" + getClass().getName() + "\n" +
				       "Content-type: application/chart\n\n");
		}
		for (int i = 0; i<10000; i++) {
		    clientOutput.print(Math.sin(i/20.0)+"\n");
		    clientOutput.flush();
		    Thread.sleep(200);
		}
	   }
	   else if (u.getFile().equals("/stock.dat")) {
		if (expectsMime) {
		    clientOutput.print("HTTP/1.0 200 Document follows\n" +
			      "Server: Java/" + getClass().getName() + "\n" +
				       "Content-type: application/chart\n\n");
		}
		double stock = 0;
		for (int i = 0; i<10000; i++) {
		    clientOutput.print(stock+"\n");
		    clientOutput.flush();
		    stock+=Math.random()-0.5;
		    if (stock > 20) stock = stock-Math.random();
		    if (stock < -20) stock = stock+Math.random();
		    Thread.sleep(900);
		}
	   }
	   else if (u.getFile().equals("/frac.dat")) {
		if (expectsMime) {
		    clientOutput.print("HTTP/1.0 200 Document follows\n" +
			      "Server: Java/" + getClass().getName() + "\n" +
				       "Content-type: application/chart\n\n");
		}

		
		double stock = 0;
		for (int i = 0; i<10000; i++) {
		    clientOutput.print(stock+"\n");
		    clientOutput.flush();
		    stock+=(Math.random()-0.5)*Math.sin(i*30.0);
		    if (stock > 20) stock = stock-Math.random();
		    if (stock < -20) stock = stock+Math.random();
		    Thread.sleep(200);
		}
	    }
	   else if (u.getFile().equals("/echo.html")) {
	       startHtml("Echo reply");
	       clientOutput.print("<p>URL was " + u.toExternalForm() + "\n");
	       clientOutput.print("<p>Socket was " + clientSocket + "\n<p><pre>");
	       mh.print(clientOutput);
	   } else {
		   String fn = u.getFile();
		   if (fn.endsWith("/"))
		       fn = fn + "index.html";
		   if (fn.startsWith("/~")) {
		       int sl2 = fn.indexOf('/', 2);
		       if (sl2 < 0) sl2 = fn.length();
		       fn = "/home/"+fn.substring(2,sl2)+"/public_html"+fn.substring(sl2);
		   } else
		       fn = "/net/tachyon/export/disk1/Mosaic/docs/" + fn;
		   InputStream is = new FileInputStream(fn);
		   if (expectsMime) {
		       String ct = "text/plain";
		       if (fn.endsWith(".html"))
			   ct = "text/html";
		       else if (fn.endsWith(".gif"))
			   ct = "image/gif";
		       clientOutput.print("HTTP/1.0 200 Document follows\n" +
					  "Server: Java/" + getClass().getName() + "\n" +
					  "Content-type: " + ct + "\n\n");
		   }
		   int nb;
		   byte buf[] = new byte[2048];
		   while ((nb = is.read(buf)) >= 0)
		       clientOutput.write(buf, 0, nb);
		   is.close();
	    }

	} catch(Exception e) {
	    System.out.print("Failed on "+u.getFile()+"\n");
	    e.printStackTrace();
	}
    }

    /** Start generating an html reply to a get request.  This is a
        convenience method to simplify getRequest */
    protected void startHtml(String title) {
	if (expectsMime)
	    clientOutput.print("HTTP/1.0 200 Document follows\n" +
			       "Server: Java/" + getClass().getName() + "\n" +
			       "Content-type: text/html\n\n");
	clientOutput.print("<html><head><title>" + title +
			   "</title></head>\n<body><h1>" +
			   title + "</h1>\n");
    }

    /** Generate an html reply to a get request that contains a
	dump of the statistics for the current server.  getRequest
	handlers can call it in response to special file names. */
    protected void generateStatistics() {
	startHtml("Server statistics");
	clientOutput.print(totalConnections + " total connections.<p>\n");
    }

    /** Generate an html reply to a get request that contains the
	output after executing a system command.  getRequest
	handlers can call it in response to special file names. */
    protected void generateProcessOutput(String title, String command) {
	startHtml(title);
	try {
	    InputStream in = new BufferedInputStream(Runtime.getRuntime().exec(command).getInputStream());
	    clientOutput.print("<pre>\n");
	    int c;
	    while ((c = in.read()) >= 0)
		switch (c) {
		  case '<':
		    clientOutput.print("&lt;");
		    break;
		  case '&':
		    clientOutput.print("&amp;");
		    break;
		  default:
		    clientOutput.write(c);
		    break;
		}
	    in.close();
	    clientOutput.print("</pre>\n");
	} catch(Exception e) {
	    clientOutput.print("Failed to execute " + command + "\n");
	}
    }

    protected void error(String msg) {
	if (expectsMime)
	    clientOutput.print("HTTP/1.0 403 Error - msg\n" +
			       "Content-type: text/html\n\n");
	clientOutput.print("<html><head>Error</head><body>" +
			   "<H1>Error when fetching document:</h1>\n" +
			   msg + "\n");
	if (mh != null) {
	    clientOutput.print("<p><pre>");
	    mh.print(clientOutput);
	}
    }

    final public void serviceRequest() {
	totalConnections++;
	try {
	    mh = new MessageHeader(clientInput);
	    String cmd = mh.findValue(null);
	    if (cmd == null) {
		error("Missing command " + mh);
		return;
	    }
	    int fsp = cmd.indexOf(' ');
	    if (fsp < 0) {
		error("Syntax error in command: " + cmd);
		return;
	    }
	    String k = cmd.substring(0, fsp);
	    int nsp = cmd.indexOf(' ', fsp + 1);
	    String p1, p2;
	    if (nsp > 0) {
		p1 = cmd.substring(fsp + 1, nsp);
		p2 = cmd.substring(nsp + 1);
	    } else {
		p1 = cmd.substring(fsp + 1);
		p2 = null;
	    }
	    expectsMime = p2 != null;
	    if (k.equalsIgnoreCase("get"))
		getRequest(new URL(defaultContext, p1), p2);
	    else {
		error("Unknown command: " + k + " (" + cmd + ")");
		return;
	    }
	} catch(IOException e) {
	    // totally ignore IOException.  They're usually client crashes.
	} catch(Exception e) {
	    error("Exception: " + e);
	    e.printStackTrace();
	}
    }

    public BasicHttpServer () {
	try {
	    defaultContext 
	       = new URL("http", InetAddress.getLocalHost().getHostName(), "/");
	} catch(Exception e) {
	    System.out.print("Failed to construct defauit URL context: "+e+"\n");
	    e.printStackTrace();
	}
    }

    public static void main(String argv[]) {
	try {	
	    new BasicHttpServer ().startServer(8888);
	} catch(Exception e) {
	    System.out.print("Failed to start the server: "+e+"\n");
	    e.printStackTrace();
	}	
    }
}
