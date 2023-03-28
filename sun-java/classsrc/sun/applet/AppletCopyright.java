/*
 * @(#)AppletCopyright.java	1.4 95/12/01
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

import java.awt.*;
import java.io.*;
import java.util.*;
import java.lang.SecurityManager;

/**
 * Applet copyright splash screen.
 */
class AppletCopyright extends Frame {
    AppletCopyright() {
	setTitle("Copyright Notice");
	Panel p = new Panel();
	TextArea txt = new TextArea(40, 75);
	txt.setEditable(false);
	txt.setText(load());
	txt.setFont(new Font("Courier", Font.BOLD, 12));
	p.add(new Button("Accept"));
	p.add(new Button("Reject"));
	add("Center", txt);
	add("South", p);
	pack();
	Dimension d = size();
	Dimension scrn = getToolkit().getScreenSize();
	move((scrn.width - d.width) / 2, (scrn.height - d.height)/ 2);
	show();
    }
    String load() {
	SecurityManager.setScopePermission();
	File f = new File(System.getProperty("java.home") + 
			  System.getProperty("file.separator") + "COPYRIGHT");
	SecurityManager.resetScopePermission();
	try {
	    if (f.canRead()) {
		ByteArrayOutputStream out = new ByteArrayOutputStream();
		FileInputStream in = new FileInputStream(f);
		byte buf[] =new byte[256];
		int n;
		while ((n = in.read(buf, 0, buf.length)) >= 0) {
		    out.write(buf, 0, n);
		}
		in.close();
		return out.toString();
	    }
	} catch (IOException e) {
	}
	return "Copyright 1995 Sun Microsystems Inc.";
    }

    public synchronized boolean action(Event evt, Object arg) {
	if ("Reject".equals(arg)) {
	    System.exit(1);
	    return true;
	}
	if ("Accept".equals(arg)) {
	    dispose();
	    notify();

	    // Get properties, set version
	    SecurityManager.setScopePermission();
	    Properties props = System.getProperties();
	    SecurityManager.resetScopePermission();
	    props.put("appletviewer.version", "beta2");

	    // Save properties
	    try {
		SecurityManager.setScopePermission();
		FileOutputStream out = new FileOutputStream(System.getProperty("user.home") + 
							    File.separator + ".hotjava" +
							    File.separator + "properties");
		SecurityManager.resetScopePermission();
		props.save(out, "AppletViewer");
		out.close();
	    } catch (IOException e) {
	    }
	    return true;
	}
	return false;
    }

    public synchronized void waitForUser() {
	try {
	    wait();
	} catch (InterruptedException e) {
	}
    }
}
