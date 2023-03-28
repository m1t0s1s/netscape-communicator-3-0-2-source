/*
 * @(#)Server.java	1.9 95/08/22 James Gosling
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
import java.net.InetAddress;
import sun.net.www.MessageHeader;
import java.net.URL;
import java.io.*;
import java.util.Hashtable;
import java.misc.Cache;

/**
 * This is the base class for http servers.  To define a new type
 * of server define a new subclass of Server with a getRequest
 * method that services one request.  Start the server by executing:
 * <pre>
 *	new MyServerClass().startServer(port);
 * </pre>
 */
public class Server extends NetworkServer {

    /** The mime header from the request. */
    MessageHeader mh;
    static int totalConnections;

    static URL defaultContext;
    ServerParameters params;

    Cache fileCache = new Cache(1000);

    /** True iff the client expects a mime header (ie. HTTP/1.n) */
    boolean expectsMime;

    Server (String argv[]) {
	params = new ServerParameters(getClass().getName(), argv);
	if (params.verbose)
	    params.print();
    }

    /** Satisfy one get request.  It is invoked with the clientInput and
	clientOutput streams initialized.  This method handles one client
	connection. When it is done, it can simply exit. */
    protected void getRequest(URL u, String param) {
	try {
	    String fn = u.file;
	    if (fn.endsWith("/"))
		fn = fn + params.welcome;
	    InputStream is = null;
	    int size = 0;
	    is = new FileInputStream(fn);
	    size = is.available();
	    String ct = params.mimeType(fn);
	    CachedFile cf = null;
	    MessageHeader mh = new MessageHeader();
	    mh.add("Content-type", ct);
	    mh.add("Server", params.serverName);
	    mh.add("Content-length", String.valueOf(size));
	    if (size <= params.maxRamCacheEntryBuffer) {
		try {
		    cf = new CachedFile(is, size, mh);
		    fileCache.put(u, cf);
		    if (params.verbose)
			System.out.print(u.toExternalForm() + ": Into RAM cache\n");
		} catch(Exception e) {
		    if (params.verbose)
			e.printStackTrace();
		}
	    } else if (params.verbose)
		System.out.print(u.toExternalForm() + ": from disk (too big for RAM cache: " + size + ")\n");
	    if (expectsMime) {
		clientOutput.print("HTTP/1.0 200 Document follows\n");
		mh.print(clientOutput);
	    }
	    if (cf != null)
		cf.sendto(clientOutput);
	    else {
		int nb;
		byte buf[] = new byte[2048];
		while ((nb = is.read(buf)) >= 0)
		    clientOutput.write(buf, 0, nb);
	    }
	    if (is != null)
		is.close();
	} catch(Exception e) {
	    if (params.verbose)
		e.printStackTrace();
	    error("Can't read " + u.file);
	}
    }

    private static File TrashFile;	// File to rename to instead of
    // removing files

    private void remove(File f) {
	if (TrashFile == null)
	    TrashFile = new File(params.CacheRoot + "/trash");
	try {
	    f.renameTo(TrashFile);
	} catch(Exception e) {
	    System.out.print("Couldn't remove " + f + "\n");
	}
    }

    /** Satisfy one get request where this host is acting as a proxy.
	It is invoked with the clientInput and
	clientOutput streams initialized.  This method handles one client
	connection. When it is done, it can simply exit. The default
	server just echoes it's input. */
    protected void getProxyRequest(URL u, String param) {
	InputStream is = null;
	OutputStream os = null;
	MessageHeader mh = null;
	boolean tryCache = params.Caching && u.canCache();
	String ff = u.toExternalForm();
	File CacheFile = new File(params.CacheRoot, u.toExternalForm()
				  .replace('/', File.separatorChar) + "+");

	CachedFile cf = null;
	if (tryCache) {
	    try {
		is = new BufferedInputStream(new FileInputStream(CacheFile));
		if (params.verbose)
		    System.out.print(u.toExternalForm() + ": Found in proxy cache\n");
		tryCache = false;
		mh = new MessageHeader(is);
		u.content_type = mh.findValue("content-type");
		/* we have it: try to put it in the RAM cache */
		int size = is.available();
		mh.set("Content-size", String.valueOf(size));
		if (size <= params.maxRamCacheEntryBuffer) {
		    try {

			/*
			 * slurp it into the RAM cache, then shoot it out
			 * right away
			 */
			cf = new CachedFile(is, size, mh);
			fileCache.put(u, cf);
			if (params.verbose)
			    System.out.print(u.toExternalForm() + ": Proxy cached file into RAM cache\n");
			if (expectsMime) {
			    clientOutput.print("HTTP/1.0 200 Document follows\n");
			    mh.print(clientOutput);
			}
			cf.sendto(clientOutput);
			is.close();
			return;
		    } catch(Exception e) {
			if (params.verbose)
			    e.printStackTrace();
		    }
		} else if (params.verbose)
		    System.out.print(u.toExternalForm() + ": Too big for RAM cache (" + size +
			       "<" + params.maxRamCacheEntryBuffer + ")\n");
	    } catch(Exception e) {
		if (is != null) {
		    e.printStackTrace();
		    is.close();
		}
		is = null;
	    }
	}
	if (is == null)
	    if (params.CacheNoConnect) {
		if (params.verbose)
		    System.out.print("Proxy cache missed with remote access disabled " + u.toExternalForm() + "\n");
		if (expectsMime)
		    clientOutput.print("HTTP/1.0 404 not found (remote access disabled)\n" +
				       "Server: " + params.serverName +
				       "\nContent-type: text/html" +
				       "\n\n");
		clientOutput.print("<html><body><h1>File not found in cache, remote access disabled.</h1>\n"
				   + u.toExternalForm() + "\n");
		return;
	    } else {
		try {
		    is = u.openStream();
		    if (params.verbose)
			System.out.print(u.toExternalForm() + ": Proxy get.\n");
		} catch(FileNotFoundException e) {
		    if (params.verbose)
			System.out.print(u.toExternalForm() + ": Proxy get failed.\n");
		    if (expectsMime)
			clientOutput.print("HTTP/1.0 404 not found\n" +
					   "Server: " + params.serverName +
					   "\nContent-type: text/html" +
					   "\n\n");
		    clientOutput.print("<html><body><h1>File not found</h1>\n"
				       + u.toExternalForm() + "\n");
		    return;
		}
	    }
	if (tryCache) {
	    try {
		try {
		    os = new BufferedOutputStream(new FileOutputStream(CacheFile));
		} catch(Exception e) {
		    new File(CacheFile.getParent()).mkdirs();
		    os = new BufferedOutputStream(new FileOutputStream(CacheFile));
		}
	    } catch(Exception e) {
	    }
	}
	if (os != null)
	    if (mh != null)
		mh.print(new PrintStream(os));
	    else
		new PrintStream(os).print("Content-type: " + u.content_type + "\n\n");
	if (expectsMime) {
	    clientOutput.print("HTTP/1.0 200 Document follows\n");
	    if (mh != null)
		mh.print(clientOutput);
	    else
		clientOutput.print("Server: " + params.serverName +
				   "\nContent-type: " + u.content_type +
				   "\n\n");
	}
	byte buf[] = new byte[2048];
	int nb;
	int nWritten = 0;
	while ((nb = is.read(buf)) >= 0) {
	    if (os != null) {
		try {
		    os.write(buf, 0, nb);
		    nWritten += nb;
		} catch(Exception e) {
		    if (params.verbose)
			System.out.print("Cache write to " + CacheFile + " failed (" + e + ")\n");
		    remove(CacheFile);
		    os.close();
		    os = null;
		}
	    }
	    clientOutput.write(buf, 0, nb);
	}
	if (os != null)
	    os.close();
	if (is != null)
	    is.close();
    }

