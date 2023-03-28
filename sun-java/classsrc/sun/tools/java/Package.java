/*
 * @(#)Package.java	1.3 95/11/12 David Connelly
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
import java.io.File;
import java.io.IOException;

/**
 * This class is used to represent the classes in a package.
 */
public
class Package {
    /**
     * The class path for locating package members.
     */
    ClassPath path;

    /**
     * The path name of the package.
     */
    String pkg;

    /**
     * Create a package given a class path and a package name.
     */
    public Package(ClassPath path, Identifier pkg) throws IOException {
	this.path = path;
	this.pkg = pkg.toString().replace('.', File.separatorChar);
    }

    /**
     * Check if a class is defined in this package
     */
    public boolean classExists(Identifier className) {
	return getBinaryFile(className) != null ||
	       getSourceFile(className) != null;
    }

    /**
     * Check if the package exists
     */
    public boolean exists() {
	ClassFile file = path.getFile(pkg);
	return file != null && file.isDirectory();
    }

    /**
     * Get the .class file of a class
     */
    public ClassFile getBinaryFile(Identifier className) {
	String fileName = className.toString() + ".class";
	return path.getFile(pkg + File.separator + fileName);
    }

    /**
     * Get the .java file of a class
     */
    public ClassFile getSourceFile(Identifier className) {
	String fileName = className.toString() + ".java";
	return path.getFile(pkg + File.separator + fileName);
    }

    public ClassFile getSourceFile(String fileName) {
	if (fileName.endsWith(".java")) {
	    return path.getFile(pkg + File.separator + fileName);
	}
	return null;
    }

    public Enumeration getSourceFiles() {
	return path.getFiles(pkg, ".java");
    }

    public Enumeration getBinaryFiles() {
	return path.getFiles(pkg, ".class");
    }
}
