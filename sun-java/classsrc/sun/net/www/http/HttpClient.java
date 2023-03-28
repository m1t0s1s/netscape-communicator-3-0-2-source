/*
 * @(#)HttpClient.java	1.52 95/12/02 Jonathan Payne
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

package sun.net.www.http;

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.SecurityManager;
import sun.net.NetworkClient;
import sun.net.ProgressData;
import sun.net.TelnetInputStream;
import sun.net.www.MessageHeader;

/**
 * @version 1.47 22 Aug 1995
 * @author Jonathan Payne
 */
public final class HttpClient extends NetworkClient {
    static final boolean    debug = false;

    static final String	userAgentString = 
    	"User-Agent: " + getSystemProperty("http.agent", 
			  "Java" + System.getProperty("java.version")) + "\r\n";

    static final String acceptString
	= "Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2\r\n";

    /** Default port number for http daemons - this is not
        a registered service as far as I can tell. */
    static final int	httpPortNumber = 80;

    public static boolean   useProxyForFirewall = Boolean.getBoolean("firewallSet");
    public static String    firewallProxyHost = getSystemProperty("firewallHost");
    public static int	    firewallProxyPort = Integer.getInteger("firewallPort", 80).intValue();

    public static boolean   useProxyForCaching = Boolean.getBoolean("proxySet");
    public static String    cachingProxyHost = getSystemProperty("proxyHost", "sunweb.ebay");
    public static int	    cachingProxyPort = Integer.getInteger("proxyPort", 80).intValue();
    /* instance-specific proxy fields override the static fields if */
    /* set. */
    private String	    proxy = null;
    private int		    proxyPort = -1;

    /* request essentially succeeded */
    public static final int	OK = 200;
    public static final int	CREATED = 201;
    public static final int	ACCEPTED = 202;
    public static final int	PARTIAL = 203;

    /* document redirection */
    public static final int	MOVED = 301;
    public static final int	FOUND = 302;
    public static final int	METHOD = 303;

    /* errors */
    public static final int	BAD = 400;
    public static final int	UNAUTHORIZED = 401;
    public static final int	PAYMENT_REQUIRED = 402;
    public static final int	FORBIDDEN = 403;
    public static final int	NOT_FOUND = 404;
    public static final int	INTERNAL_ERROR = 500;
    public static final int	NOT_IMPLEMENTED = 501;

    int			status;
    MessageHeader	mimeHeader = null;
    boolean		usingProxy = false;
    String		host;
    String		authentication = null;


    /** Url being fetched. */
    URL			url;

    private static String getSystemProperty(String key) {
	SecurityManager.setScopePermission();
	return(System.getProperty(key));
    }

    private static String getSystemProperty(String key, String def) {
	SecurityManager.setScopePermission();
	return(System.getProperty(key, def));
    }

    public synchronized void openServer(String host, int port) throws UnauthorizedHttpRequestException, UnknownHostException, IOException {
	this.host = host;
	boolean AllIsWell = false;
	String   s;

//	ProgressData.pdata.register(url);
	try {

	    if (debug) {
		System.out.println("openServer " + host + ":" + port);
	    }
	    SecurityManager security = System.getSecurityManager();
	    if (security != null) {
		security.checkConnect(host, port);
	    }
	    /*
	    if ((s = Firewall.verifyAccess(host, port)) != null) {
		String msg = "Applet at " + s + " attempted illegal URL access: ";

		msg = msg + url.toExternalForm();
		Firewall.securityError(msg);
		return;
	    }
	    */
		
	    if (url.getProtocol().equals("http")) {
		if (useProxyForCaching) {
		    /* This would only fail if the specified host is
		       unknown to the proxy, e.g., a local host was
		       specified that isn't exported to the net which
		       the proxy is on.  In that case, we keep trying
		       just in case the host is known locally. */		
		    super.openServer(cachingProxyHost, cachingProxyPort);
		    usingProxy = true;
		    return;
		}
		try {
		    super.openServer(host, port);
		} catch (UnknownHostException e) {
		    if (useProxyForFirewall) {
			if (proxy == null) {
			    super.openServer(firewallProxyHost,firewallProxyPort);
			} else {
			    super.openServer(proxy, proxyPort);
			}
			usingProxy = true;
		    } else {
			throw e;
		    }
		}
	    } else {
		// we're opening some other kind of url, most likely an
		// ftp url.
		if (proxy == null) {
		    super.openServer(firewallProxyHost,firewallProxyPort);
		} else {
		    super.openServer(proxy, proxyPort);
		}
		usingProxy = true;
	    }
	    AllIsWell = true;
	} finally {
//	    if (!AllIsWell)
//		ProgressData.pdata.unregister(url);
	}
    }

