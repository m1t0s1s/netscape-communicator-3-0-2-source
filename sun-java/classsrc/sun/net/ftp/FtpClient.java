/*
 * @(#)FtpClient.java	1.32 95/10/18 Jonathan Payne
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

package sun.net.ftp;

import java.util.StringTokenizer;
import java.io.*;
import java.net.*;
import java.lang.SecurityManager;
import sun.net.TransferProtocolClient;
import sun.net.TelnetInputStream;
import sun.net.TelnetOutputStream;

/**
 * This class implements the FTP client. 
 *
 * @version	1.32, 18 Oct 1995
 * @author	Jonathan Payne
 */

public class FtpClient extends TransferProtocolClient {
    public static final int FTP_PORT = 21;

    static int	FTP_SUCCESS = 1;
    static int	FTP_TRY_AGAIN = 2;
    static int	FTP_ERROR = 3;

    /** socket for data transfer */
    private Socket	dataSocket = null;
    private boolean	replyPending = false;
    private boolean	binaryMode = false;

    /** user name for login */
    String		user = null;
    /** password for login */
    String		password = null;

    /** last command issued */
    String		command;

    /** The last reply code from the ftp daemon. */
    int			lastReplyCode;

    /* these fields are used to determine whether ftp urls are sent to */
    /* an http server instead of using a direct connection to the */
    /* host. They aren't used directly here. */
    public static boolean	useFtpProxy = Boolean.getBoolean("ftpProxySet");
    public static String	ftpProxyHost = getPropertyFtpProxyHost();
    public static int		ftpProxyPort = Integer.getInteger("ftpProxyPort", 80).intValue();

    
    private static String getPropertyFtpProxyHost() {
	SecurityManager.setScopePermission();
	return(System.getProperty("ftpProxyHost"));
    }

    /** 
     * issue the QUIT command to the FTP server and close the connection. 
     */
    public void closeServer() throws IOException {
	if (serverIsOpen()) {
	    issueCommand("QUIT");
	    super.closeServer();
	}
    }

    protected int issueCommand(String cmd) throws IOException {
	command = cmd;

	int reply;

	if (replyPending) {
	    if (readReply() == FTP_ERROR)
		System.out.print("Error reading pending reply\n");
	}
	replyPending = false;
	do {
	    sendServer(cmd + "\r\n");
	    reply = readReply();
	} while (reply == FTP_TRY_AGAIN);
	return reply;
    }

    protected void issueCommandCheck(String cmd) throws IOException {
	if (issueCommand(cmd) != FTP_SUCCESS)
	    throw new FtpProtocolException(cmd);
    }

    protected int readReply() throws IOException {
	lastReplyCode = readServerResponse();

	switch (lastReplyCode / 100) {
	case 1:
	    replyPending = true;
	    /* falls into ... */

	case 2:
	case 3:
	    return FTP_SUCCESS;

	case 5:
	    if (lastReplyCode == 530) {
		if (user == null) {
		    throw new FtpLoginException("Not logged in");
		}
		return FTP_ERROR;
	    }
	    if (lastReplyCode == 550) {
		throw new FileNotFoundException(command + ": " + getResponseString());
	    }
	}

	/* this statement is not reached */
	return FTP_ERROR;
    }

    protected Socket openDataConnection(String cmd) throws IOException {
	ServerSocket portSocket;
	String	    portCmd;
	InetAddress myAddress = InetAddress.getLocalHost();
	byte	    addr[] = myAddress.getAddress();
	int	    shift;
	IOException e;

	portSocket = new ServerSocket(0, 1);
	portCmd = "PORT ";

	/* append host addr */
	for (int i = 0; i < addr.length; i++) {
	    portCmd = portCmd + (addr[i] & 0xFF) + ",";
	}

	/* append port number */
	portCmd = portCmd + ((portSocket.getLocalPort() >>> 8) & 0xff) + ","
		+ (portSocket.getLocalPort() & 0xff);
	if (issueCommand(portCmd) == FTP_ERROR) {
	    e = new FtpProtocolException("PORT");
	    portSocket.close();
	    throw e;
	}
	if (issueCommand(cmd) == FTP_ERROR) {
	    e = new FtpProtocolException(cmd);
	    portSocket.close();
	    throw e;
	}
	dataSocket = portSocket.accept();
	portSocket.close();

	return dataSocket;
    }

