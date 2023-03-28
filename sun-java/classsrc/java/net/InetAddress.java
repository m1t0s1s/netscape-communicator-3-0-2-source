/*
 * @(#)InetAddress.java	1.20 95/12/01 Jonathan Payne
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

package java.net;

import java.util.Hashtable;
import java.lang.SecurityManager;

/**
 * A class that represents internet addresses.
 *
 * @version     1.20, 01 Dec 1995
 * @author 	Jonathan Payne
 * @author 	Arthur van Hoff
 * @author 	Chris Warth
 */
public final 
class InetAddress {
    String hostName;
    int address;    // Currently we only deal effectively with 32-bit addresses. 
		    // However this field can be expanded to be a byte array 
		    // or a 64-bit quantity without too much effort.
    int family;

    /*
     * Load net library into runtime.
     */
    static {
	SecurityManager.setScopePermission();
	System.loadLibrary("net");
    }

    /** 
     * Constructor for the Socket.accept() method.
     * This creates an empty InetAddress, which is filled in by
     * the accept() method.  This InetAddress, however, is not
     * put in the address cache, since it is not created by name.
     */
    InetAddress() {}

    /**
     * Creates an InetAddress with the specified host name and IP address.
     * @param hostName the specified host name
     * @param addr the specified IP address.  The address is expected in 
     *	      network byte order.
     * @exception UnknownHostException If the address is unknown.
     */
    InetAddress(String hostName, byte addr[]) {
	this.hostName = hostName;
	this.family = getInetFamily();
	/*
	 * We must be careful here to maintain the network byte
	 * order of the address.  As it comes in, the most
	 * significant byte of the address is in addr[0].  It
	 * actually doesn't matter what order they end up in the
	 * array, as long as it is documented and consistent.
	 */
	address  = addr[3] & 0xFF;
	address |= ((addr[2] << 8) & 0xFF00);
	address |= ((addr[1] << 16) & 0xFF0000);
	address |= ((addr[0] << 24) & 0xFF000000);
    }

    /**
     * Gets the hostname for this address; also the key in the above
     * hash table.
     * If the host is equal to null, then this address refers to any
     * of the local machine's available network addresses.
     */
    public String getHostName() {
	if (hostName == null) {
	    try {
		hostName = getHostByAddr(address);
	    } catch (UnknownHostException e) {
		hostName = getHostAddress();
	    }
	}

	return hostName;
    }

    /**
     * Returns the raw IP address in network byte order.  The
     * highest order byte position is in addr[0].  An array of bytes
     * is returned so we are prepared for 64-bit IP addresses.
     * @return raw IP address in network byte order.
     */
    public byte[] getAddress() {	
	byte[] addr = new byte[4];

	addr[0] = (byte) ((address >>> 24) & 0xFF);
	addr[1] = (byte) ((address >>> 16) & 0xFF);
	addr[2] = (byte) ((address >>> 8) & 0xFF);
	addr[3] = (byte) (address & 0xFF);
	return addr;
    }

    /**
     * Returns the IP address string "%d.%d.%d.%d"
     * @return raw IP address in a string format
     */
    public String getHostAddress() {	
        return ((address >>> 24) & 0xFF) + "." +
               ((address >>> 16) & 0xFF) + "." +
               ((address >>>  8) & 0xFF) + "." +
               ((address >>>  0) & 0xFF);
    }

    /**
     * Returns a hashcode for this InetAddress.
     */
    public int hashCode() {
	return address;
    }

    /**
     * Compares this object against the specified object.
     * @param obj the object to compare with
     * @return true if the objects are the same; false otherwise.
     */
    public boolean equals(Object obj) {
	return (obj != null) && (obj instanceof InetAddress) &&
	    (((InetAddress)obj).address == address);
    }

    /**
     * Converts the InetAddress to a String.
     */
    public String toString() {
	return ((hostName != null) ? hostName + "/" : "") +
               getHostAddress();
    }

