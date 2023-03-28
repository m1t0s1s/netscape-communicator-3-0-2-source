/*
 * @(#)IftpClient.java	1.13 95/08/29 Jonathan Payne
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

import java.io.*;
import java.net.*;

/**
 * Create an FTP client that uses a proxy server to cross a network
 * firewall boundary.
 *
 * @version 	1.13, 29 Aug 1995
 * @author	Jonathan Payne
 * @see FtpClient
 */
public class IftpClient extends FtpClient {
    public static final int IFTP_PORT = 4666;

    /** The proxyserver to use */
    String  proxyServer = "sun-barr";
    String  actualHost = null;
    
    /**
     * Open an FTP connection to host <i>host</i>.
     */
    public void openServer(String host) throws IOException {
	if (!serverIsOpen())
	    super.openServer(proxyServer, IFTP_PORT);
	actualHost = host;
    }

    boolean checkExpectedReply() throws IOException {
	return readReply() != FTP_ERROR;
    }

    /** 
     * login user to a host with username <i>user</i> and password 
     * <i>password</i> 
     */
    public void login(String user, String password) throws IOException {
	if (!serverIsOpen()) {
	    throw new FtpLoginException("not connected to host");
	}
	user = user + "@" + actualHost;
	this.user = user;
	this.password = password;
	if (issueCommand("USER " + user) == FTP_ERROR ||
	    lastReplyCode == 220 && !checkExpectedReply()) {
	    throw new FtpLoginException("user");
	}
	if (password != null && issueCommand("PASS " + password) == FTP_ERROR) {
	    throw new FtpLoginException("password");
	}
    }

    /**
     * change the proxyserver from the default.
     */
    public void setProxyServer(String proxy) throws IOException {
	if (serverIsOpen())
	    closeServer();
	proxyServer = proxy;
    }

    /**
     * Create a new IftpClient handle.
     */
    public IftpClient(String host) throws IOException {
	super();
	openServer(host);
    }

    /** Create an uninitialized client handle */
    public IftpClient() {}
}
