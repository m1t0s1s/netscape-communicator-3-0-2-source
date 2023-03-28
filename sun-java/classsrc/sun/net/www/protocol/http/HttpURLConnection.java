/*
 * @(#)HttpURLConnection.java	1.19 95/12/08
 * 
 * Copyright (c) 1995 Sun Microsystems, Inc.  All rights reserved.
 * Permission to
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

package sun.net.www.protocol.http;

import java.net.URL;
import java.io.*;
import java.lang.SecurityManager;

import sun.net.*;
import sun.net.www.*;
import sun.net.www.http.*;

/**
 * A class to represent an HTTP connection to a remote object.
 * @author  James Gosling
 * @author  Herb Jellinek
 */

class HttpURLConnection extends URLConnection {
    
    static final String EOL = "\r\n";
    static final String version = getSystemProperty("java.version");
    public static final String userAgent =
        "User-Agent: "+
        getSystemProperty("http.agent", "Java"+version)+EOL;
    static final String acceptString =
        "Accept: text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2"+EOL;

    static final int maxRedirections = 5;

    HttpClient http;
    Handler handler;

    InputStream inputStream = null;
    boolean outputStreamOpen = false;

    HttpURLConnection (URL u, Handler handler) {
	super(u);
	this.handler = handler;
    }

    public void connect() throws IOException {
	if (connected) {
	    return;
	}
	connected = true;
	ProgressData.pdata.register(url);

	http = null;
	int count = 0;

	try {
	    http = new HttpClient(url, handler.proxy, handler.proxyPort);
	} catch (Throwable e) {
	    if (http != null)
		http.closeServer();
	    ProgressData.pdata.unregister(url);
	    if (e instanceof SecurityException) {
		throw (SecurityException)e;
	    }
	    throw (e instanceof IOException
		   ? (IOException) e
		   : new IOException(e.toString()));
	}
	if (http == null)
	    throw new IOException("Couldn't connect to "+url.toExternalForm());
    }

    // static AuthorizationDialog dialog = null;

    private static AuthenticationInfo getAuthentication(URL url, String scheme, String realm) {
/*-	Authenticator auth;
	ByteArrayOutputStream out = new
	ByteArrayOutputStream(64);
	AuthenticationInfo info;

	// always convert the scheme to lower case so we get a consistent //
	class name.auth = (Authenticator) Class.forName("sun.net.www.auth." +
					scheme.toLowerCase()).newInstance();
	if (dialog == null)
	    dialog = new AuthorizationDialog();
	String userAndPassword = dialog.getAuth(url, realm);

	if (userAndPassword == null)
	    return null;
	auth.encrypt(new StringBufferInputStream(userAndPassword), out);
	info = new AuthenticationInfo(url.host, url.getPort(), realm, scheme + " " +
				      out.toString());
	return info; */
	return null;
    }

    /*
     * Allowable input/output sequences:
     * [interpreted as POST]
     * - get output, [write output,] get input, [read input]
     * - get output, [write output]
     * [interpreted as GET]
     * - get input, [read input]
     * Disallowed:
     * - get input, [read input,] get output, [write output]
     */

    public OutputStream getOutputStream() throws IOException {
	String contentType = getRequestProperty("content-type");

	connect();

	// if there's already an input stream open, throw an exception
	if (inputStream != null) {
	    throw new IOException("Cannot write output after reading input.");
	}
	if (outputStreamOpen) {
	    throw new IOException("Cannot open output twice.");
	}
	OutputStream os = http.getOutputStream();
	outputStreamOpen = true;
	PrintStream ps = new PrintStream(os);

	// send the header
	String cmd = "POST "+http.getURLFile(url)+" HTTP/1.0"+EOL+
	    userAgent+"Referer: "+url+EOL+acceptString;
	
        // if there's an encoding property set, send that down the line.
	if (contentType == null) {
	    contentType = "application/x-www-form-urlencoded";
	}
	cmd += "Content-type: "+contentType+EOL;
	
	ps.print(cmd);

	return new HttpPostBufferStream(os);
    }

