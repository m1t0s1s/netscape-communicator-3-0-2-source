/*
 * @(#)URLConnection.java	1.16 95/12/06
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

package sun.net.www;

import java.net.URL;
import java.net.ContentHandler;
import java.util.*;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.BufferedInputStream;
import java.net.UnknownServiceException;

/**
 * A class to represent an active connection to an object
 * represented by a URL.
 * @author  James Gosling
 */

abstract public class URLConnection extends java.net.URLConnection {

    /** The URL that it is connected to */

    private String contentType;
    private int contentLength = -1;

    private MessageHeader properties;


    /** Create a URLConnection object.  These should not be created directly:
	instead they should be created by protocol handers in response to
	URL.openConnection.
	@param	u	The URL that this connects to.
     */
    public URLConnection (URL u) {
	super(u);
    }

    /** Call this routine to get the property list for this object.
     * Properties (like content-type) that have explicit getXX() methods
     * associated with them should be accessed using those methods.  */
    public MessageHeader getProperties() {
	return properties;
    }

    /** Call this routine to set the property list for this object. */
    public void setProperties(MessageHeader properties) {
	this.properties = properties;
    }

    public String getHeaderField(String name) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	MessageHeader props = properties;
	return props == null ? null : props.findValue(name);
    }

    /**
     * Return the key for the nth header field. Returns null if
     * there are fewer than n fields.  This can be used to iterate
     * through all the headers in the message.
     */
    public String GetHeaderFieldKey(int n) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	MessageHeader props = properties;
	return props == null ? null : props.getKey(n);
    }

    /**
     * Return the value for the nth header field. Returns null if
     * there are fewer than n fields.  This can be used in conjunction
     * with GetHeaderFieldKey to iterate through all the headers in the message.
     */
    public String GetHeaderField(int n) {
	try {
	    getInputStream();
	} catch (Exception e) {
	    return null;
	}
	MessageHeader props = properties;
	return props == null ? null : props.getValue(n);
    }

    /** Call this routine to get the content-type associated with this
     * object.
     */
    public String getContentType() {
	if (contentType == null)
	    contentType = getHeaderField("content-type");
	if (contentType == null) {
	    String ct = null;
	    try {
		ct = guessContentTypeFromStream(getInputStream());
	    } catch(java.io.IOException e) {
	    }
	    String ce = properties.findValue("content-encoding");
	    if (ct == null) {
		ct = properties.findValue("content-type");

		if (ct == null)
		    if (url.getFile().endsWith("/"))
			ct = "text/html";
		    else
			ct = guessContentTypeFromName(url.getFile());
	    }

	    /*
	     * If the Mime header had a Content-encoding field and its value
	     * was not one of the values that essentially indicate no
	     * encoding, we force the content type to be unknown. This will
	     * cause a save dialog to be presented to the user.  It is not
	     * ideal but is better than what we were previously doing, namely
	     * bringing up an image tool for compressed tar files.
	     */

	    if (ct == null || ce != null &&
		    !(ce.equalsIgnoreCase("7bit")
		      || ce.equalsIgnoreCase("8bit")
		      || ce.equalsIgnoreCase("binary")))
		ct = "content/unknown";
	    contentType = ct;
	}
	return contentType;
    }

    /**
     * Set the content type of this URL to a specific value.
     * @param	type	The content type to use.  One of the
     *			content_* static variables in this
     *			class should be used.
     *			eg. setType(URL.content_html);
     */
    public void setContentType(String type) {
	contentType = type;
    }

    /** Call this routine to get the content-length associated with this
     * object.
     */
    public int getContentLength() {
	int l = contentLength;
	if (l < 0) {
	    try {
		l = Integer.parseInt(properties.findValue("content-length"));
		contentLength = l;
	    } catch(Exception e) {
	    }
	}
	return l;
    }

    /** Call this routine to set the content-length associated with this
     * object.
     */
    protected void setContentLength(int length) {
	contentLength = length;
    }

    /**
     * Returns true if the data associated with this URL can be cached.
     */
    public boolean canCache() {
	return url.getFile().indexOf('?') < 0	/* && url.postData == null
	        REMIND */ ;
    }

    /**
     * Call this to close the connection and flush any remaining data.
     * Overriders must remember to call super.close()
     */
    public void close() {
	url = null;
    }
}
