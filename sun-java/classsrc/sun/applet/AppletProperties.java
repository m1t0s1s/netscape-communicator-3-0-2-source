/*
 * @(#)AppletProperties.java	1.6 95/07/31  
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

package sun.applet;

import java.io.File;
import java.io.FileInputStream;
import java.util.Properties;
import java.lang.SecurityManager;

/**
 * This class defines applet properties.
 *
 * @version 	1.6, 31 Jul 1995
 * @author 	Arthur van Hoff
 */
class AppletProperties extends Properties {
    AppletProperties() {
	SecurityManager.setScopePermission();
	this.defaults = System.getProperties();
	
	// Define a number of standard properties
	put("acl.read", "+");
	put("acl.read.default", "/tmp:~/public_html/:~/.mailcap:/etc/mailcap:/usr/etc/mailcap:/usr/local/etc/mailcap");
	put("acl.write", "+");
	put("acl.write.default", "");

	// Standard browser properties
	put("browser", "sun.applet.AppletViewer");
	put("browser.version", "1.02");
	put("browser.vendor", "Sun Microsystems Inc.");

	// These are needed to run inside Sun
	put("firewallSet", "true");
	put("firewallHost", "sunweb.ebay");
	put("firewallPort", "80");

	// Try loading the hotjava properties file to override some
        // of the above defaults.

	try {
	    FileInputStream in = new FileInputStream(System.getProperty("user.home") + 
						     File.separator + ".hotjava" +
						     File.separator + "properties");
	    load(in);
	    in.close();
	} catch (Exception e) {
	    System.out.println("[no properties loaded, using defaults]");
	}
    }
}
