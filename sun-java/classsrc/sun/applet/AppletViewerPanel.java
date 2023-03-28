/*
 * @(#)AppletViewerPanel.java	1.1 95/11/13
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

import java.util.*;
import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import java.awt.*;
import java.applet.*;
import java.awt.image.MemoryImageSource;
import java.awt.image.ColorModel;


/**
 * Sample applet panel class. The panel manages and manipulates the
 * applet as it is being loaded. It forks a seperate thread in a new
 * thread group to call the applet's init(), start(), stop(), and
 * destroy() methods.
 *
 * @version 	1.1, 13 Nov 1995
 * @author 	Arthur van Hoff
 */
class AppletViewerPanel extends AppletPanel {
    /**
     * The document url.
     */
    URL documentURL;

    /**
     * The base url.
     */
    URL baseURL;

    /**
     * The attributes of the applet.
     */
    Hashtable atts;

    /**
     * Construct an applet viewer and start the applet.
     */
    AppletViewerPanel(URL documentURL, Hashtable atts) {
	this.documentURL = documentURL;
	this.atts = atts;

	// Get the base (if any)
	String att = getParameter("codebase");
	if (att != null) {
	    if (!att.endsWith("/")) {
		att += "/";
	    }
	    try {
		baseURL = new URL(documentURL, att);
	    } catch (MalformedURLException e) {
		baseURL = documentURL;
	    }
	} else {
	    baseURL = documentURL;
	}
    }

    /**
     * Get an applet parameter.
     */
    public String getParameter(String name) {
	return (String)atts.get(name.toLowerCase());
    }

    /**
     * Get the document url.
     */
    public URL getDocumentBase() {
	return documentURL;
    }

    /**
     * Get the base url.
     */
    public URL getCodeBase() {
	return baseURL;
    }

    /**
     * Get the applet context. For now this is
     * also implemented by the AppletPanel class.
     */
    public AppletContext getAppletContext() {
	return (AppletContext)getParent();
    }
}
