/*
 * @(#)AgentIn.java	1.17 95/11/01
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.tools.debug;

import java.io.*;
import java.net.*;
import java.lang.SecurityManager;

class AgentIn implements Runnable, AgentConstants {
    RemoteAgent agent;
    DebuggerCallback client;
    Socket socket;
    
    AgentIn(RemoteAgent agent, Socket s, DebuggerCallback client,
	    boolean verbose) {
	this.agent = agent;
	this.client = client;
	this.socket = s;
	SecurityManager.setScopePermission();
	Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
	if (verbose) {
	    System.err.println("I/O socket: " + socket.toString());
	}
    }

    public void run() {
        boolean isQuitting = false;
	try {
	    DataInputStream in = new DataInputStream(
		new BufferedInputStream(socket.getInputStream()));
	    while (true) {
                int c = in.read();
                switch (c) {
                  case -1: {
                      if (!isQuitting) {
                          client.printToConsole("\nThe communications channel closed.\n");
                          System.exit(0);
                      }
                  }
                  case CMD_WRITE_BYTES: {
                      int len = in.read();
                      byte buf[] = new byte[len];
                      in.readFully(buf, 0, len);
                      String text = new String(buf, 0);
                      client.printToConsole(text);
                      break;
                  }
                  case CMD_BRKPT_NOTIFY: {
                      RemoteThread t = (RemoteThread)agent.readValue(in);
                      client.breakpointEvent(t);
                      break;
                  }
                  case CMD_EXCEPTION_NOTIFY: {
                      agent.message("AgentIn: exceptionEvent!");
                      RemoteThread t = (RemoteThread)agent.readValue(in);
                      String text = in.readUTF();
                      agent.message("text=" + text);
                      client.exceptionEvent(t, text);
                      break;
                  }
                  case CMD_THREADDEATH_NOTIFY: {
                      RemoteThread t = (RemoteThread)agent.readValue(in);
                      agent.message("AgentIn: threadDeath in " + t.getName());
                      client.threadDeathEvent(t);
                      break;
                  }
                  case CMD_QUIT_NOTIFY:
                      isQuitting = true;
                      client.quitEvent();
                      break;
                  default:
                      // ignore any garbage.
		}
	    }
	} catch (Exception e) {
	    try {
                if (!isQuitting) {
                    client.printToConsole("\nFatal exception: " + e + "\n");
                    e.printStackTrace();
                    System.exit(0);
                }
	    } catch (Exception ignore) {}
	}
    }
}
