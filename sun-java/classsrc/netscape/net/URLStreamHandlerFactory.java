/*
 * @(#)URLStreamHandlerFactory.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.net;

import java.net.URLStreamHandler;

/**
 * Class for URL stream handlers that accesses libnet stuff.
 *
 * @version 	1.0, 31 Jul 1995
 * @author 	Warren Harris
 */
public class URLStreamHandlerFactory implements java.net.URLStreamHandlerFactory {

    private static native void pInit();
    static {
	/* XXX later, when we make this a dll: */
//	Runtime.loadLibrary("nsjava");
	pInit();
    }

    /**
     * Creates a new URLStreamHandler instance with the specified protocol.
     * @param protocol protocol is not used by the Netscape implementation
     * 		but is instead extracted from the url given to the stream handler
     */
    public URLStreamHandler createURLStreamHandler(String protocol) {
	if (pSupportsProtocol(protocol))
  	    return new netscape.net.URLStreamHandler();
        else
  	    return null;	/* let a Java protocol handler handle it */
    }

    private native boolean pSupportsProtocol(String protocol);
}
