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

package netscape.applet;

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
        // XXX: cleanup this code, by using private static method.
	SecurityManager.setScopePermission();
	this.defaults = System.getProperties();
	SecurityManager.resetScopePermission();

	// Standard browser properties
	put("browser", "Netscape Navigator");
	put("browser.version", "3.0");
	put("browser.vendor", "Netscape Communications Corporation");

	// Define which packages cannot be extended by applets
	// XXX fix checkPackage stuff in AppletSecurity.java
	put("package.restrict.definition.java", "true");
	put("package.restrict.definition.sun", "true");
        // Allow Netcode/IFC to put stuff in netscape pacakages
	// put("package.restrict.definition.netscape", "true");

	// Access controls for system properties
	put("browser.applet", "true");
	put("browser.version.applet", "true");
	put("browser.vendor.applet", "true");
	put("java.version.applet", "true");
	put("java.vendor.applet", "true");
	put("java.vendor.url.applet", "true");
	put("java.class.version.applet", "true");
	put("os.name.applet", "true");
	put("os.arch.applet", "true");
	put("os.version.applet", "true");
	put("file.separator.applet", "true");
	put("path.separator.applet", "true");
	put("line.separator.applet", "true");


	// Access controls for awt,etc system properties. We could either 
        // enpower the methods where these powers are accessed or 
        // we list them here, so that they can be accessed publicly.
	put("awt.image.incrementaldraw.applet", "true");
	put("awt.image.redrawrate.applet", "true");
	put("awt.toolkit.applet", "true");
	put("awt.appletWarning.applet", "true");
	put("awt.imagefetchers.applet", "true");

	put("console.bufferlength.applet", "true");
    }
}
