/*
 * @(#)SecurityManager.java	1.25 95/12/08  
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

package java.lang;

import java.io.FileDescriptor;
import java.net.URL;

/**
 * Security Manager. An abstract class that can be subclassed
 * to implement a security policy. It allows the inspection of
 * the classloaders on the execution stack.
 *
 * @author	Arthur van Hoff
 * @version 	1.25, 08 Dec 1995
 */
public abstract
class SecurityManager {
    protected boolean inCheck;

    /**
     * If initialization succeed this is set to true and security checks will
     * succeed. Otherwise the object is not initialized and the object is
     * useless.  Native, private, and protected methods use this to guarantee
     * that flaws in Java will not allow access on such an incompletely constructed
     * instance.
     */
    private boolean initialized = false;

    /** 
     * Returns whether there is a security check in progress.
     * To avoid deadlock, we wait till any other synchronized method
     * in this class is complete (such as checkConnect, which sets this
     * variable.
     */
    synchronized public boolean getInCheck() {
	return inCheck;
    }

    /**
     * Constructs a new SecurityManager.
     * @exception SecurityException If the security manager cannot be
     * created.
     */
    protected SecurityManager() {
	if (System.getSecurityManager() != null) {
	    throw new SecurityException("can't create SecurityManager");
	}
	initialized = true;
    }
    
    /**
     * Gets the context of this Class.  
     */
    protected native Class[] getClassContext();

    /**
     * The current ClassLoader on the execution stack.
     */
    protected native ClassLoader currentClassLoader();

    /**
     * Return the position of the stack frame containing the
     * first occurrence of the named class.
     * @param name classname of the class to search for
     */
    protected native int classDepth(String name);

    /**
     * 
     */
    protected native int classLoaderDepth();

    /**
     * 
     */
    protected native boolean checkClassLoader(int caller_depth);

    /**
     * Returns true if the specified Class is on our stack. 
     * @param name the name of the class
     */
    protected boolean inClass(String name) {
        checkInitialized();
	return classDepth(name) >= 0;
    }

    /**
     * Returns a boolean indicating whether or not the current ClassLoader
     * is equal to null.
     */
    protected boolean inClassLoader() {
        checkInitialized();
	return currentClassLoader() != null;
    }

    /**
     * Returns an implementation-dependent Object which encapsulates
     * enough information about the current execution environment
     * to perform some of the security checks later.
     */
    public Object getSecurityContext() {
	return null;
    }

