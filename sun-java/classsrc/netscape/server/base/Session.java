package netscape.server.base;

import netscape.server.base.netFD;
import netscape.server.base.netbuf;
import netscape.server.base.pb_param;
import netscape.server.base.pblock;
import java.lang.String;
import java.io.IOException;
import java.lang.IndexOutOfBoundsException;


/**
 * A Session object represents one browser connection. A Session can be 
 * comprised of multiple Requests if the underlying protocol supports it.
 * The Session provides a centralized location for data relating to the 
 * remote client, and for communicating with the client.
 *
 * @author	Rob McCool
 */


public class Session {
    private static final int HASHSIZE = 4;

    /**
      The incoming data from the client. Users may either read from this
      netbuf, or use the read and getline methods.
     */
    public netbuf inbuf;
    /**
      The socket with the client's connection. Use inbuf for reading
      data from the client, and it is recommended that you use the Session's
      write methods to send data to the client. Because C NSAPI allows you 
      to write to the socket directly, this is made public.
     */
    public netFD csd;
    private pblock client;
    private int Csession;

    /**
      Constructing this object from anything except native code guarantees
      NullPointerExceptions.
     */
    public Session() {
        Csession = 0;
    }
    /**
      Read bytes from the input buffer until the entire array b is filled.
      An IOException indicates a disconnected client or similar I/O problem.
     */
    public void read(byte b[]) throws IOException {
        inbuf.getbytes(b);
    }
    /**
      Read one line (terminated by a carriage return and line feed) from the
      client, and return as a string. Do not allow more than maxsize bytes.
     */
    public String getline(int maxsize) 
        throws IOException, IndexOutOfBoundsException 
    {
        return inbuf.getline(maxsize);
    }
    /**
      Write the contents of the given byte array to the client. Throws 
      IOException when a client cannot receive the data.
     */
    public void write(byte b[]) throws IOException {
        csd.write(b);
    }
    /**
      Write len bytes starting at the given offset in the byte array b. 
      Throws OutOfBoundsExceptions if len specifies data past the end of
      the array, IOException on I/O error.
     */
    public void write(byte b[], int offset, int len) throws IOException {
        csd.write(b, offset, len);
    }
    /**
      Write the given String to the client. Throws IOException on I/O error.
     */
    public void write(String s) throws IOException {
        byte tmp[] = new byte[s.length()];
        s.getBytes(0, s.length(), tmp, 0);
        csd.write(tmp);
    }
    /**
      Write the given String to the client, followed by a line feed. 
      Throws IOException on error.
     */
    public void writeln(String s2) throws IOException {
        write(s2 + "\n");
    }
    /**
      Return the DNS hostname of the client, if available. null otherwise.
      Do not verify that the record is accurate. The response is cached
      in the client pblock to avoid multiple lookups.
     */
    public String dns() {
        return lookup(false);
    }
    /**
      Return the DNS hostname of the client, if available. null otherwise.
      Verify that the record is accurate by looking up the returned name
      and finding the client's IP address on the returned list. The response
      is cached in the client pblock to avoid multiple lookups.
     */
    public String maxdns() {
        return lookup(true);
    }
    private native String Clookup(boolean verify);
    private String lookup(boolean verify) {
        String ret;
        if((ret = client.findval("dns")) != null)
            return ret;
        return Clookup(verify);
    }
    /**
      Returns data about the client using the given name as a key. "ip"
      is the remote IP address.
     */
    public String getClientParam(String name) {
        return client.findval(name);
    }
}
