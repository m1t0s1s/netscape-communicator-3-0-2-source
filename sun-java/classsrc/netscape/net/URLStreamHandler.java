/*
 * @(#)URLStreamHandler.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.net;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.Hashtable;
import java.net.URL;
import netscape.net.URLConnection;

/**
 * Class for URL stream openers that accesses libnet stuff.
 *
 * @version 	1.0, 31 Jul 1995
 * @author 	Warren Harris
 */
public class URLStreamHandler extends java.net.URLStreamHandler {

    protected java.net.URLConnection
    openConnection(URL u)
    {
	return new URLConnection(u);
    }

    /** 
     * This method is called to parse the string spec into URL u.  If there is any inherited
     * context then it has already been copied into u.  The parameters start and limit refer
     * to the range of characters in spec that should be parsed.  The default
     * method uses parsing rules that match the http spec, which most URL
     * protocol families follow.  If you are writing a protocol handler that
     * has a different syntax, then this routine should be overridden.
     * @param	u the URL to receive the result of parsing the spec
     * @param	spec the URL string to parse
     * @param	start the character position to start parsing at.  This is
     * 		just past the ':' (if there is one)
     * @param	limit the character position to stop parsing at.  This is
     * 		the end of the string or the position of the "#"
     * 		character if present (the "#" reference syntax is
     * 		protocol independant).
     */
    protected void parseURL(URL u, String spec, int start, int limit) {
	super.parseURL(u, spec, start, limit);
	String protocol = u.getProtocol();
	if (protocol.equals("about")
	    || protocol.equals("mailto")
	    || protocol.equals("news")
	    || protocol.equals("snews")
	    || protocol.equals("javascript")
	    || protocol.equals("livescript")
	    || protocol.equals("mocha")) {
	    // for these protocols, we have to remove the bogus '/':
	    String file = u.getFile();
	    file = file.substring(1, file.length());
	    setURL(u, protocol, u.getHost(), u.getPort(), file, u.getRef());
	}
    }
}
