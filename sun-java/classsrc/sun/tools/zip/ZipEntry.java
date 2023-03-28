/*
 * @(#)ZipEntry.java	1.4 95/11/12 David Connelly
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.tools.zip;

/**
 * Zip file entry, which can be used to obtain an input stream for reading
 * the contents of the entry.
 *
 * @see ZipFile
 * @see ZipInputStream
 * @version	1.4, 11/12/95
 * @author	David Connelly
 */
public
class ZipEntry implements ZipConstants {
    ZipFile zip;	// The zip file this entry is a member of
    String path;	// The path name of the entry
    boolean directory;	// True if this entry is a directory
    long locpos;	// The position of the entry's LOC header
    long length;	// The length in bytes of the entry's data
    long mtime;		// The modification time of the entry
    
    /**
     * Returns the '/' delimited path name of the entry.
     */
    public String getPath() {
	return path;
    }

    /**
     * Returns the name of the entry, not including the directory part.
     */
    public String getName() {
	int i = path.lastIndexOf(ZipFile.separatorChar);
	return i < 0 ? path : path.substring(i + 1);
    }

    /**
     * Returns true if this is a directory.
     */
    public boolean isDirectory() {
	return directory;
    }

    /**
     * Returns the length of the entry.
     */
    public long length() {
	return length;
    }

    /**
     * Returns the last modified time of the entry.
     */
    public long lastModified() {
	return mtime;
    }

    /**
     * Returns the ZipFile object for this entry.
     */
    public ZipFile getZipFile() {
	return zip;
    }
}
