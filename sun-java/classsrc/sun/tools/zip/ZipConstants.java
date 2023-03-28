/*
 * @(#)ZipConstants.java	1.4 95/11/12 David Connelly
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

/*
 * This interface defines the constants that are used by the classes
 * which manipulate zip files.
 *
 * @version	1.4, 11/12/95
 * @author	David Connelly
 */
interface ZipConstants {
    /*
     * Header signatures
     */
    static final int SIGSIZ = 4;
    static final byte CENSIG[] = {'P','K','\001','\002'};
    static final byte LOCSIG[] = {'P','K','\003','\004'};
    static final byte ENDSIG[] = {'P','K','\005','\006'};

    /*
     * Header sizes
     */
    static final int LOCHDRSIZ = 30;	// LOC header size
    static final int CENHDRSIZ = 46;	// CEN header size
    static final int ENDHDRSIZ = 22;	// END header size

    /*
     * Local file (LOC) header field offsets
     */
    static final int LOCFLG = 6;	// encryption flags
    static final int LOCHOW = 8;	// compression method
    static final int LOCCRC = 14;	// uncompressed file crc-32 value
    static final int LOCSIZ = 18;	// compressed size
    static final int LOCLEN = 22;	// uncompressed size
    static final int LOCNAM = 26;	// filename length
    static final int LOCEXT = 28;	// extra field length

    /*
     * Central directory (CEN) header field offsets
     */

    static final int CENFLG = 4;	// encrypt, decrypt flags
    static final int CENHOW = 6;	// compression method
    static final int CENTIM = 12;	// modification time
    static final int CENSIZ = 20;	// compressed size
    static final int CENLEN = 24;	// uncompressed size
    static final int CENNAM = 28;	// filename length
    static final int CENEXT = 30;	// extra field length
    static final int CENCOM = 32;	// comment length
    static final int CENOFF = 42;	// LOC header offset

    /*
     * End of central directory (END) header field offsets
     */
    static final int ENDSUB = 8;	// number of entries on this disk
    static final int ENDTOT = 10;	// total number of entries
    static final int ENDSIZ = 12;	// central directory size in bytes
    static final int ENDOFF = 16;	// offset of first CEN header
    static final int ENDCOM = 20;	// zip file comment length
}


