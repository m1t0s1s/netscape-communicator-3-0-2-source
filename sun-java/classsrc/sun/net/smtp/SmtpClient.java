/*
 * @(#)SmtpClient.java	1.9 95/08/29 James Gosling
 * 
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.net.smtp;

import java.util.StringTokenizer;
import java.io.*;
import java.net.*;
import java.lang.SecurityManager;
import sun.net.TransferProtocolClient;

/**
 * This class implements the SMTP client.
 * You can send a piece of mail by creating a new SmtpClient, calling
 * the "to" method to add destinations, calling "from" to name the
 * sender, calling startMessage to return a stream to which you write
 * the message (with RFC733 headers) and then you finally close the Smtp
 * Client.
 *
 * @version	1.17, 12 Dec 1994
 * @author	James Gosling
 */

public class SmtpClient extends TransferProtocolClient {
    SmtpPrintStream message;

    /**
     * issue the QUIT command to the SMTP server and close the connection.
     */
    public void closeServer() throws IOException {
	if (serverIsOpen()) {
	    closeMessage();
	    issueCommand("QUIT\r\n", 221);
	    super.closeServer();
	}
    }

    void issueCommand(String cmd, int expect) throws IOException {
	sendServer(cmd);
	int reply;
	while ((reply = readServerResponse()) != expect)
	    if (reply != 220) {
		throw new SmtpProtocolException(getResponseString());
	    }
    }

    private void toCanonical(String s) throws IOException {
	issueCommand("rcpt to: " + s + "\r\n", 250);
    }

    public void to(String s) throws IOException {
	int st = 0;
	int limit = s.length();
	int pos = 0;
	int lastnonsp = 0;
	int parendepth = 0;
	boolean ignore = false;
	while (pos < limit) {
	    int c = s.charAt(pos);
	    if (parendepth > 0) {
		if (c == '(')
		    parendepth++;
		else if (c == ')')
		    parendepth--;
		if (parendepth == 0)
		    if (lastnonsp > st)
			ignore = true;
		    else
			st = pos + 1;
	    } else if (c == '(')
		parendepth++;
	    else if (c == '<')
		st = lastnonsp = pos + 1;
	    else if (c == '>')
		ignore = true;
	    else if (c == ',') {
		if (lastnonsp > st)
		    toCanonical(s.substring(st, lastnonsp));
		st = pos + 1;
		ignore = false;
	    } else {
		if (c > ' ' && !ignore)
		    lastnonsp = pos + 1;
		else if (st == pos)
		    st++;
	    }
	    pos++;
	}
	if (lastnonsp > st)
	    toCanonical(s.substring(st, lastnonsp));
    }

    public void from(String s) throws IOException {
	issueCommand("mail from: " + s + "\r\n", 250);
    }

    /** open a SMTP connection to host <i>host</i>. */
    private void openServer(String host) throws IOException {
	openServer(host, 25);
	issueCommand("helo "+InetAddress.getLocalHost().getHostName()+"\r\n", 250);
    }

    public PrintStream startMessage() throws IOException {
	issueCommand("data\r\n", 354);
	return message = new SmtpPrintStream(serverOutput, this);
    }

    void closeMessage() throws IOException {
	if (message != null)
	    message.close();
    }

    /** New SMTP client connected to host <i>host</i>. */
    public SmtpClient (String host) throws IOException {
	super();
	if (host != null) {
	    try {
		openServer(host);
		return;
	    } catch(Exception e) {
	    }
	}
	try {
	    SecurityManager.setScopePermission();
	    String s = System.getProperty("mail.host");
	    SecurityManager.resetScopePermission();
	    if (s != null) {
		openServer(s);
		return;
	    }
	} catch(Exception e) {
	}
	try {
	    openServer("localhost");
	} catch(Exception e) {
	    openServer("mailhost");
	}
    }

    /** Create an uninitialized SMTP client. */
    public SmtpClient () throws IOException {
	this(null);
    }
}

class SmtpPrintStream extends java.io.PrintStream {
    private SmtpClient target;
    private int lastc = '\n';

    SmtpPrintStream (OutputStream fos, SmtpClient cl) {
	super(fos);
	target = cl;
    }

    public void close() {
	if (target == null)
	    return;
	if (lastc != '\n') {
	    write('\r');
	    write('\n');
	}
	try {
	    target.issueCommand(".\r\n", 250);
	    target.message = null;
	    out = null;
	    target = null;
	} catch (IOException e) {
	}
    }

    public void write(int b) {
	try {
	    if (lastc == '\n' && b == '.')
		out.write('.');
	    out.write(b);
	    lastc = b;
	} catch (IOException e) {
	}
    }

    public void write(byte b[], int off, int len) {
	try {
	    int lc = lastc;
	    while (--len >= 0) {
		int c = b[off++];
		if (lc == '\n' && c == '.')
		    out.write('.');
		out.write(c);
		lc = c;
	    }
	    lastc = lc;
	} catch (IOException e) {
	}
    }
    public void print(String s) {
	int len = s.length();
	for (int i = 0; i < len; i++) {
	    write(s.charAt(i));
	}
    }
}