    /** Parse the first line of the HTTP request.  It usually looks
	something like: "HTTP/1.0 <number> comment\r\n". */

    protected void getRequestStatus(String response, String name)
	throws UnauthorizedHttpRequestException, FileNotFoundException {
	if (debug) {
	    System.out.println(response);
	}
	if (response.startsWith("HTTP/1.0 ")) {
	    status = Integer.parseInt(response.substring(9, 12));
	} else {
	    status = -1;
	}
	switch (status) {
	case UNAUTHORIZED:
	    throw new UnauthorizedHttpRequestException(url, this);

	case BAD:
	case PAYMENT_REQUIRED:
	case INTERNAL_ERROR:
	case NOT_IMPLEMENTED:
//	    throw new Exception("HTTP request failed: " + response);
//	    fall through
	case FORBIDDEN:
	case NOT_FOUND:
	    //System.out.println("Throwing file not found");
	    throw new FileNotFoundException(response + " -- " + name);
	}
    }

    public String getURLFile(URL u) {
	if (usingProxy) {
	    String result = u.getProtocol() + "://" + u.getHost();
	    if (u.getPort() != -1) {
		result += ":" + u.getPort();
	    }
	    return result + u.getFile();
	} else {
	    return u.getFile();
	}
    }

    public void processRequest(String name)
	throws UnauthorizedHttpRequestException, IOException {
	/*
	 * Read until 200 bytes or EOF, or until we hit a blank
	 * line, whichever occurs first, looking for the string
	 * "HTTP/1.0" at the beginning of the line.  This is not
	 * strictly speaking necessary from the spec., but it is
	 * necessary since Netscape does it and therefore people
	 * have written their pages this way, broken as it is.  An
	 * example is mumble:80 which sometimes spews the line
	 * "Trying..." before returning a valid HTTP reply.
	 *
	 * In any case we always retain the first line of the
	 * response because it is useful if the server turns out to
	 * be a pre-HTTP1.0 server.
	 */

	BufferedInputStream bis = (BufferedInputStream) serverInput;
	String status_response = null;
	String firstLine = null;

	bis.mark(200);
	DataInputStream dis = new DataInputStream(new TelnetInputStream(serverInput, false));

	while (true) {
	    String  line = dis.readLine();

	    if (debug) {
		System.err.println((line == null) ? "<null>" : line);
	    }

	    if (firstLine == null) {
		if (line == null) {
		    throw new SocketException("Unexpected EOF");
		}
		firstLine = new String(line);
	    }
	    if (line == null || line.length() == 0) /* end of header */
		break;
	    if (line.startsWith("HTTP/1.0")) {
		status_response = new String(line);
		if (!firstLine.startsWith("HTTP/1.0")) {
		    System.err.println("Warning: The first line from "+url.toExternalForm()+" was not HTTP/1.0 compliant.");
		    System.err.println("         \""+firstLine+"\"");
		}
		break;
	    }
	}

	bis.reset();

	try {
	    try {
	    if (status_response != null) {
		byte buf[] = new byte[10];
		int count = 0;
		int c;

		/* Even if the request status indicates error, we parse
		   the mime header in case there is something in the mime
		   header we need in order to recover from the error. E.g.,
		   authentication. */
		try {
		    getRequestStatus(status_response, name);
		} finally {
		    mimeHeader = new MessageHeader(serverInput);
		}
	    } else {
		String	line = firstLine;

		if (line == null || line.length() == 0) {
		    throw new SocketException("Unexpected EOF");
		} else if (line.indexOf("hostname unknown") != -1) {
		    throw new UnknownHostException(host);
		} else if (line.indexOf("refused") != -1
			   || line.startsWith("</BODY>")) {
		    /* REMIND: </BODY> seems to be what the proxy on
		       sunweb.ebay spits back for connection refused,
		       but I am just hoping that *some* proxy server
		       indicates "connection refused" with the string
		       "refused" some place in the error message. */
		    throw new SocketException("Connection refused");
		}
	    }
	    } finally {
//		ProgressData.pdata.connected(url);
	    }
	} catch (UnauthorizedHttpRequestException e) {
	    /* don't close server, because we might let it
	       fall through to get whatever message is behind
	       the authentication, assuming the authentication
	       fails */
	    throw e;
	} catch (Exception e) {
	    String cl = getHeaderField("content-length");
	    int len = 0;
	    if (cl == null || (len = Integer.parseInt(cl)) == 0) {
		try {
		    closeServer();
		} catch(Throwable e2) {}
		throw (e instanceof IOException
		       ? (IOException) e : new IOException(e.toString()));
	    }
	}
    }

