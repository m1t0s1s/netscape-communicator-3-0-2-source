/*
 * @(#)AppletClassLoader.java	1.18 95/09/18  
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

import java.util.Hashtable;
import java.io.InputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.net.URL;
import java.net.URLConnection;
import java.net.MalformedURLException;
import java.util.Vector;

/**
 * This class defines an applet class loader.
 *
 * @version 	1.18, 18 Sep 1995
 * @author 	Arthur van Hoff
 */
final class AppletClassLoader extends ClassLoader {
    Hashtable classes;
    URL codeBaseURL;
    URL archiveURL;
    MozillaAppletContext context;
    int	nativeZipFile;
    static boolean wantTiming = false;	// change this to get zip timing info
    long loaderTime;
    boolean mayScript;

    // list of all AppletClassLoaders which are active and available for
    // sharing.
    static Vector classloaders = new Vector(10);

    // number of EmbeddedAppletFrames using this AppletClassLoader
    // if 0, there is only one and the classloader will not be shared.
    int refCount;

    /**
     * Using this to allocate an AppletClassLoader instead of the
     * constructor will re-use classloaders for the same codebase
     */
    static synchronized AppletClassLoader getClassLoader(
                       MozillaAppletContext cx,
                       URL codebase, URL archive, boolean mayScript) {
        AppletClassLoader loader = null;
        // search the available classloaders
        for (int i = 0; i < classloaders.size(); i++) {
            AppletClassLoader test
                = (AppletClassLoader)classloaders.elementAt(i);
            /* classloaders are shared iff all of these are true:
             *  - the codebase urls match
             *  - the archive urls match
             *  - the mayscript permissions match
             *  - either there is no mayscript, or the contexts match
             *    (i.e. the applets are on the same page)
             */
            if (codebase.equals(test.codeBaseURL) &&
                archive.equals(test.archiveURL) &&
                mayScript == test.mayScript &&
                (!mayScript || cx == test.context)) {
                loader = test;
                break;
            }
        }
        if (loader == null) {
            loader = new AppletClassLoader(cx, codebase, archive);
            loader.mayScript = mayScript;
            classloaders.addElement(loader);
        }
        loader.refCount++;
        return loader;
    }

    /**
     *  Release the classloader.
     *  This removes it from the global classloader table if the
     *  refcount goes to zero.
     */
    void releaseClassLoader() {
        if (refCount == 0) {
            throw new InternalError("tried to release unshared classloader");
        }

        if (MozillaAppletContext.debug > 1) {
            System.err.println("# released reference to: " + this);
        }
        if (--refCount == 0) {
            classloaders.removeElement(this);
            if (MozillaAppletContext.debug > 1) {
                System.err.println("# removed: " + this);
            }
        }
    }

    /**
     * Load applets from the codebase URL. Remember the document URL
     * so that the AppletSecurity manager can allow access back to
     * the documentURL.
     */
    AppletClassLoader(MozillaAppletContext cx, URL codebase, URL archive) {
        refCount = 0;

        // false is the default - i didn't change the signature of this
        // function so i could avoid disturbing the native code in
        // mo_java.c that allocates one of these.
        mayScript = false;

	// try downloading a zip file
	String file = archive.getFile();
	if (file.endsWith(".zip")) {
	    try {
		if (wantTiming) {
		    long before = System.currentTimeMillis();
		    nativeZipFile = openZipFile(archive.openStream());
		    long after = System.currentTimeMillis();
		    long duration = after - before;
		    loaderTime += duration;
		    System.err.println("# Zip download time: " + archive + ": "
				       + duration + " (total = " + loaderTime + ")");
		}
		else {
		    nativeZipFile = openZipFile(archive.openStream());
		}
		if (context != null && context.debug > 1) {
		    System.err.println("# Loading classes from downloaded zip file: " + archive);
		}
	    } catch (IOException e) {
		System.err.println("# Failed to pull over zip file " + archive);
	    }
	}
	// set up the codebase:
	// Strip the tail end of the URL so that it ends in a /
	file = codebase.getFile();
	int i = file.lastIndexOf('/');
	if ((i > 0) && (i < file.length()-1)) {
	    try {
		codebase = new URL(codebase, file.substring(0, i+1));
	    } catch (MalformedURLException e) {
	    }
	}
	this.codeBaseURL = codebase;
	this.archiveURL = archive;
	this.context = cx;
	this.classes = new Hashtable();
    }
    
    native int openZipFile(InputStream in) throws IOException;
    native byte[] loadFromZipFile(int zipFile, String path) throws IOException;
    native void closeZipFile(int zipFile) throws IOException;

    void close() {
	if (nativeZipFile != 0) {
	    try {
		closeZipFile(nativeZipFile);
		if (context != null && context.debug > 1) {
		    System.err.println("# Closed downloaded zip file: " + codeBaseURL);
		}
	    } catch (IOException e) {
		if (context != null && context.debug > 1) {
		    System.err.println("# Failed to close downloaded zip file: " + codeBaseURL);
		}
	    }
	    nativeZipFile = 0;
	}
    }

