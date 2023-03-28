/*
 * @(#)NSAPISocketImplFactory.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.net;

import java.net.*;

/**
 * This interface defines a factory for NSAPISocketImpl instances.
 * It is used by the socket class to create socket implementations
 * that implement various policies.
 */
public 
class NSAPISocketImplFactory implements SocketImplFactory {

    /**
     * Creates a new SocketImpl instance.
     */
    public SocketImpl createSocketImpl() {
	return new NSAPISocketImpl();
    }
}


