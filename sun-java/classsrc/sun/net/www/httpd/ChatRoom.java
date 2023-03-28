/*
 * @(#)ChatRoom.java	1.2 95/08/22
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights reserved
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file copyright.html
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

package sun.net.www.httpd;

import java.io.*;
import java.util.*;
import sun.net.NetworkServer;

/**
 * A class to implement a room in which people can chat.
 * @author  James Gosling
 */


class ChatRoom {
    static final int histlen = 20;
    String history[] = new String[histlen];
    int histpos = 0;
    String name;
    PrintStream clients[] = new PrintStream[1];
    String clnames[] = new String[1];

    private ChatRoom (String n) {
	name = n;
    }

    static Hashtable ht = new Hashtable(20);
    static ChatRoom New(String n) {
	Object o = ht.get(n);
	if (o != null)
	    return (ChatRoom) o;
	ChatRoom ret = new ChatRoom (n);
	ht.put(n, ret);
	return ret;
    }

    synchronized void addListener(PrintStream p, String clname) {
	int i;
	for (i = clients.length; --i >= 0 && clients[i] != null;);
	addString(clname + " has just entered the room.\n");
	if (i >= 0) {
	    clients[i] = p;
	    clnames[i] = clname;
	} else {
	    PrintStream nc[] = new PrintStream[clients.length * 2];
	    System.arraycopy(clients, 0, nc, 0, clients.length);
	    String nn[] = new String[nc.length];
	    System.arraycopy(clnames, 0, nn, 0, clnames.length);
	    nc[clients.length] = p;
	    nn[clients.length] = clname;
	    clients = nc;
	    clnames = nn;
	}
	p.print("Welcome to " + name + "\n");
	int nseen = 0;
	String lastName = null;
	for (i = clients.length; --i >= 0;)
	    if (clients[i] != null && clients[i] != p) {
		String n = clnames[i];
		if (lastName != null) {
		    if (nseen > 1)
			p.print(", ");
		    p.print(lastName);
		}
		lastName = n;
		nseen++;
	    }
	if (lastName != null) {
	    if (nseen > 1)
		p.print(" and ");
	    p.print(lastName);
	    p.print(nseen > 1 ? " are " : " is ");
	    p.print("already in the room.\n");
	} else
	    p.print("You are alone in the room.\n");
	for (i = history.length; --i >= 0;) {
	    int slot = histpos - i;
	    if (slot < 0)
		slot += history.length;
	    if (history[slot] != null) {
		p.print(history[slot]);
		p.print("\n");
	    }
	}
	p.flush();
    }

    synchronized void removeListener(PrintStream p) {
	int i;
	int nlisteners = 0;
	for (i = clients.length; --i >= 0;)
	    if (clients[i] == p) {
		try {
		    clients[i].close();
		} catch(Exception e);
		clients[i] = null;
		addString(clnames[i] + " has left the room.\n");
	    } else if (clients[i] != null)
		nlisteners++;
	if (nlisteners == 0) {
	    ht.remove(name);
	}
    }

    synchronized void addString(String s) {
	history[histpos] = s;
	histpos++;
	if (histpos > clients.length)
	    histpos = 0;
	for (int i = clients.length; --i >= 0;)
	    try {
	    if (clients[i] != null) {
		try {
		    clients[i].print(s);
		    clients[i].print("\n");
		    clients[i].flush();
		} catch(Exception e) {
		    removeListener(clients[i]);
		}
	    }
	    } catch(Exception e) {
	    }

    }

    void converse(NetworkServer ns) {
	InputStream in = ns.clientInput;
	String clientName = null;
	char line[] = new char[1];
	try {
	    while (true) {
		int c;
		int dp = 0;
		while ((c = in.read()) != -1 && c != '\n' && c != '\r') {
		    if (dp >= line.length) {
			char nc[] = new char[line.length * 2];
			System.arraycopy(line, 0, nc, 0, line.length);
			line = nc;
		    }
		    line[dp++] = (char) c;
		}
		if (c < 0)
		    break;
		if (dp <= 0)
		    continue;
		String s = String.copyValueOf(line, 0, dp);
		if (clientName == null) {
		    clientName = s;
		    addListener(ns.clientOutput, clientName);
		} else {
		    addString(clientName + ": " + s);
		}
	    }
	} catch(Exception e) {
	}
	removeListener(ns.clientOutput);
    }
}
