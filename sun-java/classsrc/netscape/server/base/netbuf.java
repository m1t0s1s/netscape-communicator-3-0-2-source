package netscape.server.base;

import netscape.server.base.Buffer;
import netscape.server.base.netFD;
import java.io.IOException;

/**
 * netbuf is a type of Buffer which handles network I/O to a socket. With
 * an SSL capable server, the underlying socket may transparently encrypt
 * and decrypt data for you. 
 *
 * @author	Rob McCool
 */


public class netbuf extends Buffer {
    /**
      The network descriptor is made public for convenience when
      writing data to the socket. Reading from this socket will
      guarantee confusion.
     */
    public netFD sd;

    private int Cnetbuf;
    private final int sizelimit = 128 * 1024;
    private static final int defaultSize = 4096;

    public native int getbytes(byte[] b, int offset, int maxlength)
        throws IOException;

    /**
      Send length bytes from the netbuf's underlying data source to the 
      given network socket. This is different from the default Buffer
      behavior in that it does not simply transfer until end of data.
     */
    public native int sendToSocket(netFD sd, int length) throws IOException;

    public int sendToSocket(netFD sd) throws IOException {
        return sendToSocket(sd, -1);
    }


    private native void create(netFD csocket, int size);

    /**
      Create a new netbuf using the given socket as the data source, and
      using a buffer of the given size. The size may not exceed 128 * 1024.
     */
    public netbuf(netFD csocket, int size) { 
        sd = csocket;
        create(csocket, size);
    }
    /**
      Create a new netbuf using the given socket as the data source, and 
      using a default buffer size.
     */
    public netbuf(netFD csocket) { 
        sd = csocket;
        create(csocket, defaultSize);
    }
    /**
      This constructor is not for use by Java programs. Using it guarantees
      NullPointerExceptions when the netbuf is accessed.
     */
    public netbuf() { Cnetbuf = 0; }

    /**
      Free the underlying C netbuf structure and its memory. Does not close
      the netbuf's socket.
     */
    public native void close();
}
