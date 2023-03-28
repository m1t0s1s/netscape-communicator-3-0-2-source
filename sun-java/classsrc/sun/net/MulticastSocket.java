/*
 * @(#)MulticastSocket.java	1.8 95/12/02 Pavani Diwanji
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

package sun.net;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.SocketException;
import java.net.DatagramPacket;
import java.net.*;
import java.lang.SecurityManager;

/**
 * The multicast datagram socket class 
 * @author Pavani Diwanji
*/
public final
class MulticastSocket extends java.net.DatagramSocket {
    
    static {
	SecurityManager.setScopePermission();
        System.loadLibrary("net");
    }

    public MulticastSocket() throws SocketException {
	super();
    }
    public MulticastSocket(int port) throws SocketException {
	super(port);
    }
    
    /**
     * Joins a multicast group.
     * @param mcastaddr is the multicast address to join 
     * @exception SocketException: when address is not a multicast address
     * TBD: subclass of socket exception i.e. BadAddress
     */
    public void joinGroup(InetAddress mcastaddr) throws SocketException {
	// TBD: Security check plugs in here, if we want.
	// Its not needed as we do a check on every send and receive anyways.
	multicastJoin(mcastaddr);
    }

    /**
     * Leave a multicast group.
     * @param mcastaddr is the multicast address to leave
     * @exception SocketException: when address is not a multicast address
     * TBD: subclass of socket exception i.e. BadAddress
     */
    public void leaveGroup(InetAddress mcastaddr) throws SocketException {
	// TBD: Security check plugs in here, if we want.
	// Its not needed as we do a check on every send and receive anyways.
	multicastLeave(mcastaddr);
    }
    
    /**
     * Sends a datagram packet to the destination
     * @param p	the packet to be sent
     * @param ttl optional time to live for multicast packet.
     * default ttl is 1.
     * @exception IOException i/o error occurred
     * @exception SocketException occurs while setting ttl.
     */

    public synchronized void send(DatagramPacket p, byte ttl)
	 throws IOException, SocketException {

	// check the address is ok wiht the security manager on every send.
	// since here the to_address is going to be multicast address
	// we need a seperate hook in the security manager i.e
	// maxttl = security.checkMulticast()
        // we need to make sure that the ttl that user sets is less
	// than the allowed maxttl.
	SecurityManager security = System.getSecurityManager();
	if (security != null) {
	    security.checkConnect(p.getAddress().getHostAddress(), p.getPort());
	}

	// set the ttl
	setTTL(ttl);
	// call the datagram method to send
	send(p);
    }
    
    private native void setTTL(byte ttl);
    private native void multicastJoin(InetAddress inetaddr);
    private native void multicastLeave(InetAddress inetaddr);

}

