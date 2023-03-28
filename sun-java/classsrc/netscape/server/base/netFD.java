
package netscape.server.base;
import java.io.IOException;

/**
 * A netFD is a socket descriptor used for performing I/O with a networked
 * client. Limited facilities are provided primarily to be used to communicate
 * with a client already connected with the server.
 *
 * @author	Rob McCool
 */


public class netFD {
    private int Csocket;

    /**
      Construct a net class from an existing socket descriptor
     */
    public netFD(int fd) {
        Csocket = fd;
    }

    /**
      Write length bytes to the socket from the byte array b starting
      at the given offset
     */
    public native void write(byte b[], int offset, int length) 
        throws IOException;

    /**
      Write the entire byte array b to the socket.
     */
    public void write(byte b[]) throws IOException{  
        write(b, 0, b.length);
    }

    /**
      Read length bytes into byte array b starting at the given offset.
      Returns the number of bytes actually read.
      Will wait a maximum of timeout seconds to read the data, if no
      data arrives an IOException is thrown. Setting a timeout of zero
      will wait indefinitely.
     */
    public native int read(byte b[], int offset, int length, int timeout)
        throws IOException;
}