    /** Close an open connection to the server. */
    public void closeServer() {
	if (url != null) {
//	    ProgressData.pdata.unregister(url);
	    this.url = null;
	}
	try {
	super.closeServer();
	} catch(Throwable e) {}
    }

    public MessageHeader getMimeHeader() {
	return mimeHeader;
    }

    public String getHeaderField(String fieldName) {
	return mimeHeader != null
	    ? mimeHeader.findValue(fieldName)
	    : null;
    }

    public int getStatus() {
	return status;
    }

    public InputStream getInputStream() {
	return serverInput;
    }

    public OutputStream getOutputStream() {
	return serverOutput;
    }

    public void setAuthentication(String au) {
	authentication = au;
    }

    public void setAuthentication(AuthenticationInfo info) {
	if (info == null) {
	    authentication = "";
	    } else {
		authentication = info.auth;
	    }
    }

    
    public String getAuthentication() {
	if (authentication  == null) {
	    return "";
	} else {
	    String  auth = "Authorization: " + authentication + "\r\n";
	    return auth;
	}
    }

    public String getAuthentication(URL url) {
	AuthenticationInfo info = AuthenticationInfo.getAuth(url);
	setAuthentication(info);
	return getAuthentication();
    }



    public String getAuthenticationScheme() {
	return getAuthenticationField(1);
    }

    public String getAuthenticationRealm() {
	return getAuthenticationField(3);
    }

    public String getAuthenticationField(int which) {
	String	field = getHeaderField("www-authenticate");
	if (field == null) {
	    return null;
	}
	StringTokenizer t = new StringTokenizer(field, " =\"");
	while (--which > 0) {
	    t.nextToken();
	}
	if (t.hasMoreTokens()) {
	    return t.nextToken();
	}
	return null;
    }

    public HttpClient(URL url, String auth, String proxy, int  proxyPort) throws UnauthorizedHttpRequestException, IOException {
	this.proxy = proxy;
	this.proxyPort = proxyPort;
	setAuthentication(auth);
	this.url = url;
	int port = url.getPort();
	if (port == -1) {
	    port = httpPortNumber;
	}
	openServer(url.getHost(), port);
    }

    public HttpClient(URL url, String proxy, int  proxyPort)
	throws UnauthorizedHttpRequestException, IOException {
	this.proxy = proxy;
	this.proxyPort = proxyPort;
	this.url = url;
	int port = url.getPort();
	if (port == -1) {
	    port = httpPortNumber;
	}
	openServer(url.getHost(), port);
    }

    public HttpClient(URL url, String auth)
	throws UnauthorizedHttpRequestException, IOException {
	this(url, auth, null, -1);
    }

    public HttpClient(URL url)
	throws UnauthorizedHttpRequestException, IOException {
	this(url, null, -1);
    }
}
