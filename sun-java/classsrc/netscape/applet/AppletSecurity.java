/*
 * @(#)AppletSecurity.java	1.29 95/12/08  
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
import java.io.FileDescriptor;
import java.net.URL;
import java.net.InetAddress;
import java.util.StringTokenizer;
import java.util.Vector;
import java.lang.SecurityManager;

/**
 * This class defines an applet security policy
 *
 * @version 	1.29, 08 Dec 1995
 * @author	Sami Shaio
 * @author 	Arthur van Hoff
 */
public
class AppletSecurity extends SecurityManager {
    boolean initACL;
    String readACL[];
    String writeACL[];
    int networkMode;

    final static int NETWORK_NONE = 1;
    final static int NETWORK_HOST = 2;
    final static int NETWORK_UNRESTRICTED = 3;

    /**
     * Construct and initialize.
     */
    public AppletSecurity() {
	reset();
    }

    /**
     * Reset from Properties
     */
    void reset() {
	SecurityManager.setScopePermission();
	String str = System.getProperty("appletviewer.security.mode");
	SecurityManager.resetScopePermission();
	if (str == null) {
	    str = "host";
	}

	if (str.equals("unrestricted")) {
	    networkMode = NETWORK_UNRESTRICTED;
	} else if (str.equals("none")) {
	    networkMode = NETWORK_NONE;
	} else {
	    networkMode = NETWORK_HOST;
	}
    }

    /** 
     * Returns whether there is a security check in progress.
     * We are synchonized here, to block other threads from reading 
     * status that pertains to sibling threads (that are 
     * already running in checkConnect())  We can remove this method
     * if the super class is defined to run synchronized.
     */
    public synchronized boolean getInCheck() {
	return super.getInCheck();
    }

    /**
     * True if called directly from an applet.
     * XXX: Is the following used anywhere? It should be removed.
     */
    boolean fromApplet() {
	return checkClassLoader(1);
    }

    /**
     * True if called indirectly from an applet.
     */
    boolean inApplet() {
	return inClassLoader();
    }

    /**
     * The only variable that currently affects whether an applet can
     * perform certain operations is the host it came from.
     */
    public Object getSecurityContext() {
	AppletClassLoader loader = (AppletClassLoader)currentClassLoader();
	if (loader == null) {
	    return null;
	} else {
	    return loader.codeBaseURL;
	}
    }

    /**
     * Applets are not allowed to create class loaders.
     */
    public synchronized void checkCreateClassLoader(int caller_depth) {
	if (checkClassLoader(caller_depth+1)) {
	    throw new AppletSecurityException("classloader");
	}
    }

    /**
     * Applets are not allowed to manipulate threads outside
     * applet thread groups.
     */
    public synchronized void checkAccess(Thread t, int caller_depth) {
	if (!checkScopePermission(caller_depth+1)
                && !(t.getThreadGroup() instanceof AppletThreadGroup)) {
	    throw new AppletSecurityException("thread");
	}
    }

    /**
     * Applets are not allowed to send exceptions other than
     * ThreadDeath exception.
     */
    public synchronized void checkAccess(Thread t, Throwable o, int caller_depth) {
	if (!(o instanceof java.lang.ThreadDeath)
                && !checkScopePermission(caller_depth+1)) {
	    throw new AppletSecurityException("thread can't send exception");
	}
    }

    /**
     * Applets are not allowed to manipulate thread groups outside
     * applet thread groups.
     */
    public synchronized void checkAccess(ThreadGroup g, int caller_depth) {
	if (!checkScopePermission(caller_depth+1)
                && !(g instanceof AppletThreadGroup)) {
	    throw new AppletSecurityException("threadgroup", g.toString());
	}
    }

    /**
     * Applets are not allowed to exit the VM.
     */
    public synchronized void checkExit(int status) {
	if (inApplet()) {
	    throw new AppletSecurityException("exit", String.valueOf(status));
	}
    }

    /**
     * Applets are not allowed to fork processes.
     */
    public synchronized void checkExec(String cmd){
	if (inApplet()) {
	    throw new AppletSecurityException("exec", cmd);
	}
    }

    /**
     * Applets are not allowed to link dynamic libraries.
     */
    public synchronized void checkLink(String lib, int caller_depth){

        if (!checkScopePermission(caller_depth+1))
	    throw new AppletSecurityException("link", lib);
    }

    /**
     * Applets are not allowed to access the entire system properties
     * list, only properties explicitly labeled as accessible to applets.
     */
    public synchronized void checkPropertiesAccess(int caller_depth) {
	if (!checkScopePermission(caller_depth+1)) {
	    throw new AppletSecurityException("properties");
	}
    }

    /**
     * Applets can access the system property named by <i>key</i>
     * only if its twin <i>key.applet</i> property is set to true.
     * For example, the property <code>java.home</code> can be read by
     * applets only if <code>java.home.applet</code> is <code>true</code>.
     */
    public synchronized void checkPropertyAccess(String key, int caller_depth) {
	if (!checkScopePermission(caller_depth+1)) {
	    SecurityManager.setScopePermission();
	    if (!"true".equalsIgnoreCase(System.getProperty(key + ".applet"))) {
	        throw new AppletSecurityException("properties");
            }
	}
    }

