/*
 * @(#)ClassPath.java	1.21 95/12/04 David Connelly
 *
 * Copyright (c) 1994, 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.tools.java;

import java.util.Enumeration;
import java.util.Hashtable;
import java.io.File;
import java.io.IOException;
import sun.tools.zip.*;

/**
 * This class is used to represent a class path, which can contain both
 * directories and zip files.
 */
public
class ClassPath {
    static final char dirSeparator = File.pathSeparatorChar;

    /**
     * The original class path string
     */
    String pathstr;

    /**
     * List of class path entries
     */
    private ClassPathEntry[] path;

    /**
     * Build a class path from the specified path string
     */
    public ClassPath(String pathstr) {
	int i, j, n;
	// Save original class path string
	this.pathstr = pathstr;
	// Count the number of path separators
	i = n = 0;
	while ((i = pathstr.indexOf(dirSeparator, i)) != -1) {
	    n++; i++;
	}
	// Build the class path
	ClassPathEntry[] path = new ClassPathEntry[n+1];
	int len = pathstr.length();
	for (i = n = 0; i < len; i = j + 1) {
	    if ((j = pathstr.indexOf(dirSeparator, i)) == -1) {
		j = len;
	    }
	    if (i == j) {
		path[n] = new ClassPathEntry();
		path[n++].dir = new File(".");
	    } else {
		File file = new File(pathstr.substring(i, j));
		if (file.isFile()) {
		    try {
			ZipFile zip = new ZipFile(file);
			path[n] = new ClassPathEntry();
			path[n++].zip = zip;
		    } catch (ZipFormatException e) {
		    } catch (IOException e) {
			// Ignore exceptions, at least for now...
		    }
		} else {
		    path[n] = new ClassPathEntry();
		    path[n++].dir = file;
		}
	    }
	}
	// Trim class path to exact size
	this.path = new ClassPathEntry[n];
	System.arraycopy((Object)path, 0, (Object)this.path, 0, n);
    }

    /**
     * Load the specified file from the class path
     */
    public ClassFile getFile(String name) {
	for (int i = 0; i < path.length; i++) {
	    if (path[i].zip != null) {
		String newname = name.replace(File.separatorChar,
					      ZipFile.separatorChar);
		ZipEntry entry = path[i].zip.get(newname);
		if (entry != null) {
		    return new ClassFile(entry);
		}
	    } else {
		File file = new File(path[i].dir.getPath(), name);
		if (file.exists()) {
		    return new ClassFile(file);
		}
	    }
	}
	return null;
    }

    /**
     * Returns list of files given a package name and extension.
     */
    public Enumeration getFiles(String pkg, String ext) {
	Hashtable files = new Hashtable();
	for (int i = path.length; --i >= 0; ) {
	    if (path[i].zip != null) {
		Enumeration e = path[i].zip.entries();
		while (e.hasMoreElements()) {
		    ZipEntry entry = (ZipEntry)e.nextElement();
		    String name = entry.getPath();
		    name = name.replace(ZipFile.separatorChar,
					File.separatorChar);
		    if (name.startsWith(pkg) && name.endsWith(ext)) {
			files.put(name, new ClassFile(entry));
		    }
		}
	    } else {
		File dir = new File(path[i].dir.getPath(), pkg);
		if (dir.isDirectory()) {
		    String[] list = dir.list();
		    for (int j = 0; j < list.length; j++) {
			String name = list[j];
			if (name.endsWith(ext)) {
			    File file = new File(dir.getPath(), name);
			    files.put(name, new ClassFile(file));
			}
		    }
		}
	    }
	}
	return files.elements();
    }

    /**
     * Returns original class path string
     */
    public String toString() {
	return pathstr;
    }
}

/**
 * A class path entry, which can either be a directory or an open zip file.
 */
class ClassPathEntry {
    File dir;
    ZipFile zip;
}
