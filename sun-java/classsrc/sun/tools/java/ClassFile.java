/*
 * @(#)ClassFile.java	1.5 95/12/07 David Connelly
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

import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.IOException;
import sun.tools.zip.*;

/**
 * This class is used to represent a file loaded from the class path, and
 * can either be a regular file or a zip file entry.
 */
public
class ClassFile {
    /*
     * Non-null if this represents a regular file
     */
    private File file;

    /*
     * Non-null if this represents a zip file entry
     */
    private ZipEntry entry;

    /**
     * Constructor for instance representing a regular file
     */
    public ClassFile(File file) {
	this.file = file;
    }

    /**
     * Contructor for instance representing a zip file entry
     */
    public ClassFile(ZipEntry entry) {
	this.entry = entry;
    }

    /**
     * Returns true if this is zip file entry
     */
    public boolean isZipped() {
	return entry != null;
    }

    /**
     * Returns input stream to either regular file or zip file entry
     */
    public InputStream getInputStream() throws IOException {
	if (file != null) {
	    return new FileInputStream(file);
	} else {
	    try {
		return new ZipInputStream(entry);
	    } catch (ZipFormatException e) {
		throw new IOException(e.getMessage());
	    }
	}
    }

    /**
     * Returns true if file exists.
     */
    public boolean exists() {
	return file != null ? file.exists() : true;
    }

    /**
     * Returns true if this is a directory.
     */
    public boolean isDirectory() {
	return file != null ? file.isDirectory() : entry.isDirectory();
    }

    /**
     * Return last modification time
     */
    public long lastModified() {
	return file != null ? file.lastModified() : entry.lastModified();
    }

    /**
     * Get file path. The path for a zip file entry will also include
     * the zip file name.
     */
    public String getPath() {
	if (file != null) {
	    return file.getPath();
	} else {
	    ZipFile zipfile = entry.getZipFile();
	    return zipfile.getPath() + "(" + entry.getPath() + ")";
	}
    }

    /**
     * Get name of file entry excluding directory name
     */
    public String getName() {
	return file != null ? file.getName() : entry.getName();
    }

    /**
     * Get length of file
     */
    public long length() {
	return file != null ? file.length() : entry.length();
    }

    public String toString() {
	return (file != null) ? file.toString() : entry.toString();
    }
}
