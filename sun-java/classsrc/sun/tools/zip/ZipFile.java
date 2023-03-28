/*
 * @(#)ZipFile.java	1.4 95/11/12 David Connelly
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

import java.io.RandomAccessFile;
import java.io.IOException;
import java.io.File;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * This class represents a zip file and can be used to read its contents.
 * Currently, there is no support for compressed or encrypted zip file
 * entries.
 *
 * @see ZipInputStream
 * @see ZipEntry
 * @version	1.4, 11/12/95
 * @author	David Connelly
 */
public
class ZipFile implements ZipConstants {
    private RandomAccessFile file;  // The open zip file
    private String path;	    // Path name of the zip file
    private Hashtable table;	    // Hash table of zip file entries
    long cenpos;		    // Position of central directory
    private long endpos;	    // Position of end-of-central directory
    private long pos;		    // Current position within zip file

    /**
     * The file separator string used for zip file entry names.
     */
    public static final String separator = "/";

    /**
     * The file separator character used for zip file entry names.
     */
    public static final char separatorChar = separator.charAt(0);

    /**
     * Opens a zip file given the specified path name.
     * @param path the name of the zip file
     * @exception ZipFormatException If the file is not a zip file.
     * @exception IOException If an I/O error has occurred.
     */
    public ZipFile(String path) throws ZipFormatException, IOException {
	file = new RandomAccessFile(path, "r");
	this.path = path;
	init();
    }

    /**
     * Opens a new zip file given the specified File object.
     * @param file the zip file to be opened for reading
     * @exception ZipFormatException If the file is not a zip file.
     * @exception IOException If an I/O error has occurred.
     */
    public ZipFile(File file) throws ZipFormatException, IOException {
	this(file.getPath());
    }

    /**
     * Returns the zip file entry for the given path name or null if
     * no such entry exists.
     * @param path the '/' delimited path name of the entry
     * @return the zip file entry
     */
    public ZipEntry get(String path) {
	return (ZipEntry)table.get(path);
    }

    /**
     * Returns the system-specific path name of the zip file.
     */
    public String getPath() {
        return path;
    }

    /**
     * Returns an enumeration of the zip file entries. Use the Enumeration
     * methods on the returned object to fetch the elements sequentially.
     * @see Enumeration
     */
    public Enumeration entries() {
	return table.elements();
    }

    /*
     * Reads data at specified file position into an array of bytes.
     * This method will block until some input is available.
     */
    synchronized int read(long pos, byte b[], int off, int len)
    	    throws IOException {
        if (pos != this.pos) {
	    file.seek(pos);
	}
	int n = file.read(b, off, len);
	if (n > 0) {
	    this.pos = pos + n;
	}
	return n;
    }

    /*
     * Reads a byte of data at the specified file position. This method
     * will block until some input is available.
     */
    synchronized int read() throws IOException {
	if (pos != this.pos) {
	    file.seek(pos);
	}
	int n = file.read();
	if (n > 0) {
	    this.pos = pos + 1;
	}
	return n;
    }

    /*
     * Read contents of central directory (CEN) and build hash table of
     * zip file entries.
     */
    private void init() throws ZipFormatException, IOException {
	// Seek to beginning of END record
	if (!findEnd()) {
	    throw new ZipFormatException("Could not find END header");
	}
	// Read END header and check signature
	byte endbuf[] = new byte[ENDHDRSIZ];
	file.readFully(endbuf);
	if (!sigMatch(endbuf, ENDSIG)) {
	    throw new ZipFormatException("Invalid END header signature"); 
	}
	// Get position and length of central directory
	cenpos = get32(endbuf, ENDOFF);
	int cenlen = (int)get32(endbuf, ENDSIZ);
	if (cenpos + cenlen != endpos) {
	    throw new ZipFormatException("Invalid END header format");
	}
	// Get total number of entries
	int nent = get16(endbuf, ENDTOT);
	if (nent * CENHDRSIZ > cenlen) {
	    throw new ZipFormatException("Invalid END header format");
	}
	// Check number of drives
	if (get16(endbuf, ENDSUB) != nent) {
	    throw new ZipFormatException("Contains more than one drive");
	}
	// Seek to first CEN record and read central directory
	file.seek(cenpos);
	byte cenbuf[] = new byte[cenlen];
	file.readFully(cenbuf);
	// Scan entries in central directory and build lookup table.
	table = new Hashtable(nent);
	for (int off = 0; off < cenlen; ) {
	    // Check CEN header signature
	    if (!sigMatch(cenbuf, off, CENSIG)) {
		throw new ZipFormatException("Invalid CEN header signature");
	    }
	    ZipEntry entry = new ZipEntry();
	    entry.zip = this;
	    // Get path name of entry
	    int pathlen = get16(cenbuf, off + CENNAM);
	    if (pathlen == 0 || off + CENHDRSIZ + pathlen > cenlen) {
		throw new ZipFormatException("Invalid CEN header format");
	    }
	    // Path names ending with '/' are for directory entries.
	    char c = (char)(cenbuf[off + CENHDRSIZ + pathlen - 1] & 0xff);
	    if (c == separatorChar) {
		entry.directory = true;
	    }
	    entry.path = new String(cenbuf, 0, off + CENHDRSIZ,
				    entry.directory ? pathlen - 1 : pathlen);
	    // Get position of LOC header
	    entry.locpos = get32(cenbuf, off + CENOFF);
	    if (entry.locpos + get32(cenbuf, off + CENSIZ) > cenpos) {
		throw new ZipFormatException("Invalid CEN header format");
	    }
	    // Get uncompressed file size
	    entry.length = get32(cenbuf, off + CENLEN);
	    // Get modification time
	    entry.mtime = get32(cenbuf, off + CENTIM);
	    // Add entry to hash table
	    if (table.put(entry.path, entry) != null) {
		throw new ZipFormatException("Invalid CEN header format");
	    }
	    // Skip to next header
	    off += CENHDRSIZ + pathlen + get16(cenbuf, off + CENEXT) +
		   get16(cenbuf, off + CENCOM);
	}
	// Make sure we got the right number of entries
	if (table.size() != nent) {
	    throw new ZipFormatException("Invalid CEN header format");
	}
    }

