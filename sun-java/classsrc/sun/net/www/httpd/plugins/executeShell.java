/*
 * @(#)executeShell.java	1.3 95/08/22
 * 
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights reserved Permission to
 * use, copy, modify, and distribute this software and its documentation for
 * NON-COMMERCIAL purposes and without fee is hereby granted provided that
 * this copyright notice appears in all copies. Please refer to the file
 * copyright.html for further important copyright and licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

package sun.net.www.httpd.plugins;

import sun.net.www.httpd.ServerPlugin;
import sun.net.www.httpd.Server;
import java.net.URL;

/**
 * A class to define a server plugin that will execute a shell command
 * and return its output as an html document.
 * @author  James Gosling
 */

class executeShell extends ServerPlugin {
    String validate() {
	if (args.length < 1)
	    return "No shell command specified";
	return null;
    }
    void getRequest(Server s, URL u) {
	String command = args[0].toString();
	for (int i = 1; i < args.length; i++)
	    command = command + " " + args[i].toString();
	System.out.print("Trying to exec <" + command + ">\n");
	s.generateProcessOutput(command, command);
    }
}