    /* public methods */

    /** open a FTP connection to host <i>host</i>. */
    public void openServer(String host) throws IOException {
	int port = FTP_PORT;

	/*
	String source = Firewall.verifyAccess(host, port);

	if (source != null) {
	    Firewall.securityError("Applet at " +
				   source +
				   " tried to open FTP connection to "
				   + host + ":" + port);
	    return;
	}
	*/
	openServer(host, port);
    }

    /** open a FTP connection to host <i>host</i> on port <i>port</i>. */
    public void openServer(String host, int port) throws IOException {
	/*
	String source = Firewall.verifyAccess(host, port);

	if (source != null) {
	    Firewall.securityError("Applet at " +
				   source +
				   " tried to open FTP connection to "
				   + host + ":" + port);
	    return;
	}
	*/
	super.openServer(host, port);
	if (readReply() == FTP_ERROR)
	    throw new FtpProtocolException("Welcome message");
    }


    /** 
     * login user to a host with username <i>user</i> and password 
     * <i>password</i> 
     */
    public void login(String user, String password) throws IOException {
	/* This is bogus.  It shouldn't send a password unless it
       	needs to. */

	if (!serverIsOpen())
	    throw new FtpLoginException("not connected to host");
	this.user = user;
	this.password = password;
	if (issueCommand("USER " + user) == FTP_ERROR)
	    throw new FtpLoginException("user");
	if (password != null && issueCommand("PASS " + password) == FTP_ERROR)
	    throw new FtpLoginException("password");
    }

    /** GET a file from the FTP server */
    public TelnetInputStream get(String filename) throws IOException {
	Socket	s;

	try {
//	    throw new FileNotFoundException("Just kidding!");
	    s = openDataConnection("RETR " + filename);
	} catch (FileNotFoundException fileException) {
	    /* Well, "/" might not be the file delimitor for this
	       particular ftp server, so let's try a series of
	       "cd" commands to get to the right place. */
	    StringTokenizer t = new StringTokenizer(filename, "/");
	    String	    pathElement = null;

	    while (t.hasMoreElements()) {
		pathElement = t.nextToken();

		if (!t.hasMoreElements()) {
		    /* This is the file component.  Look it up now. */
		    break;
		}
		try {
		    cd(pathElement);
		} catch (FtpProtocolException e) {
		    /* Giving up. */
		    throw fileException;
		}
	    }
	    if (pathElement != null) {
		s = openDataConnection("RETR " + pathElement);
	    } else {
		throw fileException;
	    }
	}

	return new FtpInputStream(this, s.getInputStream(), binaryMode);
    }

    /** PUT a file to the FTP server */
    public TelnetOutputStream put(String filename) throws IOException {
	Socket s = openDataConnection("STOR " + filename);

	return new TelnetOutputStream(s.getOutputStream(), binaryMode);
    }

    /** LIST files on a remote FTP server */
    public TelnetInputStream list() throws IOException {
	Socket s = openDataConnection("LIST");

	return new TelnetInputStream(s.getInputStream(), binaryMode);
    }

    /** CD to a specific directory on a remote FTP server */
    public void cd(String remoteDirectory) throws IOException {
	issueCommandCheck("CWD " + remoteDirectory);
    }

    /** Set transfer type to 'I' */
    public void binary() throws IOException {
	issueCommandCheck("TYPE I");
	binaryMode = true;
    }

    /** Set transfer type to 'A' */
    public void ascii() throws IOException {
	issueCommandCheck("TYPE A");
	binaryMode = false;
    }

    /** New FTP client connected to host <i>host</i>. */
    public FtpClient(String host) throws IOException {
	super();
	openServer(host, FTP_PORT);
    }

    /** New FTP client connected to host <i>host</i>, port <i>port</i>. */
    public FtpClient(String host, int port) throws IOException {
	super();
	openServer(host, port);
    }

    /** Create an uninitialized FTP client. */
    public FtpClient() {}
}