    /*
     * Adds new entry to lookup hash table.
     */
    private static final int INBUFSIZ = 64;

    /*
     * Find end-of-central (END) directory record.
     */
    private boolean findEnd() throws IOException {
	// Start searching backwards from end of file
	long len = file.length();
	file.seek(len);
	// Set limit on how far back we need to search. The END header
	// must be located within the last 64K bytes of the file.
	long markpos = Math.max(0, len - 0xffff);
	// Search backwards INBUFSIZ bytes at a time from end of file
	// stopping when the END header signature has been found. Since
	// the signature may straddle a buffer boundary, we need to stash
	// the first SIGSIZ-1 bytes of the previous record at the end of
	// the current record so that the search may overlap.
	byte buf[] = new byte[INBUFSIZ + SIGSIZ];
	setBytes(buf, INBUFSIZ, SIGSIZ, 0);
	for (pos = len; pos > markpos; ) {
	    int n = Math.min((int)(pos - markpos), INBUFSIZ);
	    pos -= n;
	    file.seek(pos);
	    file.readFully(buf, 0, n);
	    while (--n > 0) {
		if (sigMatch(buf, n, ENDSIG)) {
		    // Could be END header, but we need to make sure that
		    // the record extends to the end of the file.
		    endpos = pos + n;
		    if (len - endpos < ENDHDRSIZ) {
			continue;
		    }
		    file.seek(endpos);
		    byte endbuf[] = new byte[ENDHDRSIZ];
		    file.readFully(endbuf);
		    int comlen = get16(endbuf, ENDCOM);
		    if (endpos + ENDHDRSIZ + comlen != len) {
			continue;
		    }
		    // This is definitely the END record, so position
		    // the file pointer at the header and return.
		    file.seek(endpos);
		    pos = endpos;
		    return true;
		}
	    }
	}
	return false;
    }

    /*
     * Check for matching header signature at specified offset within
     * byte array.
     */
    static final boolean sigMatch(byte b[], int off, byte sig[]) {
	for (int i = 0; i < SIGSIZ; i++) {
	    if (b[off+i] != sig[i]) {
		return false;
	    }
	}
	return true;
    }

    /*
     * Check for matching header signature at start of byte array.
     */
    static final boolean sigMatch(byte b[], byte sig[]) {
	return sigMatch(b, 0, sig);
    }

    /*
     * Fill part of byte array with specified byte value.
     */
    static final void setBytes(byte b[], int off, int len, int v) {
	while (len-- > 0) {
	    b[off++] = (byte)v;
	}
    }

    /*
     * Fetch unsigned 16-bit value from byte array at specified offset.
     * The bytes are assumed to be in Intel (little-endian) byte order.
     */
    static final int get16(byte b[], int off) {
	return (b[off] & 0xff) | ((b[off+1] & 0xff) << 8);
    }

    /*
     * Fetch unsigned 32-bit value from byte array at specified offset.
     * The bytes are assumed to be in Intel (little-endian) byte order.
     */
    static final long get32(byte b[], int off) {
	return (b[off] & 0xff) | ((b[off+1] & 0xff) << 8) |
	       ((b[off+2] & 0xff) << 16) | ((b[off+3] & 0xff) << 24);
    }
}
