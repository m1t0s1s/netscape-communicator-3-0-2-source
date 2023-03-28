/*
 * @(#)NetworkServer.java	1.9 95/12/01 James Gosling
 * 
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for NON-COMMERCIAL purposes and without fee is hereby
 * granted provided that this copyright notice appears in all copies. Please
 * refer to the file "copyright.html" for further important copyright and
 * licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */
package sun.net;

import java.io.*;
import java.net.Socket;
import java.net.ServerSocket;
import java.lang.SecurityManager;

/**
 * This is the base class for network servers.  To define a new type
 * of server define a new subclass of NetworkServer with a serviceRequest
 * method that services one request.  Start the server by executing:
 * <pre>
 *	new MyServerClass().startServer(port);
 * </pre>
 */
public class NetworkServer implements Runnable, Cloneable {
    /** Socket for communicating with client. */
    public Socket clientSocket = null;
    private Thread serverInstance;
    private ServerSocket serverSocket;

    /** Stream for printing to the client. */
    public PrintStream clientOutput;

    /** Buffered stream for reading replies from client. */
    public InputStream clientInput;

    /** Close an open connection to the client. */
    public void close() throws IOException {
	clientSocket.close();
	clientSocket = null;
	clientInput = null;
	clientOutput = null;
    }

    /** Return client connection status */
    public boolean clientIsOpen() {
	return clientSocket != null;
    }

    final public void run() {
	if (serverSocket != null) {
	    SecurityManager.setScopePermission();
	    Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
	    SecurityManager.resetScopePermission();
	    // System.out.print("Server starts " + serverSocket + "\n");
	    while (true) {
		try {
		    Socket ns = serverSocket.accept();
//		    System.out.print("New connection " + ns + "\n");
		    NetworkServer n = (NetworkServer)clone();
		    n.serverSocket = null;
		    n.clientSocket = ns;
		    new Thread(n).start();
		} catch(Exception e) {
		    System.out.print("Server failure\n");
		    e.printStackTrace();
		    try {
			serverSocket.close();
		    } catch(IOException e2) {}
		    System.out.print("cs="+serverSocket+"\n");
		    break;
		}
	    }
//	    close();
	} else {
	    try {
		clientOutput = new PrintStream(
			new BufferedOutputStream(clientSocket.getOutputStream()),
					       false);
		clientInput = new BufferedInputStream(clientSocket.getInputStream());
		serviceRequest();
		// System.out.print("Service handler exits
		// "+clientSocket+"\n");
	    } catch(Exception e) {
		// System.out.print("Service handler failure\n");
		// e.printStackTrace();
	    }
	    try {
		close();
	    } catch(IOException e2) {}
	}
    }

    /** Start a server on port <i>port</i>.  It will call serviceRequest()
        for each new connection. */
    final public void startServer(int port) throws IOException {
	serverSocket = new ServerSocket(port, 50);
	serverInstance = new Thread(this);
	serverInstance.start();
    }

    /** Service one request.  It is invoked with the clientInput and
	clientOutput streams initialized.  This method handles one client
	connection. When it is done, it can simply exit. The default
	server just echoes it's input. It is invoked in it's own private
	thread. */
    public void serviceRequest() throws IOException {
	byte buf[] = new byte[300];
	int n;
	clientOutput.print("Echo server " + getClass().getName() + "\n");
	clientOutput.flush();
	while ((n = clientInput.read(buf, 0, buf.length)) >= 0) {
	    clientOutput.write(buf, 0, n);
	}
    }

    public static void main(String argv[]) {
	try {
	    new NetworkServer ().startServer(8888);
	} catch (IOException e) {
	    System.out.print("Server failed: "+e+"\n");
	}
    }

    /**
     * Clone this object;
     */
    public Object clone() {
	try { 
	    return super.clone();
	} catch (CloneNotSupportedException e) {
	    // this shouldn't happen, since we are Cloneable
	    throw new InternalError();
	}
    }

    public NetworkServer () {
    }
}