    public InputStream getInputStream() throws IOException {
	if (inputStream != null) {
	    return inputStream;
	}

	InputStream is;
	int redirections = 0;
	
	while (true) {
	    connect();
	    
	    if (!outputStreamOpen) {
		// this is a GET url.
		String auth = ""; //getAuthentication(url);
		String cmd = "GET "+http.getURLFile(url)+" HTTP/1.0"+EOL+
		    userAgent+auth+acceptString+EOL;
		PrintStream out = (PrintStream)http.getOutputStream();
		out.print(cmd);
		out.flush();
	    }
	    
	    try {
		http.processRequest(url.getFile());
		is = http.getInputStream();
		
		String newLocation = http.getHeaderField("location");
		if (newLocation != null) {
		    if (outputStreamOpen) {
			http.closeServer();
			connected = false;
			http = null;
			throw new IOException("Cannot redirect with POST: "+
					      url);
		    }
		    if (++redirections > maxRedirections) {
			throw new IOException("Too many redirections ("+
					      redirections+"): "+url);
		    }
		    ProgressData.pdata.unregister(url);
		    // set URL fields in place
		    URL newurl = new URL(url, newLocation);
		    url = newurl;
		    if (!"http".equals(url.getProtocol())) {
			http.closeServer();	// close connection
			connected = false;
			http = null;
			// REMIND this should be possible
			throw new IOException("Cannot redirect to other protocol: "+
					      url);
		    }
		    ProgressData.pdata.register(url);
		}
		// if successful, no rewrite
		if (http.getStatus() == HttpClient.OK ||
		    newLocation == null) {
		    setProperties(http.getMimeHeader());
		    if (getProperties() == null) {
			setProperties(new MessageHeader());
		    }
		    /*
		     * If the http stream returns a content-length attribute
		     * we wrap a MeteredStream around the network stream so
		     * we get some feedback about the progress of the
		     * connection.
		     */
		    String ct = http.getHeaderField("content-length");
		    int len = 0;
		    if (ct != null && (len = Integer.parseInt(ct)) != 0) {
			is = new MeteredStream(is, len, url);
		    } else {
			ProgressData.pdata.unregister(url);
		    }
		    inputStream = is;
		    return is;
		}
		http.closeServer();	// close connection
		ProgressData.pdata.unregister(url);
		http = null;
		inputStream = null;
		connected = false;
	    } catch (IOException e) {
		ProgressData.pdata.unregister(url);
		throw e;
	    }
	}
    }

    /**
     * Gets a header field by name. Returns null if not known.
     * @param name the name of the header field
     */
    public String getHeaderField(String name) {
	if (http == null) {
	    try {
		getInputStream();
	    } catch (Exception e) {
	    }
	}
	return http.getHeaderField(name);
    }

    private static String getSystemProperty(String key, String def) {
	SecurityManager.setScopePermission();
	return(System.getProperty(key, def));
    }
    private static String getSystemProperty(String key) {
	SecurityManager.setScopePermission();
	return(System.getProperty(key));
    }
}


/*-
class AuthorizationDialog extends Frame {
    TextField passwordField;
    TextField userField;
    // Label	mainLabel;
    Window cw;
    Window sw;

    String auth;
    boolean authSet;

    public AuthorizationDialog () {
	super(true, 300, 150, Color.lightGray);
	setDefaultFont(wServer.fonts.getFont("Dialog", Font.BOLD, 12));

	Label l;

	// mainLabel = new Label("", null, cw, null);
	// mainLabel.setHFill(true);

	cw = new Window(this, "Center", background, 300, 100);
	cw.setLayout(new RowColLayout(0, 2, true));

	Column col = new Column(cw, null, false);

	new Space(col, null, 12, 5, false, false);
	l = new Label("User:", null, col, null);
	userField = new TextField("", "", cw, true);

	col = new Column(cw, null, false);
	new Space(col, null, 12, 5, false, false);
	l = new Label("Password:", null, col, null);

	passwordField = new TextField("", "", cw, true);
	passwordField.setEchoCharacter('#');

	sw = new Window(this, "South", background, 100, 30);
	sw.setLayout(new RowLayout(true));
	new FrameButton("Login", sw, this);
	new FrameButton("Clear", sw, this);
	new FrameButton("Cancel", sw, this);
    }

    int preferredHeight = 151;

    public synchronized String getAuth(URL url, String realm) {
	String title = "Login to " + realm + " at " + url.host;
	int width = cw.getFontMetrics(defaultFont).stringWidth(title);

	setTitle(title);
	authSet = false;
	reshape(x, y, width + 80, preferredHeight);
	resize();
	map();
	while (!authSet)
	    wait();
	return auth;
    }

    public void map() {
	userField.setText("");
	passwordField.setText("");
	super.map();
    }

    private synchronized void setAuth(String auth) {
	this.auth = auth;
	authSet = true;
	notifyAll();
    }

    public void handleButtonClick(Button b) {
	String label = b.label;

	if (label.equals("Login")) {
	    setAuth(userField.getText() + ":" + passwordField.getText());
	    unMap();
	} else if (label.equals("Clear")) {
	    userField.setText("");
	    passwordField.setText("");
	} else {
	    setAuth(null);
	    unMap();
	}
    }
}
*/

/*-
class FrameButton extends Button {
    AuthorizationDialog owner;

    public FrameButton(String name, Container cw, AuthorizationDialog d) {
	super(name, "", cw);
	owner = d;
    }

    public void selected(Component c, int pos) {
	owner.handleButtonClick(this);
    }
}
*/
