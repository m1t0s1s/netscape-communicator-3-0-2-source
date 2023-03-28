/*
 * @(#)URLInputStream.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.net;

import netscape.net.URLConnection;
import java.io.IOException;

public class URLInputStream extends java.io.InputStream {

    protected URLConnection connection;

    public URLInputStream(URLConnection con)
    {
	this.connection = con;
    }

    /**
     * Does a GET URL, preparing the input stream for reading.
     */
    protected native void open() throws IOException;

    /**
     * Reads a byte of data. This method will block if no input is 
     * available.
     * @return 	the byte read, or -1 if the end of the
     *		stream is reached.
     * @exception IOException If an I/O error has occurred.
     */
    public int read() throws IOException
    {
	byte[] b = new byte[1];
	int result = read(b, 0, 1);
	if (result == -1)
	    return result;
	else
	    return b[0] & 0xFF;
    }

    /**
     * Reads into an array of bytes.  This method will
     * block until some input is available.
     * @param b	the buffer into which the data is read
     * @param off the start offset of the data
     * @param len the maximum number of bytes read
     * @return  the actual number of bytes read, -1 is
     * 		returned when the end of the stream is reached.
     * @exception IOException If an I/O error has occurred.
     */
    public native int read(byte b[], int off, int len) throws IOException;

    /**
     * Returns the number of bytes that can be read
     * without blocking.
     * @return the number of available bytes.
     */
    public native int available() throws IOException;

    /**
     * Closes the input stream. Must be called
     * to release any resources associated with
     * the stream.
     * @exception IOException If an I/O error has occurred.
     */
    public void close() throws IOException {
	connection.close();
    }
}
