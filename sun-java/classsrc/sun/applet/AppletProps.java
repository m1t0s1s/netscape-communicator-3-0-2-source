/*
 * @(#)AppletProps.java	1.6 95/12/01
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
import java.util.Properties;
import java.lang.SecurityManager;

import sun.net.www.http.HttpClient;
import sun.net.ftp.FtpClient;

class AppletProps extends Frame {
    
    TextField proxyHost;
    TextField proxyPort;
    TextField firewallHost;
    TextField firewallPort;
    Choice networkMode;
    Choice accessMode;

    AppletProps() {
	setTitle("AppletViewer Properties");
	Panel p = new Panel();
	p.setLayout(new GridLayout(0, 2));

	p.add(new Label("Http proxy server:"));
	p.add(proxyHost = new TextField());

	p.add(new Label("Http proxy port:"));
	p.add(proxyPort = new TextField());

	p.add(new Label("Firewall proxy server:"));
	p.add(firewallHost = new TextField());

	p.add(new Label("Firewall proxy port:"));
	p.add(firewallPort = new TextField());

	p.add(new Label("Network access:"));
	p.add(networkMode = new Choice());
	networkMode.addItem("None");
	networkMode.addItem("Applet Host");
	networkMode.addItem("Unrestricted");

	p.add(new Label("Class access:"));
	p.add(accessMode = new Choice());
	accessMode.addItem("Restricted");
	accessMode.addItem("Unrestricted");

	add("Center", p);
	p = new Panel();
	p.add(new Button("Apply"));
	p.add(new Button("Reset"));
	p.add(new Button("Cancel"));
	add("South", p);
	move(200, 150);
	pack();
	reset();
    }

    void reset() {
	((AppletSecurity)System.getSecurityManager()).reset();

	switch (((AppletSecurity)System.getSecurityManager()).networkMode) {
	  case AppletSecurity.NETWORK_UNRESTRICTED:
	    networkMode.select("Unrestricted");
	    break;
	  case AppletSecurity.NETWORK_NONE:
	    networkMode.select("None");
	    break;
	  default:
	    networkMode.select("Applet Host");
	    break;
	}

        // XXX: cleanup setScopePermission
	SecurityManager.setScopePermission();
	if (Boolean.getBoolean("package.restrict.access.sun")) {
	    accessMode.select("Restricted");
	} else {
	    accessMode.select("Unrestricted");
	}

	if (Boolean.getBoolean("proxySet")) {
	    proxyHost.setText(System.getProperty("proxyHost"));
	    proxyPort.setText(System.getProperty("proxyPort"));
	    HttpClient.useProxyForCaching = true;
	    HttpClient.cachingProxyHost = System.getProperty("proxyHost");;
	    HttpClient.cachingProxyPort = 
		Integer.valueOf(System.getProperty("proxyPort")).intValue();
	} else {
	    proxyHost.setText("");
	    proxyPort.setText("");
	    HttpClient.useProxyForCaching = false;
	}
	if (Boolean.getBoolean("firewallSet")) {
	    firewallHost.setText(System.getProperty("firewallHost"));
	    firewallPort.setText(System.getProperty("firewallPort"));
	    HttpClient.useProxyForFirewall = true;
	    HttpClient.firewallProxyHost = System.getProperty("firewallHost");;
	    HttpClient.firewallProxyPort = 
		Integer.valueOf(System.getProperty("firewallPort")).intValue();
	} else {
	    firewallHost.setText("");
	    firewallPort.setText("");
	    HttpClient.useProxyForFirewall = false;
	}
    }

    void apply() {
	// Get properties, set version
	SecurityManager.setScopePermission();
	Properties props = System.getProperties();
	SecurityManager.resetScopePermission();
	props.put("appletviewer.version", "beta2");
	if (proxyHost.getText().length() > 0) {
	    props.put("proxySet", "true");
	    props.put("proxyHost", proxyHost.getText().trim());
	    props.put("proxyPort", proxyPort.getText().trim());
	} else {
	    props.put("proxySet", "false");
	}
	if (firewallHost.getText().length() > 0) {
	    props.put("firewallSet", "true");
	    props.put("firewallHost", firewallHost.getText().trim());
	    props.put("firewallPort", firewallPort.getText().trim());
	} else {
	    props.put("firewallSet", "false");
	}
	if ("None".equals(networkMode.getSelectedItem())) {
	    props.put("appletviewer.security.mode", "none");
	} else if ("Unrestricted".equals(networkMode.getSelectedItem())) {
	    props.put("appletviewer.security.mode", "unrestricted");
	} else {
	    props.put("appletviewer.security.mode", "host");
	}

	if ("Restricted".equals(accessMode.getSelectedItem())) {
	    props.put("package.restrict.access.sun", "true");
	    props.put("package.restrict.access.netscape", "true");
	} else {
	    props.put("package.restrict.access.sun", "false");
	    props.put("package.restrict.access.netscape", "false");
	}

	// Save properties
	try {
	    reset();

	    SecurityManager.setScopePermission();
	    FileOutputStream out = new FileOutputStream(System.getProperty("user.home") + 
							File.separator + ".hotjava" +
							File.separator + "properties");
	    SecurityManager.resetScopePermission();
	    props.save(out, "AppletViewer");
	    out.close();
	    hide();
	} catch (IOException e) {
	    System.out.println("Failed to save properties: " + e);
	    e.printStackTrace();
	    reset();
	}
    }

    public boolean action(Event evt, Object obj) {
	if ("Apply".equals(obj)) {
	    apply();
	    return true;
	}
	if ("Reset".equals(obj)) {
	    reset();
	    return true;
	}
	if ("Cancel".equals(obj)) {
	    hide();
	    return true;
	}
	return false;
    }
}

