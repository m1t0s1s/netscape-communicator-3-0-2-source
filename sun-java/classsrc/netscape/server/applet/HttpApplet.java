/*
 * @(#)HttpApplet.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.server.applet;

import netscape.net.*;
import java.net.*;
import java.io.*;
import java.util.Hashtable;

public class HttpApplet extends ServerApplet {

    ////////////////////////////////////////////////////////////////////////////
    // Http-Specific Accessors
    ////////////////////////////////////////////////////////////////////////////

    public String getMethod() {
	return getRequestProperty("method");
    }

    URL requestURI;

    public URL getURI() throws MalformedURLException {
	if (requestURI == null)
	    requestURI = new URL(getURL(), getRequestProperty("uri"));
	return requestURI;
    }

    public String getProtocol() {
	return getRequestProperty("protocol");
    }

    public String getQuery() {
	return getRequestProperty("query");
    }

    public String getPath() {
	return getServerProperty("path");
    }

    public void setContentType(String type) {
	setResponseProperty("content-type", type);
    }

    URL serverURL;

    public URL getURL() throws MalformedURLException {
	if (serverURL == null)
	    serverURL = new URL(getServerProperty("url"));
	return serverURL;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Response Methods
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Start the HTTP response to the request. This function returns 
     * NOACTION if the request was a HEAD request and thus no
     * body should be sent. Otherwise, it returns PROCEED.
     * Any headers contained in srvhdrs will be sent, and the
     * status code set with http::status will be sent.
     */
    public native int startResponse() throws IOException;

    /**
     * Start the HTTP response to the request. This function returns 
     * false if the request was a HEAD request and thus no
     * body should be sent. Otherwise, it returns true.
     * Any headers contained in srvhdrs will be sent, and the
     * status code set with http::status will be sent.
     */
    public boolean returnNormalResponse(String contentType, File statFile)
		throws IOException {
	setContentType(contentType);
	if (statFile != null)
	    setFileInfo(statFile);
	status(OK);
	responseStarted = true;
	return startResponse() != NOACTION;
    }

    /**
     * Start the HTTP response to the request. This function returns 
     * false if the request was a HEAD request and thus no
     * body should be sent. Otherwise, it returns true.
     * Any headers contained in srvhdrs will be sent, and the
     * status code set with http::status will be sent.
     */
    public boolean returnNormalResponse(String contentType)
		throws IOException {
	return returnNormalResponse(contentType, null);
    }

    /**
     * Start the HTTP response to the request returning the contents
     * of a file.
     */
    public void returnFile(String contentType, File file) throws IOException {
	if (returnNormalResponse(contentType, file)) {
	    FileInputStream fin = new FileInputStream(file);
	    PrintStream out = getOutputStream();
	    int n;
	    byte buf[] = new byte[1024];
	    while ((n = fin.read(buf)) != -1) {
		out.write(buf, 0, n);
	    }
	}
    }

    /**
     * Start the HTTP response to the request returning the contents
     * of a file.
     */
    public void returnFile(File file) throws IOException {
	returnFile(netscape.net.URLConnection.guessContentTypeFromName(file.getName()),
		   file);
    }

    /**
     * Start the HTTP response to the request. This function returns 
     * false if the request was a HEAD request and thus no
     * body should be sent. Otherwise, it returns true.
     * Any headers contained in srvhdrs will be sent, and the
     * status code set with http::status will be sent.
     */
    public boolean returnErrorResponse(String contentType, int status,
				       String reason)
		throws IOException {
	setContentType(contentType);
	status(status, reason);
	responseStarted = true;
	return startResponse() != NOACTION;
    }

    /**
     * Start the HTTP response to the request. This function returns 
     * false if the request was a HEAD request and thus no
     * body should be sent. Otherwise, it returns true.
     * Any headers contained in srvhdrs will be sent, and the
     * status code set with http::status will be sent.
     */
    public boolean returnErrorResponse(String contentType, int status)
		throws IOException {
	return returnErrorResponse(contentType, status, null);
    }

    /**
     * Status sets the status to the given integer code. The reason string
     * will be used to describe the status.
     */
    public native void status(int n, String reason);

    /**
     * Status sets the status to the given integer code. The reason string
     * will be a default internal string for the given code.
     */
    public void status(int n) {
        status(n, null);
    }

    /**
     * Set HTTP response headers according to the given file. If the client
     * gave a conditional request, that is, only send the object if it has
     * changed since a certain date, this function will return 
     * ABORTED to indicate that the calling function should not
     * proceed with the request.
     */
    public native int setFileInfo(File fi);

    /**
     * Given a URI or virtual path, translate it into a filesystem path
     * using the server's name translation facilities. Returns null
     * if no mapping could be generated.
     */
    public native String translateURI(String uri);

    /**
     * This generates a full URL from a prefix and suffix string. The 
     * prefix and suffix strings are concatenated to form the URL after the
     * http://host:port part. Either prefix or suffix can be null.
     */
    public static String uri2url(String prefix, String suffix) {
        StringBuffer ret = new StringBuffer("http");
	Server server = getServer();
        int port = server.getListeningPort();

        if (server.securityActive())
            ret.append("s");
        ret.append(":" + server.getAddress().getHostName());
        if (port != DEFAULT_PORT)
            ret.append(":" + port);
        if (prefix == null)
            ret.append(prefix);
        if (suffix == null)
            ret.append(suffix);
        return ret.toString();
    }

    /**
     * Takes the form data from this request, and converts it into an array
     * of decoded name=value strings. If the request does not have form
     * data attached, or any of the parameters the client gave are not sane,
     * an IOException is thrown. 
     */
    public Hashtable getFormData() throws IOException {
        String method = getMethod();
        String formdata;
        if (method.equals("GET") || method.equals("HEAD")) {
            formdata = getQuery();
            if (formdata == null)
                throw new IOException("missing query string");
        }
        else if (method.equals("POST")) {
            String t;
            int cl;
            byte[] b;

            /* Get content length, and sanity check it */
            t = getHeader("content-length");
            if (t == null)
                throw new IOException("no content length");
            try {
                cl = Integer.parseInt(t, 10);
            }
            catch (NumberFormatException e) {
                cl = -1;
            }
            if ((cl < 0) || (cl > MAXFORMLENGTH))
                throw new IOException("illegal content length");

            /* Get content type, and sanity check it. */
            t = getHeader("content-type");
            if ((t == null) || (!t.equals(FORMTYPE)))
                throw new IOException("illegal content type");

            b = new byte[cl];
	    DataInputStream din =
		new DataInputStream(getClientSocket().getInputStream());
	    din.readFully(b);
            formdata = new String(b, 0, 0, b.length);
        }
        else
            throw new IOException("unknown method");

        return URIUtil.splitFormData(formdata);
    }

    public static final int OK = 200;
    public static final int NO_RESPONSE = 204;
    public static final int REDIRECT = 302;
    public static final int NOT_MODIFIED = 304;
    public static final int BAD_REQUEST = 400;
    public static final int UNAUTHORIZED = 401;
    public static final int FORBIDDEN = 403;
    public static final int NOT_FOUND = 404;
    public static final int SERVER_ERROR = 500;
    public static final int NOT_IMPLEMENTED = 501;

    private static final int DEFAULT_PORT = 80;
    private static final int MAXFORMLENGTH = 1048576;
    private static final String FORMTYPE = "application/x-www-form-urlencoded";
    
    ////////////////////////////////////////////////////////////////////////////
    // Private Stuff
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Called by the system. 
     */
    protected int handleRequest(int rq) {
	int result;
	try {
	    result = super.handleRequest(rq);
	} finally {
	    if (getResponseProperty("content-type") == null)
		setResponseProperty("content-type", "text/plain");
	}
	return result;
    }
}