    /* Cached addresses - our own litle nis, not! */
    static Hashtable	    addressCache = new Hashtable();
    static InetAddress	    unknownAddress;
    static InetAddress	    anyLocalAddress;
    static InetAddress      localHost;

    static {
	unknownAddress = new InetAddress();
	anyLocalAddress = new InetAddress();
	makeAnyLocalAddress(anyLocalAddress);
	try {
	    localHost = getByName(getLocalHostName());
	} catch (Exception ex) {
	    localHost = unknownAddress;
	}
    }

    /**
     * Returns a network address for the indicated host.  A host name
     * of null refers to default address for the local machine.  A local
     * cache is used to speed access to addresses.  If a all
     * addresses for host are needed, use the getAllByName() method.
     * @param host the specified host
     * @exception UnknownHostException If the address is unknown.
     */
    public static InetAddress getByName(String host)
	throws UnknownHostException
    {
	if (host == null || host.length() == 0) {
	    return getLocalHost();
	}
	return getAllByName(host)[0]; 
    }

    /** 
     * Given a hostname, return an array of all the corresponding InetAddresses.  
     * @exception UnknownHostException If the host name could not be resolved
     */
    public static InetAddress getAllByName(String host)[]
	throws UnknownHostException
    {
	if (host == null) {
	    throw new UnknownHostException(host);
	}

	/* make sure the connection to the host is allowed, before we
	   create the InetAddress */
	SecurityManager security = System.getSecurityManager();
	if (security != null && !security.getInCheck()) {
	    security.checkConnect(host, -1);
	}

	Object obj; // object we recall from our cache
        synchronized(addressCache) {	
            /* Cache.get can return: null, unknownAddress, or InetAddress[] */
	    obj = addressCache.get(host);

	    /* If no entry in cache, then do the host lookup */
	    if (obj == null) {
	        try {
                    /*
                    * Do not put the call to lookup() inside the
                    * constructor.  if you do you will still be
                    * allocating space when the lookup fails.
                    */
		    byte[][] byte_array =  lookupAllHostAddr(host);
                    if (0 == byte_array.length)
                        throw new UnknownHostException(host);
		    InetAddress[] addr_array = new InetAddress[byte_array.length];
		    for (int i = 0; i < byte_array.length; i++) {
		        byte addr[] = byte_array[i];
		        addr_array[i] = new InetAddress(host, addr);
		    }
       		    obj = addr_array;
                    // System.err.println("InetAddress cache added " + ((InetAddress[])obj)[0]);
	        } catch (UnknownHostException e) {
		    obj  = unknownAddress;
	        }
	        addressCache.put(host, obj);
	    } 
	} // Release synchronous lock on addressCache
	    
	if (obj == unknownAddress) {
	    /*
	     * We currently cache the fact that a host is unknown.
	     */
	    throw new UnknownHostException(host);
	}

        /* if (host.charAt(0) >= '0' && host.charAt(0) <= '9')
                System.err.println("Tranlated host name " + host + " to " + 
                        ((InetAddress[])obj).length + " host " +
                        ((InetAddress[])obj)[0]);
        */

        // Copy array, so that hacker can't molest contents of our cache
    	try {
    	      obj = ((InetAddress [])obj).clone();
    	} catch (CloneNotSupportedException cnse) {
    	      cnse.printStackTrace(); // internal error : hack: what to throw??
              obj = null;
    	}
    	return (InetAddress [])obj;
    }

    /**
     * Returns the local host.
     * @exception UnknownHostException If the host name could not be resolved
     */
    public static InetAddress getLocalHost() throws UnknownHostException {
        if (localHost.equals(unknownAddress)) {
	    throw new UnknownHostException();
	}
	return localHost;
    }

    private static native String getLocalHostName() throws UnknownHostException;
    private static native void makeAnyLocalAddress(InetAddress addr);
    private static native byte[] lookupHostAddr(String hostname) throws UnknownHostException;
    private static native byte[][]
        lookupAllHostAddr(String hostname) throws UnknownHostException;
    private static native String getHostByAddr(int addr) throws UnknownHostException;
    private static native int getInetFamily();
}

