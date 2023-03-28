/*
 * @(#)ServerPlugin.java	1.3 95/08/22
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

import java.net.URL;

/**
 * A class to establish a framework for code fragments that plug
 * into the Java http server.
 * @author  James Gosling
 */

public class ServerPlugin {
    /** The part of the URL path matched by the *.  When a plugin is
	defined in the server configuration file, like this:<pre>
	plugin /foo/* classname args...</pre>
	and it is subsequently invoked by a URL that looks like this:<pre>
	/foo/whatever</pre>
	<b>whatever</b> will be stored in <i>path</i>. */
    protected String path;
    /** The arguments from the plugin statement in the command line.
	args[0] will contain the class name. */
    protected Object args[];

    public ServerPlugin() {
    }
    final void init(String p, Object a[]) {
	path = p;
	args = a;
    }

    /** Override this method if you need to do some validation of the "args"
	array.  It is called when the configuration file is read.  Return
	null if everything is OK, a String error message otherwise */
    String validate() {
	return null;
    }

    /** Override this method to provide results back to a get.  It should
	print to the server's clientOutput stream.  In class
	sun.net.www.httpd.Server there are a number of helper methods to use to
	construct mime headers.  startHtml(), error() and
	generateProcessOutput() being the handiest. */
    void getRequest(Server s, URL u) {
	s.error("class ServerPlugin should not be directly used.");
    }
}