    /**
     * Checks to see if the ClassLoader has been created.
     * @param caller_depth Distance up stack to unqualified caller
     * @exception SecurityException If a security error has occurred.
     */
    public void checkCreateClassLoader(int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the ClassLoader has been created.
     * Using old (archaic) api.
     * @exception SecurityException If a security error has occurred.
     */
    public void checkCreateClassLoader() {
	checkCreateClassLoader(2);
    }
    
    /**
     * Checks to see if the specified Thread is allowed to modify
     * the Thread group.
     * @param g the Thread to be checked
     * @param caller_depth Distance up stack to unqualified caller
     * @exception SecurityException If the current Thread is not
     * allowed to access this Thread group.
     */
    public void checkAccess(Thread g, int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the specified Thread is allowed to send
     * an exception to the Thread group.
     * @param g the Thread to be checked
     * @param o the Throwable to be checked
     * @param caller_depth Distance up stack to unqualified caller
     * @exception SecurityException If the current Thread is not
     * allowed to access this Thread group.
     */
    public void checkAccess(Thread g, Throwable o, int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the specified Thread is allowed to modify
     * the Thread group. Using old (archaic) interface.
     * @param g the Thread to be checked
     * @exception SecurityException If the current Thread is not
     * allowed to access this Thread group.
     */
    public void checkAccess(Thread g) {
        checkAccess(g, 2);
    }

    /**
     * Checks to see if the specified Thread group is allowed to 
     * modify this group.
     * @param g the Thread group to be checked
     * @param caller_depth Distance up stack to unqualified caller
     * @exception  SecurityException If the current Thread group is 
     * not allowed to access this Thread group.
     */
    public void checkAccess(ThreadGroup g, int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the specified Thread group is allowed to 
     * modify this group. Using old (archaic) api.
     * @param g the Thread group to be checked
     * @exception  SecurityException If the current Thread group is 
     * not allowed to access this Thread group.
     */
    public void checkAccess(ThreadGroup g) {
        checkAccess(g, 4);
    }
    /**
     * Checks to see if the system has exited the virtual 
     * machine with an exit code.
     * @param status exit status, 0 if successful, other values
     * indicate various error types.
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkExit(int status) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the system command is executed by 
     * trusted code.
     * @param cmd the specified system command
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkExec(String cmd) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the specified linked library exists.
     * @param lib the name of the library
     * @param caller_depth Distance up stack to unqualified caller
     * @exception  SecurityException If the library does not exist.
     */
    public void checkLink(String lib, int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the specified linked library exists.
     * Support use of old (archaic) api.
     * @param lib the name of the library
     * @exception  SecurityException If the library does not exist.
     */
    public void checkLink(String lib) {
        checkLink(lib, 2);
        checkLink(lib, 3);
    }

    /**
     * Checks to see if an input file with the specified
     * file descriptor object gets created.
     * @param fd the system dependent file descriptor
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkRead(FileDescriptor fd) {
	throw new SecurityException();
    }

    /**
     * Checks to see if an input file with the specified system dependent
     * file name gets created.
     * @param file the system dependent file name
     * @exception  SecurityException If the file is not found.
     */
    public void checkRead(String file) {
	throw new SecurityException();
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
	throw new SecurityException();
    }

    /**
     * Checks to see if an output file with the specified 
     * file descriptor object gets created.
     * @param fd the system dependent file descriptor
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkWrite(FileDescriptor fd) {
	throw new SecurityException();
    }

    /**
     * Checks to see if an output file with the specified system dependent
     * file name gets created.
     * @param file the system dependent file name
     * @exception  SecurityException If the file is not found.
     */
    public void checkWrite(String file) {
	throw new SecurityException();
    }

    /**
     * Checks to see if a file with the specified system dependent
     * file name can be deleted.
     * @param file the system dependent file name
     * @exception  SecurityException If the file is not found.
     */
    public void checkDelete(String file) {
	throw new SecurityException();
    }

    /**
     * Checks to see if a socket has connected to the specified port on the
     * the specified host.
     * @param host the host name port to connect to
     * @param port the protocol port to connect to
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkConnect(String host, int port) {
	throw new SecurityException();
    }

    /**
     * Checks to see if the current execution context and the indicated
     * execution context are both allowed to connect to the indicated
     * host and port.
     */
    public void checkConnect(String host, int port, Object context) {
	throw new SecurityException();
    }

    /**
     * Checks to see if a URLConnection can call connect.
     */
    public void checkURLConnect(URL url) {
	throw new SecurityException();
    }

    /**
     * Checks to see if a server socket is listening to the specified local
     * port that it is bounded to.
     * @param port the protocol port to connect to
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkListen(int port) {
	throw new SecurityException();
    }

    /**
     * Checks to see if a socket connection to the specified port on the 
     * specified host has been accepted.
     * @param host the host name to connect to
     * @param port the protocol port to connect to
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkAccept(String host, int port) {
	throw new SecurityException();
    }

    /**
     * Checks to see who has access to the System properties.
     * @param caller_depth Distance up stack to unqualified caller
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkPropertiesAccess(int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see who has access to the System properties.
     * Support use of old (archaic) api.
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkPropertiesAccess() {
        checkPropertiesAccess(2);
    }

    /**
     * Checks to see who has access to the System property named by <i>key</i>
     * @param key the System property that the caller wants to examine
     * @param caller_depth Distance up stack to unqualified caller
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkPropertyAccess(String key, int caller_depth) {
	throw new SecurityException();
    }

    /**
     * Checks to see who has access to the System property named by <i>key</i>
     * Support use of old (archaic) interface
     * @param key the System property that the caller wants to examine
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkPropertyAccess(String key) {
        checkPropertyAccess(key, 2);
    }

    /**
     * Checks to see who has access to the System property named by <i>key</i>
     * and <i>def</i>
     * @param key the System property that the caller wants to examine
     * @param def default value to return if this property is not defined
     * @exception  SecurityException If a security error has occurred.
     */
    public void checkPropertyAccess(String key, String def) {
	throw new SecurityException();
    }

    /**
     * Checks to see if top-level windows can be created by the
     * caller. A return of false means that the window creation is
     * allowed but the window should indicate some sort of visual
     * warning. Returning true means the creation is allowed with no
     * special restrictions. To disallow the creation entirely, this
     * method should throw a SecurityException.
     * @param window the new window that's being created.
     */
    public boolean checkTopLevelWindow(Object window) {
	return false;
    }

    /**
     * Check if an applet can access a package.
     */
    public void checkPackageAccess(String pkg) {
	throw new SecurityException();
    }

    /**
     * Check if an applet can define classes in a package.
     */
    public void checkPackageDefinition(String pkg) {
	throw new SecurityException();
    }

    /**
     * Check if an applet can set a networking-related object factory.
     */
    public void checkSetFactory() {
	throw new SecurityException();
    }

    /* Check to be sure we are fully initialized.
     * Throw a security exception if we're not 
     */
    private native boolean checkInitialized();

    /**
     * The security manager for the system
     */
    private static SecurityManager security;

    /**
     * Sets the System security. This value can only be set once.
     * @param s the security manager
     * @exception SecurityException If the SecurityManager has already been set.
     */
    public static void setSecurityManager() {
	if (security != null) {
	    throw new SecurityException("SecurityManager already set");
	}
 	security = System.getSecurityManager();
    }

    /**
     * Check to see if it is allowed to create class loaders.
     */
    public static void checksCreateClassLoader(int caller_depth) {
	if (security != null) {
	    security.checkCreateClassLoader(caller_depth+1);
	}
    }

    /**
     * Check to see if it is allowd to Exit
     * @param status exit status, 0 if successful, other values indicate
     *        various error types. 
     */
    public static void checksExit(int status) {
	if (security != null) {
	    security.checkExit(status);
	}
    }

    /**
     * Check to see if it is allowed to fork processes.
     */
    public static void checksExec(String cmd) {
	if (security != null) {
	    security.checkExec(cmd);
	}
    }

    /**
     * Check to see if it is allowed to link dynamic libraries.
     */
    public static void checksLink(String lib, int caller_depth) {
	if (security != null) {
	    security.checkLink(lib, caller_depth+1);
	}
    }

    /**
     * Check to see if there is access for the system property
     */
    public static void checksPropertiesAccess(int caller_depth) {
	if (security != null) {
	    security.checkPropertiesAccess(caller_depth+1);
	}
    }

    /**
     * Check to see if there is access for the system property
     */
    public static void checksPropertyAccess(String key, int caller_depth) {
	if (security != null) {
	    security.checkPropertyAccess(key, caller_depth+1);
	}
    }

    /**
     * Checks to see if the specified Thread is allowed to modify
     * the Thread group.
     * @param t the Thread to be checked
     * @param caller_depth Distance up stack to unqualified caller
     * @exception SecurityException If the current Thread is not
     * allowed to access this Thread group.
     */
    public static void checksAccess(Thread t, int caller_depth) {
	if (security != null) {
	    security.checkAccess(t, caller_depth+1);
	}
    }

    /**
     * Checks to see if the specified Thread is allowed to send
     * an exception to the Thread group.
     * @param t the Thread to be checked
     * @param o the Throwable to be checked
     * @param caller_depth Distance up stack to unqualified caller
     * @exception SecurityException If the current Thread is not
     * allowed to access this Thread group.
     */
    public static void checksAccess(Thread t, Throwable o, int caller_depth) {
	if (security != null) {
	    security.checkAccess(t, o, caller_depth+1);
	}
    }

    /**
     * Checks to see if the specified ThreadGroup is allowed to modify
     * the Thread group.
     * @param g the ThreadGroup to be checked
     * @param caller_depth Distance up stack to unqualified caller
     * @exception SecurityException If the current ThreadGroup is not
     * allowed to access this Thread group.
     */
    public static void checksAccess(ThreadGroup g, int caller_depth) {
	if (security != null) {
	    security.checkAccess(g, caller_depth+1);
	}
    }

    /**
     * Checks to see if a URLConnection can call connect.
     * @param host the host name port to connect to
     * @param port the protocol port to connect to
     * @exception  SecurityException If a security error has occurred.
     */
    public static void checksURLConnect(URL url) {
	if (security != null) {
	    security.checkURLConnect(url);
	}
    }

    /**
     * To look for annotation (including scanning up stack):
     */
    public native boolean checkScopePermission(int caller_depth);


    /**
     * To set the annotation on a stack frame, becomes super user.
     */
    public native static void setScopePermission();

    /**
     * To set the annotation on a stack frame, to become wimpy applet
     */
    public native static void setAppletScopePermission();

    /**
     * To reset the annotation on a stack frame, permissions depend on 
     * frame above
     */
    public native static void resetScopePermission();
}	
