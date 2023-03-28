/*
 * @(#)URLOutputStream.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.net;

import netscape.net.URLConnection;
import java.io.IOException;

public class URLOutputStream extends java.io.OutputStream {
    protected URLConnection connection;

    public URLOutputStream(URLConnection con)
    {
	this.connection = con;
    }

    /**
     * Does a POST URL, preparing the input stream for writing.
     */
    protected native void open() throws IOException;

    /**
     * Writes a byte. This method will block until the byte is actually
     * written.
     * @param b	the byte
     * @exception IOException If an I/O error has occurred.
     */
    public native void write(int b) throws IOException;

    /**
     * Writes an array of bytes. Will block until the bytes
     * are actually written.
     * @param b	the data to be written
     * @exception IOException If an I/O error has occurred.
     */
    public void write(byte b[]) throws IOException {
	writeBytes(b, 0, b.length);
    }

    /**
     * Writes a sub array of bytes. 
     * @param b	the data to be written
     * @param off	the start offset in the data
     * @param len	the number of bytes that are written
     * @exception IOException If an I/O error has occurred.
     */
    public void write(byte b[], int off, int len) throws IOException {
	writeBytes(b, off, len);
    }

    private native void writeBytes(byte b[], int off, int len) throws IOException;

    /**
     * Closes the output stream. Must be called
     * to release any resources associated with
     * the stream.
     * @exception IOException If an I/O error has occurred.
     */
    public void close() throws IOException {
	pClose();

	// Discard this output stream:
	connection.currentOutputStream = null;
    }

    public native void pClose() throws IOException;

    protected void finalize() {
	try {
	    close();
	} catch (IOException e) {
	    // what to do?
	}
    }
}