    /**
     * Parse an ACL. Deals with "~" and "+"
     */
    void parseACL(Vector v, String path, String defaultPath) {
	SecurityManager.setScopePermission();
	StringTokenizer t = new StringTokenizer(path, System.getProperty("path.separator"));
	while (t.hasMoreTokens()) {
	    String dir = t.nextToken();
	    if (dir.startsWith("~")) {
		v.addElement(System.getProperty("user.home") + dir.substring(1));
	    } else if (dir.equals("+")) {
		if (defaultPath != null) {
		    parseACL(v, defaultPath, null);
		}
	    } else {
		v.addElement(dir);
	    }
	}
    }

    /**
     * Parse an ACL.
     */
    String[] parseACL(String path, String defaultPath) {
	if (path == null) {
	    return new String[0];
	}
	if (path.equals("*")) {
	    return null;
	}
	Vector v = new Vector();
	parseACL(v, path, defaultPath);

	String acl[] = new String[v.size()];
	v.copyInto(acl);
	return acl;
    }

    /**
     * Initialize ACLs. Called only once.
     */
    void initializeACLs() {
	SecurityManager.setScopePermission();
	readACL = parseACL(System.getProperty("acl.read"), 
			   System.getProperty("acl.read.default"));
	writeACL = parseACL(System.getProperty("acl.write"), 
			    System.getProperty("acl.write.default"));
	initACL = true;
    }

    /**
     * Check if an applet can read a particular file.
     */
    public synchronized void checkRead(String file) {
	AppletClassLoader loader = (AppletClassLoader)currentClassLoader();
	if (loader != null) {
	    checkRead(file, loader.codeBaseURL);
	}
    }

    public synchronized void checkRead(String file, URL base) {
	if (base != null) {
	    if (!initACL) {
		initializeACLs();
	    }
	    if (readACL == null) {
		return;
	    }
	    for (int i = readACL.length ; i-- > 0 ;) {
		if (file.startsWith(readACL[i])) {
		    return;
		}
	    }
	    // if the applet is loaded from a file URL, allow reading
	    // in that directory
	    if (base.getProtocol().equals("file")) {
		// URL.getFile() should really be changed to return a native
		// path so that this conversion would not be necessary, but
		// that's too risky right now. -- DAC
		String dir = base.getFile().replace('/', File.separatorChar);
		if (file.startsWith(dir)) {
		    return;
		}
	    }
	    
	    throw new AppletSecurityException("file.read", file);
	}
    }

    /**
     * Checks to see if the current context or the indicated context are
     * both allowed to read the given file name.
     * @param file the system dependent file name
     * @param context the alternate execution context which must also
     * be checked
     * @exception  SecurityException If the file is not found.
     */
    public void checkRead(String file, Object context) {
	checkRead(file);
	if (context != null) {
	    checkRead(file, (URL) context);
	}
    }

    /**
     * Check if an applet can write a particular file.
     */
    public synchronized void checkWrite(String file) {
	if (inApplet()) {
	    if (!initACL) {
		initializeACLs();
	    }
	    if (writeACL == null) {
		return;
	    }
	    for (int i = writeACL.length ; i-- > 0 ;) {
		if (file.startsWith(writeACL[i])) {
		    return;
		}
	    }
	    throw new AppletSecurityException("file.write", file);
	}
    }

    /**
     * Applets are not allowed to open file descriptors unless
     * it is done through a socket, in which case other access
     * restrictions still apply.
     */
    public synchronized void checkRead(FileDescriptor fd) {
	if ((inApplet() && !inClass("java.net.SocketInputStream"))
	    || (!fd.valid()) ) {
	    throw new AppletSecurityException("fd.read");
	}
    }

    /**
     * Applets are not allowed to open file descriptors unless
     * it is done through a socket, in which case other access
     * restrictions still apply.
     */
    public synchronized void checkWrite(FileDescriptor fd) {
	if ( (inApplet() && !inClass("java.net.SocketOutputStream")) 
	     || (!fd.valid()) ) {
	    throw new AppletSecurityException("fd.write");
	}
    }

    /**
     * For now applets can't listen on any port.
     */
    public synchronized void checkListen(int port) {
	AppletClassLoader loader = (AppletClassLoader)currentClassLoader();
	if (loader == null) {
	    // Not called from an applet, so it is ok
	    return;
	}
	if (port > 1024) {
	    // Applets are not allowed to listen on ports that are
	    // considered to be system services
	    return;
	}
	throw new AppletSecurityException("socket.listen", String.valueOf(port));
    }

    /**
     * Check if an applet can accept a connection from the given host:port.
     * This is called after the connection has come in. One of the assumptions
     * that this makes is that the incoming address is valid (otherwise the
     * originating host check won't work). Of course, we are making that
     * assumption on the connect side as well.
     */
    public synchronized void checkAccept(String host, int port) {
	AppletClassLoader loader = (AppletClassLoader)currentClassLoader();
	if (loader == null) {
	    // Not called from an applet, so it is ok
	    return;
	}
	checkConnect(loader.codeBaseURL.getHost(), host);
    }

