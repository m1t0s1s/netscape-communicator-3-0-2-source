/*
 * @(#)NetworkClient.java	1.18 95/08/22 Jonathan Payne
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

import java.io.*;
import java.net.Socket;
import java.net.UnknownHostException;
import java.net.URL;

/**
 * This is the base class for network clients.
 *
 * @version	1.18, 22 Aug 1995
 * @author	Jonathan Payne
 */
public class NetworkClient {
    /** Socket for communicating with server. */
    protected Socket	serverSocket = null;

    /** Stream for printing to the server. */
    public PrintStream	serverOutput;

    /** Buffered stream for reading replies from server. */
    public InputStream	serverInput;

    /** Open a connection to the server. */
    public void openServer(String server, int port)
	throws IOException, UnknownHostException {
	if (serverSocket != null)
	    closeServer();
	serverSocket = new Socket(server, port);
	serverOutput = new PrintStream(new BufferedOutputStream(serverSocket.getOutputStream()),
				       true);
	serverInput = new BufferedInputStream(serverSocket.getInputStream());
    }

    /** Close an open connection to the server. */
    public void closeServer() throws IOException {
	if (! serverIsOpen()) {
	    return;
	}
	serverSocket.close();
	serverSocket = null;
	serverInput = null;
	serverOutput = null;
    }

    /** Return server connection status */
    public boolean serverIsOpen() {
	return serverSocket != null;
    }

    /** Create connection with host <i>host</i> on port <i>port</i> */
    public NetworkClient(String host, int port) throws IOException {
	openServer(host, port);
    }

    public NetworkClient() {}
}