    protected void finalize() {
	close();
        if (MozillaAppletContext.debug > 1) {
            System.err.println("# finalized: " + this);
        }
    }

    /**
     * Check if this classloader has JavaScript privileges.
     */
    boolean mayScript() {
        return mayScript;
    }

    /**
     * Load a class from a URL. This does the actual work of loading the
     * class and then defining it.
     */
    private Class loadClass(String name, URL url) throws IOException {
	byte[] data = readURL(url);
	return defineClass(name, data, 0, data.length);
    }

    /**
     * Load a class from this class loader. This method is used by applets
     * that want to explicitly load a class.
     */
    public Class loadClass(String name) throws ClassNotFoundException {
	return loadClass(name, true);
    }

    /**
     * Load and resolve a class. This method is called by the java runtime
     * to get a class that another class needs (e.g. a superclass).
     */
    protected Class loadClass(String name, boolean resolve) throws ClassNotFoundException {
	Class clazz;
	if (wantTiming) {
	    long before = System.currentTimeMillis();
	    clazz = loadClass1(name, resolve);
	    long after = System.currentTimeMillis();
	    long duration = after - before;
	    loaderTime += duration;
	    System.err.println("# Class load time: " + name + ": "
			       + duration + " (total = " + loaderTime + ")");
	}
	else 
	    clazz = loadClass1(name, resolve);
	return clazz;
    }

    Class loadClass1(String name, boolean resolve) throws ClassNotFoundException {
	Class cl = (Class)classes.get(name);
	if (cl == null) {
	    // XXX: We should call a Security.checksPackageAccess() native method, and pass name as arg
	    SecurityManager security = System.getSecurityManager();
	    if (security != null) {
		int i = name.lastIndexOf('.');
		if (i >= 0) {
		    security.checkPackageAccess(name.substring(0, i));
		}
	    }
	    try {
		return findSystemClass(name);
	    } catch (ClassNotFoundException e) {
	    }
	    cl = findClass(name);
	}
	if (cl == null) {
	    throw new ClassNotFoundException(name);
	}

	if (this.nativeZipFile != 0) {
	    SecurityManager.checksURLConnect(this.archiveURL);
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
    private synchronized Class findClass(String name) throws ClassNotFoundException {
	Class cl = (Class)classes.get(name);
	if (cl != null) {
	    return cl;
	}

	if (context != null && context.debug > 1) {
	    System.err.println("# Find class " + name);
	}

	// XXX: We should call a Security.checksPackageDefinition() native method, and pass name as arg
	SecurityManager security = System.getSecurityManager();
	if (security != null) {
	    int i = name.lastIndexOf('.');
	    if (i >= 0) {
		security.checkPackageDefinition(name.substring(0, i));
	    }
	}

	String cname = name.replace('.', '/') + ".class";
	
	if (nativeZipFile != 0) {
	    try {
		byte[] data = loadFromZipFile(nativeZipFile, cname);
                if (data != null) {
                    cl = defineClass(name, data, 0, data.length);
                    if (cl != null && context != null && context.debug > 1) {
                        System.err.println("# Loaded " + cname + " from downloaded zip file.");
                    }
                }
	    } catch (IOException e) {
		if (context.debug > 1)
		    System.err.println("# Failed to load " + cname + " from downloaded zip file.");
	    }
	}
	if (cl == null) {
	    URL url;
	    try {
		url = new URL(codeBaseURL, cname);
	    } catch (MalformedURLException e) {
		throw new ClassNotFoundException(name);
	    }
	    if (context != null && context.debug > 1) {
		System.err.println("# Fetching " + url);
	    }
	    try {
		cl = loadClass(name, url);
	    } catch (IOException e) {
		throw new ClassNotFoundException(name);
	    }
	}
	if (!name.equals(cl.getName())) {
	    Class oldcl = cl;
	    cl = null;
	    throw new ClassFormatError(name + " != " + oldcl.getName());
	}
	classes.put(name, cl);
	return cl;
    }

    byte[] getResource(URL url) {
	byte[] data = null;
	String urlFile = url.getFile();
	String codebaseFile = codeBaseURL.getFile();
	System.out.println("url="+url+" codebase="+codeBaseURL);
	if (!urlFile.startsWith(codebaseFile)) 
	    return null;
	String resourcePath = urlFile.substring(codebaseFile.length());
	System.out.println("resourcePath="+resourcePath);
	try {
	    data = loadFromZipFile(nativeZipFile, resourcePath);
	    if (data != null && context != null && context.debug > 1) {
		System.err.println("# Loaded " + resourcePath + " from downloaded zip file.");
	    }
	} catch (IOException e) {
	    if (context.debug > 1)
		System.err.println("# Failed to load " + resourcePath + " from downloaded zip file.");
	}
	return data;
    }

    byte[] readURL(URL url) throws IOException {
	byte[] data;
	InputStream in = null;
	try {
	    URLConnection c = url.openConnection();
	    c.setAllowUserInteraction(false);
	    in = c.getInputStream();

	    int len = c.getContentLength();
	    data = new byte[(len == -1) ? 4096 : len];
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
	} finally {
	    if (in != null) {
		in.close();
	    }
	}
	return data;
    }
}
