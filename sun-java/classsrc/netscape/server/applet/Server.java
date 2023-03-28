/*
 * @(#)Server.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.server.applet;

import java.net.*;

class Server {
    /**
     * The port number that this server listens to.
     */
    public native int getListeningPort();

    /**
     * If the server does not listen to every IP address on the system, 
     * this specifies the specific address that it listens to.
     */
    public native InetAddress getListeningAddress();

    /**
     * Set to true if a Commerce is encrypting data to and from the 
     * clients.
     */
    public native boolean securityActive();

    /**
     * The address that the server thinks is its own. The name of this
     * address is a fully qualified domain name, and does not necessarily
     * have to be the machine's hostname, but it's what the administrator
     * wants to appear in URLs.
     */
    public native InetAddress getAddress();
}
