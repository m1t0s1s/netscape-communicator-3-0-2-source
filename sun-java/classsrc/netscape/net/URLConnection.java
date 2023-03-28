/*
 * @(#)URLConnection.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.net;

import java.net.URL;
import java.net.UnknownServiceException;
import java.io.IOException;
import java.net.UnknownHostException;
import netscape.net.URLInputStream;
import netscape.net.URLOutputStream;
import netscape.applet.AppletSecurity;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * Class for URL connection that accesses libnet stuff.
 *
 * @version 	1.0, 31 Jul 1995
 * @author 	Warren Harris
 */
public class URLConnection extends java.net.URLConnection {
    static final String EOL = "\r\n";

    int pStreamData;
    URLInputStream currentInputStream = null;
    URLOutputStream currentOutputStream = null;
    String postHeaders;

    /**
     * Constructs a URL connection to the specified URL.
     * No security enforcement here... it must all be done during 
     * connect method (which is called later), even though we try to 
     * re-map the host-name to an IP address.
     * @param url the specified URL
     */
    protected URLConnection(URL url) {
	super(url);
	String hostAddress = null;
	try {
	    hostAddress = url.getHostAddress();
	} catch (UnknownHostException e) {
	} catch (SecurityException e) {
        }
	pCreate(url.toExternalForm(), hostAddress);
    }

    private native void pCreate(String urlString, String addressString);

    protected native void finalize();

    /**
     * URLConnection objects go through two phases: first they are
     * created, then they are connected.  After being created, and before
     * being connected, various option can be specified (eg. doInput,
     * UseCaches, ...).  After connecting, it is an Error to try to set
     * them.  Operations that depend on being connected, like
     * getContentLength, will implicitly perform the connection if
     * necessary.  Connecting when already connected does nothing.
     */
    public void connect() throws IOException {
	if (connected)
	    return;
	SecurityManager security = System.getSecurityManager();
	if (security != null && security instanceof AppletSecurity)
	    ((AppletSecurity)security).checkURLConnect(url);
	
	// Set this before we open the input stream:
	connected = true;

	// Set the connection's postHeaders if there are any. The
	// input stream open method will use it:
	StringBuffer propString = new StringBuffer();
	if (postHeaders != null)
	    propString.append(postHeaders);
	boolean contentTypeSet = false;
	Hashtable props = properties;
	if (props == null) 
	    props = defaultProperties;
	if (props != null) {
	    Enumeration keys = props.keys();
	    while (keys.hasMoreElements()) {
		String key = (String)keys.nextElement();
		// We can't allow the content length to be set explicitly
		// because it will cause netlib to crash.
		if (key.equalsIgnoreCase("Content-length"))
		    continue;
		if (key.equalsIgnoreCase("Content-type"))
		    contentTypeSet = true;
		String value = (String)props.get(key);
		propString.append(key);
		propString.append(":");
		propString.append(value);
		propString.append(EOL);
	    }
	}
	if (!contentTypeSet) {
	    propString.append("Content-type: multipart/form-data");
	    propString.append(EOL);
	}
	postHeaders = propString.toString();
	properties = null;

	// Close the output stream is there was one. This will prepare
	// the postData for the input stream open method.
	if (currentOutputStream != null) {
	    currentOutputStream.close();
	}

	// Finally, get the input stream and open it. This will grab
	// the post data and post headers and send the request, readying
	// the input stream for reading.
	URLInputStream in = (URLInputStream)getInputStream();
	in.open();
    }

    /**
     * Gets the content length. Returns -1 if not known.
     */
    public int getContentLength() {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return -1;
	}
	return getContentLength0();
    }

    public native int getContentLength0();

    /**
     * Gets the content type. Returns null if not known.
     */
    public String getContentType() {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	return getContentType0();
    }

    public native String getContentType0();

    /**
     * Gets a header field by name. Returns null if not known.
     * @param name the name of the header field
     */
    public String getHeaderField(String name) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	return getHeaderField0(name);
    }

    public native String getHeaderField0(String name);

    /**
     * Return the key for the nth header field. Returns null if
     * there are fewer than n fields.  This can be used to iterate
     * through all the headers in the message.
     */
    public String getHeaderFieldKey(int n) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	return getHeaderFieldKey0(n);
    }

    public native String getHeaderFieldKey0(int n);

    /** 
     * Calls this routine to get an InputStream that reads from the object.
     * Protocol implementors should implement this if appropriate. 
     * @exception UnknownServiceException If the protocol does not 
     * support input.
     */
    public java.io.InputStream getInputStream() throws IOException {
	if (!connected)
	    connect();
	if (!doInput)
	    throw new java.net.UnknownServiceException("protocol doesn't support input");
	if (currentInputStream == null) {
	    currentInputStream = new netscape.net.URLInputStream(this);
	}
	return currentInputStream;
    }

    /**
     * Calls this routine to get an OutputStream that writes to the object.
     * Protocol implementors should implement this if appropriate.
     * @exception UnknownServiceException If the protocol does not
     * support output.
     */
    public java.io.OutputStream getOutputStream() throws IOException {
	if (connected)
	    throw new IllegalAccessError("Already connected");
	if (!doOutput)
	    throw new java.net.UnknownServiceException("protocol doesn't support output");
	if (currentOutputStream == null) {
	    currentOutputStream = new netscape.net.URLOutputStream(this);
	    currentOutputStream.open();
	}
	return currentOutputStream;
    }

    /**
     * Set/get a general request property.
     * @param key The keyword by which the request is known (eg "accept")
     * @param value The value associated with it.
     */
    public void setRequestProperty(String key, String value) {
	if (connected)
	    throw new IllegalAccessError("Already connected");
	if (properties == null)
	    properties = new Hashtable();
	if (value != null)
	    properties.put(key, value);
	else
	    properties.remove(key);
    }
    public String getRequestProperty(String key) {
	if (connected)
	    throw new IllegalAccessError("Already connected");
	if (properties == null)
	    return null;
	else
	    return (String)properties.get(key);
    }
    Hashtable properties;

    /**
     * Set/get the default value of a general request property. When a
     * URLConnection is created, it gets initialized with these properties.
     * @param key The keyword by which the request is known (eg "accept")
     * @param value The value associated with it.
     */
    public static void setDefaultRequestProperty(String key, String value) {
	if (defaultProperties == null)
	    defaultProperties = new Hashtable();
	if (value != null)
	    defaultProperties.put(key, value);
	else 
	    defaultProperties.remove(key);
    }
    public static String getDefaultRequestProperty(String key) {
	if (defaultProperties == null)
	    return null;
	else
	    return (String)defaultProperties.get(key);
    }
    static Hashtable defaultProperties;

    /**
     * Closes a URLConnection.
     */
    native void close() throws IOException;
}
