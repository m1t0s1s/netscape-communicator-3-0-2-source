/*
 * @(#)NSAPISocketOutputStream.java	1.9 95/11/06 Warren Harris
 *
 * Copyright (c) 1996 Netscape Communications Corporation. All Rights Reserved.
 */

package netscape.net;

import java.io.FileOutputStream;
import java.io.IOException;
import java.net.*;

/**
 * This stream extends FileOutputStream to implement a
 * NSAPISocketOutputStream. Note that this class should <b>NOT</b> be
 * public.
 */
class NSAPISocketOutputStream extends FileOutputStream
{
    private NSAPISocketImpl impl;
    private byte temp[] = new byte[1];
    
    /**
     * Creates a new NSAPISocketOutputStream. Can only be called
     * by a Socket. This method needs to hang on to the owner Socket so
     * that the fd will not be closed.
     * @param impl the socket output stream inplemented
     */
    NSAPISocketOutputStream(NSAPISocketImpl impl) throws IOException {
	super(impl.getFileDescriptor());
	this.impl = impl;
    }

    /**
     * Writes to the socket.
     * @param b the data to be written
     * @param off the start offset in the data
     * @param len the number of bytes that are written
     * @exception IOException If an I/O error has occurred.
     */
    private native void socketWrite(byte b[], int off, int len)
	throws IOException;

    /** 
     * Writes a byte to the socket. 
     * @param b the data to be written
     * @exception IOException If an I/O error has occurred. 
     */
    public void write(int b) throws IOException {
	temp[0] = (byte)b;
	socketWrite(temp, 0, 1);
    }

    /** 
     * Writes the contents of the buffer <i>b</i> to the socket.
     * @param b the data to be written
     * @exception SocketException If an I/O error has occurred. 
     */
    public void write(byte b[]) throws IOException {
	socketWrite(b, 0, b.length);
    }

    /** 
     * Writes <i>length</i> bytes from buffer <i>b</i> starting at 
     * offset <i>len</i>.
     * @param b the data to be written
     * @param off the start offset in the data
     * @param len the number of bytes that are written
     * @exception SocketException If an I/O error has occurred.
     */
    public void write(byte b[], int off, int len) throws IOException {
	socketWrite(b, off, len);
    }

    /**
     * Closes the stream.
     */
    public void close() throws IOException {
	impl.close();
    }

    /** 
     * Overrides finalize, the fd is closed by the Socket.
     */
    protected void finalize() {}
}
