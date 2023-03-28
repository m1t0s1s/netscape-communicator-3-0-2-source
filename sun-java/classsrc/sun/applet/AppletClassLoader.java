/*
 * @(#)AppletClassLoader.java	1.21 95/10/09  
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

import java.util.Hashtable;
import java.io.InputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.net.URL;
import java.net.URLConnection;
import java.net.MalformedURLException;

/**
 * This class defines an applet class loader.
 *
 * @version 	1.21, 09 Oct 1995
 * @author 	Arthur van Hoff
 */
class AppletClassLoader extends ClassLoader {
    Hashtable classes = new Hashtable();
    URL base;

    /**
     * Load applets from a base URL.
     */
    AppletClassLoader(URL base) {
	// Strip the tail end of the URL so that it ends in a /
	String file = base.getFile();
	int i = file.lastIndexOf('/');
	if ((i > 0) && (i < file.length()-1)) {
	    try {
		base = new URL(base, file.substring(0, i+1));
	    } catch (MalformedURLException e) {
	    }
	}
	this.base = base;
    }

    /**
     * Load a class from a URL.
     */
    private Class loadClass(URL url) throws IOException {
	InputStream in = null;
	try {
	    try {
		URLConnection c = url.openConnection();
		c.setAllowUserInteraction(false);
		in = c.getInputStream();

		int len = c.getContentLength();
		byte data[] = new byte[(len == -1) ? 4096 : len];
		int total = 0, n;
		
		while ((n = in.read(data, total, data.length - total)) >= 0) {
		    if ((total += n) == data.length) {
			if (len < 0) {
			    byte newdata[] = new byte[total * 2];
			    System.arraycopy(data, 0, newdata, 0, total);
			    data = newdata;
			} else {
			    break;
			}
		    }
		}
		return defineClass(data, 0, total);
	    } finally {
		if (in != null) {
		    in.close();
		}
	    }
	} catch (IOException e) {
	    e.printStackTrace();
	    throw e;
	} catch (Throwable e) {
	    e.printStackTrace();
	    throw new IOException("class not loaded: " + url);
	}
    }

    /**
     * Load a class from this class loader.
     */
    public Class loadClass(String name) throws ClassNotFoundException {
	return loadClass(name, true);
    }

    /**
     * Load and resolve a class.
     */
    protected Class loadClass(String name, boolean resolve) throws ClassNotFoundException {
	Class cl = (Class)classes.get(name);
	if (cl == null) {
	    SecurityManager security = System.getSecurityManager();
	    if (security != null) {
		int i = name.lastIndexOf('.');
		if (i >= 0) {
		    security.checkPackageAccess(name.substring(0, i));
		}
	    }
	    try {
		return findSystemClass(name);
	    } catch (Throwable e) {
	    }

	    cl = findClass(name);
	}
	if (cl == null) {
	    throw new ClassNotFoundException(name);
	}
	if (resolve) {
	    resolveClass(cl);
	}
	return cl;
    }

    /**
     * This method finds a class. The returned class
     * may be unresolved. This method has to be synchronized
     * to avoid two threads loading the same class at the same time.
     * Must be called with the actual class name.
     */
    private synchronized Class findClass(String name) {
	Class cl = (Class)classes.get(name);
	if (cl != null) {
	    return cl;
	}

	System.out.println(Thread.currentThread().getName() + " find class " + name);

	SecurityManager security = System.getSecurityManager();
	if (security != null) {
	    int i = name.lastIndexOf('.');
	    if (i >= 0) {
		security.checkPackageDefinition(name.substring(0, i));
	    }
	}

	String cname = name.replace('.', '/') + ".class";

	try {
	    URL url = new URL(base, cname);
	    System.out.println("Opening stream to: " + url + " to get " + name);
	    cl = loadClass(url);
	    if (!name.equals(cl.getName())) {
		Class oldcl = cl;
		cl = null;
		throw new ClassFormatError(name + " != " + oldcl.getName());
	    }
	    classes.put(name, cl);
	} catch (FileNotFoundException e) {
	    System.err.println("File not found when looking for: " + name);
	} catch (ClassFormatError e) {
	    System.err.println("File format exception when loading: " + name);
	} catch (IOException e) {
	    System.err.println("I/O exception when loading: " + name);
	} catch (Exception e) {
	    System.err.println(e + " exception when loading: " + name);
	} catch (ThreadDeath e) {
	    System.err.println(e + " killed when loading: " + name);
	    throw e;
	} catch (Error e) {
	    System.err.println(e + " error when loading: " + name);
	}
	return cl;
    }
}
