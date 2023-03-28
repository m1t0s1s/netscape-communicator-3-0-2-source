package netscape.server.base;

import netscape.server.base.netFD;

import java.io.IOException;
import java.lang.IndexOutOfBoundsException;

/**
 * Buffers are an abstract object for buffering input data. They do not 
 * handle output data.<p>
 *
 * @author	Rob McCool
 */

public abstract class Buffer {
    /**
      getbytes transfers at most maxlength bytes into b at the given
      offset. If an I/O error occurs, an IOException is thrown. maxlength
      bytes will be transferred unless an end of data condition is 
      encountered. This call will block if data is unavailable.

      Returns the number of characters actually transferred.
     */
    public abstract int getbytes(byte[] b, int offset, int maxlength)
        throws IOException;

    /**
      An alternate interface to getbytes which simply fills the entire bytes
      array starting at offset zero. This call will block if data is 
      unavailable.
     */
    public int getbytes(byte[] b) throws IOException {
        return getbytes(b, 0, b.length);
    }

    /**
      Returns the next byte available from the underlying object as an 
      integer, or -1 on EOF. This call will block if data is unavailable.
     */
    public int getc() throws IOException {
        // XXXrobm Some optimization may be necessary here.
        byte[] barray = new byte[1];
        if(getbytes(barray, 0, 1) == 0)
            return -1;
        return barray[0];
    }

    /**
      Scans in a line from the input source. Does not transfer the trailing
      carriage return or linefeed. Does not handle carriage returns in the
      middle of the line. 

      Will only transfer up to maxlength bytes for a line. If the line is
      longer than that, an IndexOutOfBoundsException is thrown.
     */
    public String getline(int maxlength) 
        throws IOException, IndexOutOfBoundsException 
    {
        StringBuffer line = new StringBuffer();
        int i, x;

      getchars:
        for(x = 0; x < maxlength; ++x) {
            i = getc();
            switch(i) {
              case (int) '\r':
                continue getchars;
              case (int) '\n':
              case -1:
                return line.toString();
              default:
                line.append((char) i);
            }
        }
        throw(new IndexOutOfBoundsException());
    }

    /**
      Sends all remaining data from the input source to the given
      object. Returns the number of bytes transferred. Throws an IOException
      when an error occurs.
     */
    public abstract int sendToSocket(netFD sd) throws IOException;

    /**
      Buffers require an explicit close to free any underlying system
      resources they are consuming.
     */
    public abstract void close();
}
