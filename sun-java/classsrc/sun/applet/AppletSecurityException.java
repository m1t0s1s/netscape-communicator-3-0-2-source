/*
 * @(#)AppletSecurityException.java	1.3 95/07/26
 *
 * Copyright (c) 1994-1995 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * An applet security exception.
 *
 * @version 	1.3, 26 Jul 1995
 * @author 	Arthur van Hoff
 */

import java.lang.SecurityManager;

public 
class AppletSecurityException extends SecurityException {
    public AppletSecurityException(String name) {
	super(getProperty(name));
	System.out.println("*** Security Exception: " + name + " ***");
	printStackTrace();
    }
    public AppletSecurityException(String name, String arg) {
	super(getProperty(name) + ": " + arg);
	System.out.println("*** Security Exception: " + name + ":" + arg + " ***");
	printStackTrace();
    }
    private static String getProperty(String name) {
	SecurityManager.setScopePermission();
	return(System.getProperty("security." + name, "security." + name));
    }
}