    /** Start generating an html reply to a get request.  This is a
        convenience method to simplify getRequest */
    public void startHtml(String title)
    {
	if (expectsMime)
	    clientOutput.print("HTTP/1.0 200 Document follows\n" +
			       "Server: Java/" + params.serverName + "\n" +
			       "Content-type: text/html\n\n");
	clientOutput.print("<html><head><title>" + title +
			   "</title></head>\n<body><h1>" +
			   title + "</h1>\n");
    }

    /** Generate an html reply to a get request that contains a
        dump of the statistics for the current server.  getRequest
        handlers can call it in response to special file names. */
    protected void generateStatistics()
    {
	startHtml("Server statistics");
	clientOutput.print(totalConnections + " total connections.<p>\n");
    }

    /** Generate an html reply to a get request that contains the
        output after executing a system command.  getRequest
        handlers can call it in response to special file names. */
    public void generateProcessOutput(String title, String command)
    {
	startHtml(title);
	try {
	    InputStream in = new BufferedInputStream(Runtime.getRuntime().execin(command));
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

    public void error(String msg)
    {
	if (expectsMime)
	    clientOutput.print("HTTP/1.0 403 Error - " + msg + "\n" +
			       "Content-type: text/html\n\n");
	clientOutput.print("<html><head>Error</head><body>" +
			   "<H1>Error when fetching document:</h1>\n" +
			   msg + "\n");
	if (mh != null) {
	    clientOutput.print("<p><hr><pre>\n");
	    mh.print(clientOutput);
	    clientOutput.print("</pre>\n");
	}
    }

    final public void serviceRequest()
    {
	totalConnections++;
	if (defaultContext == null)
	    defaultContext = new URL("http", InetAddress.getLocalHost().getHostName(), "/");
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
	    if (k.equalsIgnoreCase("get")) {
		URL u = new URL(defaultContext, p1);
		CachedFile cf = (CachedFile) fileCache.get(u);
		if (cf != null) {
		    /* Found in RAM cache */
		    if (params.verbose)
			System.out.print(u.toExternalForm() + ": get from RAM cache\n");
		    if (expectsMime) {
			clientOutput.print("HTTP/1.0 200 Document follows\n");
			cf.headerto(clientOutput);
		    }
		    cf.sendto(clientOutput);
		} else if (u.protocol.equals(defaultContext.protocol) &&
			   u.host.equals(defaultContext.host)) {
		    /* Local request */
		    String fn = u.file;
		    Object r = params.applyRules(fn);
		    if (fn == null || fn == params.failObject)
			error("Access denied (by rule)");
		    else if (r instanceof String) {
			u.file = (String) r;
			getRequest(u, p2);
		    } else if (r instanceof ServerPlugin) {
			((ServerPlugin) r).getRequest(this, u);
		    } else {
			error("Can't cope with " + r + " yet\n");
		    }
		} else
		    getProxyRequest(u, p2);
	    } else {
		error("Unknown command: " + k + " (" + cmd + ")");
		return;
	    }
	} catch(IOException e) {
	    // totally ignore IOException.  Theyre usually client
	    // crashes.
	    error("IOException: " + e);
	} catch(Exception e) {
	    e.printStackTrace();
	    error("Exception: " + e);
	}
    }

    public static void main(String argv[])
    {
	Server s = new Server (argv);
	try {
	    s.startServer(s.params.port);
	} catch(Exception e) {
	    e.printStackTrace();
	}
    }
}
