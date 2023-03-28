package netscape.server.base;

import netscape.server.base.Buffer;
import java.io.IOException;
import java.io.FileNotFoundException;

/**
 * filebuf is a type of buffer representing a file. On most systems, the
 * buffering is accomplished through memory-mapped I/O. 
 *
 * @author	Rob McCool
 */


public class filebuf extends Buffer {
    private int Cfilebuf;
    private final int sizelimit = 128 * 1024;
    private final int defaultSize = 4096;

    public native int getbytes(byte[] b, int offset, int maxlength)
        throws IOException;

    public native int sendToSocket(netFD sd) throws IOException;

    private native void create(String name, int size) 
        throws FileNotFoundException, IOException;

    /**
      Create a new filebuf using the given filename and the given buffer 
      size. Size may not exceed 128*1024, and is ignored if the underlying
      system supports memory mapped file I/O. Throws FileNotFoundException 
      if the file doesn't exist or can't be read.
     */
    public filebuf(String name, int size) 
        throws FileNotFoundException, IOException 
    { 
        create(name, size);
    }

    /**
      Create a new filebuf using the given filename and a default buffer 
      size. Throws FileNotFoundException if the file doesn't exist or
      can't be read.
     */
    public filebuf(String name) 
        throws FileNotFoundException, IOException 
    { 
        create(name, defaultSize);
    }

    /**
      This constructor is not for use by Java programs. Using this 
      constructor guarantees NullPointerExceptions.
     */
    public filebuf() { Cfilebuf = 0; }

    /**
      Closes the underlying file descriptor and frees any memory being used
      by the C version of the filebuf.
     */
    public native void close();
}