    /**
     * Check if an applet can connect to the given host:port.
     */
    public synchronized void checkConnect(String host, int port) {
	AppletClassLoader loader = (AppletClassLoader)currentClassLoader();
	if (loader == null) {
	    // Not called from an applet, so it is ok
	    return;
	}
	checkConnect(loader.codeBaseURL.getHost(), host);
    }

    /**
     * Checks to see if the applet and the indicated execution context
     * are both allowed to connect to the indicated host and port.
     */
    public void checkConnect(String host, int port, Object context) {
	checkConnect(host, port);
	if (context != null) {
	    checkConnect(((URL) context).getHost(), host);
	}
    }

    /**
     * Check if an applet from a host can connect to another
     * host. This usually means that you need to determine whether
     * the hosts are inside or outside the firewall. For now applets
     * can only access the host they came from.
     */
    public synchronized void checkConnect(String fromHost, String toHost) {
	//System.out.println("check connect " + fromHost + " to " + toHost);
	if (fromHost == null) {
	    return;
	}

	switch (networkMode) {
	  case NETWORK_NONE:
	    throw new AppletSecurityException("socket.connect", fromHost + "->" + toHost);

	  case NETWORK_HOST:
            if (0 == fromHost.length() && 0 == toHost.length())
                return; // allow non-network connections to local host

	    // set inCheck so InetAddress knows it doesn't have to
	    // check security
	    inCheck = true;

	    // Try comparing InetAddresses
	    try {
		if (InetAddress.getByName(fromHost).
                    equals(InetAddress.getByName(toHost))) {
			return;
		}
                /* System.err.println("connection/info denied " + InetAddress.getByName(fromHost) + 
                        " vs " + InetAddress.getByName(toHost)); */
	    } catch (java.net.UnknownHostException e) {
                // System.err.println("Unknown host of either " + fromHost + " or " + toHost);
	    } finally {
		inCheck = false;
	    }
	    break;

	  case NETWORK_UNRESTRICTED:
	    return;
	}
	throw new AppletSecurityException("socket.connect", fromHost + "->" + toHost);
    }

    /**
     * Check if a URLConnection can call connect.
     */
    public synchronized void checkURLConnect(URL url) {
//	System.err.println("### Attempting to connect to URL "+url.toExternalForm()+" host="+url.getHost()+" port="+url.getPort());
	AppletClassLoader loader = (AppletClassLoader)currentClassLoader();
	if (loader == null) {
	    // Not called from an applet, so it is ok
	    return;
	}
	String codeBaseProtocol = loader.codeBaseURL.getProtocol();
	String protocol = url.getProtocol();
	if (protocol.equals(codeBaseProtocol)) {
	    if (protocol.equals("http") ||
		protocol.equals("https") ||
		protocol.equals("ftp") ||
		protocol.equals("gopher")) {
		// Make sure we can connect to the given host && port
		checkConnect(url.getHost(), url.getPort());
		return;	/* success */
	    }
	    else if (protocol.equals("file")) {
		// Make sure we can connect to the given host && port
		checkConnect(url.getHost(), url.getPort());
		
		// If this is a file: url then we also have to check that it's not trying to 
		// access anything above the directory the applet came from:
		String baseDir = loader.codeBaseURL.getFile();
		String file = url.getFile();
		if (baseDir == null || file == null) return;
		if (file.startsWith(baseDir)) return;	/* success */
	    }
	}
	throw new AppletSecurityException("protocol", protocol);
    }

    /**
     * Checks to see if top-level windows can be created by the caller.
     */
    public synchronized boolean checkTopLevelWindow(Object window) {
	if (inClassLoader()) {
	    /* XXX: this used to return depth > 3. However, this lets */
	    /* some applets create frames without warning strings. */
	    return false;
	}
	return true;
    }

    /**
     * Check if an applet can access a package.
     */
    public synchronized void checkPackageAccess(String pkg) {
	int i = pkg.indexOf('.');
	if (i > 0) {
	    pkg = pkg.substring(0, i);
	}
//	System.err.println("checkPackageAccess: " + "package.restrict.access." + pkg + " --> "
//			   + Boolean.getBoolean("package.restrict.access." + pkg));
	if (inClassLoader() && Boolean.getBoolean("package.restrict.access." + pkg)) {
	    throw new SecurityException();
	}
    }

    /**
     * Check if an applet can define classes in a package.
     */
    public synchronized void checkPackageDefinition(String pkg) {
	int i = pkg.indexOf('.');
	if (i > 0) {
	    pkg = pkg.substring(0, i);
	}
//	System.err.println("checkPackageDefinition: " + "package.restrict.definition." + pkg + " --> "
//			   + Boolean.getBoolean("package.restrict.definition." + pkg));
	if (inClassLoader() && Boolean.getBoolean("package.restrict.definition." + pkg)) {
	    throw new SecurityException();
	}
    }

    /**
     * Check if an applet can set a networking-related object factory.
     * We install our factory before the security manager is installed,
     * so just disallow factories being installed.
     */
    public synchronized void checkSetFactory() {
	throw new SecurityException();
    }

}
